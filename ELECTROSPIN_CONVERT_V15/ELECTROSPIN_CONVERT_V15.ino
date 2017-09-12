/*==============================================================
DUAL AXIS ELECTROSPINNING RIG
This code operates two stepper motors to actuate spindle rotation and y axis movement.
switches 2 heating element outputs to control temperature.

INPUT values

Spindle rpm
Spindle Diameter
Spindle surface speed (rotation)
Spindle Surface speed (Y axis - lateral motion)
? Spindle surface distance from needle
Enclosure temp
enclosure humidity
spindle temperature

OUTPUT values

Spindle Stepper speed
Spindle Stepper direction
Y- axis Stepper speed
Y- axis Stepper direction

Humidity control fan
Spindle Heater
Enclosure heater
==============================================================*/
#include <TimerOne.h>
#include <TimerThree.h>

/*==============================================================
Definitions
==============================================================*/
#define Y_STEPS_PER_MM 320
#define X_STEPS_PER_MM 8000
//
#define EG_SPINDLE 1.0 
#define EG_Y 1.0
//OUTPUTS
//Spindle is linked to E0 on ramps baord
#define S_STEP 26
#define S_DIR 28
#define S_EN 24

//y axis is linked to X axis on ramps board
#define Y_STEP 54 
#define Y_DIR 55
#define Y_LIM 3
#define Y_EN 38

//x axis is linked to z axis on ramps board
#define X_LIM 18
#define X_STEP 46
#define X_DIR 48
#define X_EN 62

//#define EN_HEAT 8
//#define S_HEAT

//#define DHT 5
//#define DS18B20 4

//INPUTS
#define spindle_steps_rev 400
#define Y_steps_rev 100
#define y_accel 50
#define s_accel 20

#define spindle_microsteps = 0; //set to the microstepping value on your stepper motor drivers (for gecko drivers this has to be calculated by measuring your system and using ratios)
#define y_microsteps = 0;
#define x_microsteps = 0;
/*==============================================================
//Variables
==============================================================*/

long debouncing_time = 5; //Debouncing Time in Milliseconds
long y_step_count=0;// tracking y axis movement actual step count will be half this number rounded down

float current_spindle_sps=100.0; // current spindle speed in stepper motor steps per second (sps)
unsigned long target_spindle_sps=0; //global vairbale for storage of target spindle speed prior to updating the speed.

unsigned long y_step_period=1000000;

int spindle_step_period=1000000/current_spindle_sps;

float spindle_micro_rat = 669.28; //this value incorporates the microstepping and the mechanical gear ratio into one number. obtain it by setting it to a random value and measuring the resulting motion then multiplying this value by the ration between input speed and output speed.
float max_pos=0;
float min_pos=0;
float x_pos=0;
long target_x_pos=0;
long previous_x_pos=0;

boolean y_motion=false;
boolean backAndForth=false; // false means moving away from home point
boolean current_y_dir=false; //false means away from limit switch or digital value low on direction pin.
boolean SPINNING = false;
volatile boolean Y_LIMIT = false;
volatile boolean X_LIMIT = false;
volatile boolean y_move = false;



volatile unsigned long y_axis_position=0;
volatile unsigned long x_axis_position=0;
volatile unsigned long y_axis_target_pos=0;
volatile unsigned long last_micros;
/*==============================================================
//SETUP
==============================================================*/

void setup() {
//set I/O
  pinMode(Y_STEP, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
  pinMode(Y_EN, OUTPUT);
  
  pinMode(X_STEP, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(X_EN, OUTPUT);

  pinMode(S_STEP, OUTPUT);
  pinMode(S_DIR, OUTPUT);
  pinMode(S_EN, OUTPUT);
    
  pinMode(Y_LIM, INPUT_PULLUP);
  pinMode(X_LIM, INPUT_PULLUP);
  
  Serial.begin(115200);
  Serial.println("READY");
}
/*==============================================================
// MAIN
==============================================================*/

void loop() {


  if(Serial.available()){
    receive_command();
  }

  if(SPINNING && !y_move){ // if system started move the y axis between start and end position perpetually


      if(!backAndForth){
        move_to(long(max_pos));
      }
      else{
        
        move_to(long(min_pos));

      }
    
  }


}

/*==============================================================
FUNCTIONS
==============================================================*/
void receive_command(){
  String command = Serial.readStringUntil('\n');

   if (command.substring(0,command.indexOf(":")) == "SS") { //(SS = spindle speed) check if the characters from start of string until ":" 
      int spindle_rpm = command.substring(command.indexOf(":") + 1).toInt(); //capture the characters after the ":" and convert to an integer
      Serial.print("SPINDLE_RPM:   ");
      Serial.println(spindle_rpm);
      
      target_spindle_sps = spindle_rps(0, spindle_rpm, 0); // calculate spindle steps per second required to attain the rpm.
      
      if(SPINNING){update_spindle(target_spindle_sps);}
      
      Serial.print("SPINDLE_SPS:    ");
      Serial.println(target_spindle_sps);
      
    } 
   if (command.substring(0,command.indexOf(":")) == "YS") { //YS = Y_START (start position of y axis)
      min_pos = command.substring(command.indexOf(":") + 1).toFloat()*Y_STEPS_PER_MM;
      Serial.print("ZERO_POS:   ");
      Serial.println(min_pos);
    } 
    if (command.substring(0,command.indexOf(":")) == "YE") {// YE = Y_END (end position of y axis)
      max_pos = command.substring(command.indexOf(":") + 1).toFloat()*Y_STEPS_PER_MM;
      if(max_pos>(150.0*Y_STEPS_PER_MM)){
        max_pos=150.0*Y_STEPS_PER_MM;
      }
      Serial.print("MAX_POS:   ");
      Serial.println(max_pos);
    } 
     if (command.substring(0,command.indexOf(":")) == "YV") {//YV = Y_VELOCITY (speed y axis moves between start and end positions
      float y_speed = command.substring(command.indexOf(":") + 1).toFloat();
      Serial.print("Y_SPEED:   ");
      Serial.println(y_speed);
      y_speed=1000000/(y_speed*Y_STEPS_PER_MM);
      y_step_period=long(y_speed);
    }
    if (command.substring(0,command.indexOf(":")) == "XP") { //YS = Y_START (start position of y axis)
      x_pos = command.substring(command.indexOf(":") + 1).toFloat();
      target_x_pos=long(x_pos)*X_STEPS_PER_MM;
     /* if(target_x_pos>(180*X_STEPS_PER_MM)){
          target_x_pos=180*X_STEPS_PER_MM;
      }*/
     /* if(SPINNING){
        target_x_pos=float(previous_x_pos)-target_x_pos;
        
        if(x_pos>0){
          digitalWrite(X_DIR, HIGH);//direction Toward limit switch
            move_to_x(target_x_pos);
            previous_x_pos=target_x_pos;
        }
        
        if(target_x_pos<0){
            digitalWrite(X_DIR, LOW);//direction away from limit switch
            target_x_pos=long(target_x_pos*(-1));
            move_to_x(target_x_pos);
            previous_x_pos=target_x_pos;
        }

      }*/
      
      Serial.print("X Position:   ");
      Serial.println(x_pos);
    } 
     if (command.substring(0,command.indexOf(":")) == "EN") {//
        enable_steppers();
    } 
    if (command.substring(0,command.indexOf(":")) == "DS") {//
        disable_steppers();
    } 
     if (command.substring(0,command.indexOf(":")) == "START") { //enable stepper motors and begin motion
      //START
      Serial.println("STARTING SYSTEM");
      Serial.print("Current Y AXIS position");
      Serial.println(y_axis_position);
      init_timers();
      //enable steppers
      enable_steppers();
      //home Y_axis and move to start position
      home_axes();
      //move to target x position
        digitalWrite(X_DIR, LOW);//direction away from limit switch
      move_to_x(target_x_pos);
      
      //start spindle rotation
      update_spindle(target_spindle_sps);

     move_to(long(min_pos));
     SPINNING = true;

     
    } 
     if (command.substring(0,command.indexOf(":")) == "STOP") { //enable stepper motors and begin motion
      //START
      Serial.println("STOPPING SYSTEM");
      SPINNING=false;
      disable_steppers();
    }
    return;
  
}
//==============================================================
int y_rps(float surf_speed, float diameter){
  
  float dist_per_rev = 2*PI*(diameter/2); 
  float rps = surf_speed/dist_per_rev;
  int y_steps_sec = int(rps * Y_steps_rev);
  
  return(y_steps_sec);
}
//==============================================================
  //change spindle step frequency here                                            UPDATE SPINDLE
//==============================================================
void update_spindle(float spindle_sps){

    
    while(spindle_sps > current_spindle_sps){// accelerate to target speed
      
        spindle_step_period = spindle_step_period - 1;
        Timer1.setPeriod(spindle_step_period);            //set step period in microseconds
        current_spindle_sps=(1000000/spindle_step_period); //calculate current steps per second from step period
        delayMicroseconds(current_spindle_sps*4);//this is accel rate
    }
    
      while(spindle_sps < current_spindle_sps){//deccelerate to target speed
        
        spindle_step_period = spindle_step_period + 1;
        Timer1.setPeriod(spindle_step_period);
        current_spindle_sps=(1000000/spindle_step_period);
        delayMicroseconds(current_spindle_sps/5);//this is accel rate
      
      }

}
//==============================================================
//==============================================================       SPINDLE SPEED CALCULATION
//==============================================================
int spindle_rps(float surf_speed, int rpm, float diameter){
  unsigned long spindle_sps=0;
      
      
  if(surf_speed>0){
    
    float dist_per_rev = PI*diameter; 
    float rps = surf_speed/dist_per_rev;
    spindle_sps = float(rps * spindle_steps_rev * spindle_micro_rat);
  }
  else if(rpm>0){
    Serial.println(rpm);
    spindle_sps= long((rpm/60.0)*spindle_micro_rat);
  }
  else{
    return spindle_sps=0;
  }
  
  return (spindle_sps);
  
}

//==============================================================
//==============================================================          Y_AXIS UPDATE
//==============================================================
void update_y_axis(long y_period, int y_axis_direction){
  
  enable_y_axis_motion();//enable y_axis_timer interrupt 
      
  digitalWrite(Y_DIR, y_axis_direction);
  
  if(y_axis_direction==1){
    current_y_dir=true;//towards limit switch
  }
  else{
    current_y_dir=false;//away from limit switch
  }
  Timer3.setPeriod(y_period);
  return;
}
//==============================================================
//==============================================================          ENABLE STEPPERS
//==============================================================
void enable_steppers(){
  
  Serial.println("Motors Enabled");
   digitalWrite(S_EN, LOW);
   digitalWrite(X_EN, LOW);
   digitalWrite(Y_EN, LOW);
  return; 
  
}
//==============================================================          Disable Steppers
void disable_steppers(){
  current_spindle_sps=0;
  Serial.println("Motors Disabled");
   digitalWrite(S_EN, HIGH);
   digitalWrite(X_EN, HIGH);
   digitalWrite(Y_EN, HIGH);
  return; 
  
}
//==============================================================
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++              HOME AXES
//==============================================================
void home_axes(){
  Y_LIMIT=false; //reset the limit switch
  update_y_axis(500, 1);//set y axis direction towards limit switch
  
  while(!Y_LIMIT){
    if(digitalRead(Y_LIM)==HIGH){Y_LIMIT=true;}
  }
  
   disable_y_axis_motion();
   y_axis_position=0;
   Y_LIMIT=false; //reset the limit switch
   Serial.println("Y_LIMIT TRIGGEERED");
  
  delay(1000);
  
  Serial.println("Homing X Axis");
  digitalWrite(X_DIR, HIGH); //set direction towards limit switch
  X_LIMIT=false;
  
  while(!X_LIMIT){
    digitalWrite(X_STEP, digitalRead(X_STEP) ^ 1);
    delayMicroseconds(30);
    if(digitalRead(X_LIM)==HIGH){X_LIMIT=true;}
  }
  
    disable_x_axis_motion();
    x_axis_position=0;
    X_LIMIT=false;
    Serial.println("X_LIMIT TRIGGERED");

  
}
//==============================================================              TIMER INTERRUPTS
void spindle_step(){
    digitalWrite(S_STEP, digitalRead(S_STEP) ^ 1);
}
//==============================================================
void Y_axis_step(){
  
  digitalWrite(Y_STEP, digitalRead(Y_STEP) ^ 1);
  
  if(current_y_dir){
    y_axis_position+=-1;}
  else{
    y_axis_position+=1;
  }


    if(y_axis_position==y_axis_target_pos && y_move){
      disable_y_axis_motion();
      Serial.println("Y AXIS POSITION AFTER MOTION:  ");
      Serial.println(y_axis_position);
      y_move=false;
    }

return;  
}
//==============================================================              Y AXIS TIMER INTERRUPT CONTROL
void disable_y_axis_motion(){
  Serial.println("MOTION DISABLED");
  Timer3.detachInterrupt();
  return;
}
void enable_y_axis_motion(){
  Serial.println("MOTION ENABLED");
  Timer3.attachInterrupt(Y_axis_step);
  return;
}
void disable_x_axis_motion(){
  Serial.println("X axis disabled");
  digitalWrite(X_EN, LOW);
}
//==============================================================              Y_AXIS MOTION CONTROL
void move_to(unsigned long y_pos_in_steps){
  
Serial.print("Moving to position:   ");
Serial.println(y_pos_in_steps);
  
  y_axis_target_pos=y_pos_in_steps;
  y_move=true;
  if(y_pos_in_steps>y_axis_position){
    update_y_axis(y_step_period, 0);

    backAndForth=true; //axis moving away from limit switch
  }
  else{
    update_y_axis(y_step_period, 1);

    backAndForth=false; //axis moving towards limit switch
  }
    
    return;
}
//==============================================================              X_AXIS MOTION CONTROL
void move_to_x(unsigned long x_pos_in_steps){
  //unsigned long i=0;
  for(unsigned long i=0; i<x_pos_in_steps; i++){
    digitalWrite(X_STEP, digitalRead(X_STEP) ^ 1);
    delayMicroseconds(50);
  }
}
//==============================================================            ENABLE ALL TIMERS with high period
void init_timers(){
    Timer3.initialize(1000000);
    Timer1.initialize(1000000);
    Timer1.attachInterrupt(spindle_step); //attach service routine
    //Timer3.attachInterrupt(Y_axis_step);
return;
}

  
   
