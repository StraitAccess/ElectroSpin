import controlP5.*;
import java.nio.ByteBuffer;
import processing.serial.*;


 Serial myPort;
 
 
ControlP5 cp5;
PFont f;
boolean settings_fetched = false;

String needle_dist, needle_spd, spindle_spd, needle_start, needle_end;

//======================================================================
//======================================================================
//======================================================================

void setup() {
  //open the serial port
    println(Serial.list());                                           // * Initialize Serial
  myPort = new Serial(this, Serial.list()[2], 115200);                //   Communication with
  myPort.bufferUntil(10); 
  
  
  // Create the font
  //printArray(PFont.list());
  f = createFont("swmono.ttf", 16);
  textFont(f);
    
  size(1024, 500);
  cp5 = new ControlP5(this);
  cp5.addTextfield("Distance From Spindle Core in millimeters")
    .setPosition(20, 100)
    .setSize(200, 40)
    .setAutoClear(false)
    .setValue("20")
    ;
  cp5.addTextfield("Needle Movement Speed")
  .setPosition(20, 270)
  .setSize(200, 40)
  .setAutoClear(false)
  .setValue("5")
  ;
  
  cp5.addTextfield("Needle START Position")
  .setPosition(320, 100)
  .setSize(200, 40)
  .setAutoClear(false)
  .setValue("20")
  ;
  
  cp5.addTextfield("Needle END Position")
  .setPosition(320, 170)
  .setSize(200, 40)
  .setAutoClear(false)
  .setValue("100")
  ;
  
  cp5.addTextfield("Spindle Speed in revolutions per minute")
  .setPosition(620, 100)
  .setSize(200, 40)
  .setAutoClear(false)
  .setValue("500")
  ;
    
    cp5.addButton("SEND_SETTINGS")
     .setBroadcast(false)
     .setValue(128)
     .setPosition(0,400)
     .setSize(500,100)
     .setBroadcast(true)
     ;
     
    cp5.addButton("STOP_MACHINE")
     .setBroadcast(false)
     .setValue(128)
     .setPosition(524,400)
     .setSize(500,100)
     .setBroadcast(true)
     ; 
     
     PImage[] imgs = {loadImage("button_a.png"),loadImage("button_b.png"),loadImage("button_c.png")};
     cp5.addButton("PLAY")
     .setBroadcast(false)
     .setValue(0)
     .setPosition(975,230)
     .setImages(imgs)
     .updateSize()
     .setBroadcast(true)
     ;
 
}
//======================================================================
//======================================================================
//======================================================================
 
void draw () {
  background(50);
  textAlign(CENTER);
  fill(255);
  text("NEEDLE DISTANCE", 120, 70);
  text("NEEDLE SPEED", 120, 240);
  text("SPINDLE SPEED", 720, 70);
  text("NEEDLE MOTION RANGE", 420, 70);
  
  //print Units
  textAlign(LEFT);
  text("MM", 240, 125);
  text("MM/Sec", 240, 295);
  text("MM", 540, 125);
  text("MM", 540, 195);
  text("RPM", 840, 125);
  
  rect(880, 229, 200, 51);
  fill(0);
  text("START", 900, 260);
  //frameRate(12);
  //println(mouseX + " : " + mouseY);
  
}
//======================================================================
//======================================================================
//======================================================================
 
void SEND_SETTINGS() {
  
    println("the following text was submitted :");
    
    needle_dist = cp5.get(Textfield.class,"Distance From Spindle Core in millimeters").getText();
    spindle_spd = cp5.get(Textfield.class,"Spindle Speed in revolutions per minute").getText();
    needle_spd = cp5.get(Textfield.class,"Needle Movement Speed").getText();
    needle_start = cp5.get(Textfield.class,"Needle START Position").getText();
    needle_end = cp5.get(Textfield.class,"Needle END Position").getText();

    myPort.write("SS:" + spindle_spd);
    myPort.write("\n");
    delay(10);
    myPort.write("XP:" + needle_dist);
    myPort.write("\n");
    delay(10);
    myPort.write("YS:" + needle_start);
    myPort.write("\n");
    delay(10);
    myPort.write("YE:" + needle_end);
    myPort.write("\n");
    delay(10);
    myPort.write("YV:" + needle_spd);
    myPort.write("\n");
    delay(10);


}

//======================================================================
void PLAY(){
  println("STARTING ELECTROSPINNING");
  myPort.write("START");
  myPort.write("\n");
  
}
//======================================================================
void STOP_MACHINE(){
  println("STOPPING ELECTROSPINNING");
  myPort.write("STOP");
  myPort.write("\n");
  
}

//======================================================================

byte[] floatArrayToByteArray(float[] input)
{
  int len = 4*input.length;
  int index=0;
  byte[] b = new byte[4];
  byte[] out = new byte[len];
  ByteBuffer buf = ByteBuffer.wrap(b);
  for(int i=0;i<input.length;i++) 
  {
    buf.position(0);
    buf.putFloat(input[i]);
    for(int j=0;j<4;j++) out[j+i*4]=b[3-j];
  }
  return out;
}


void serialEvent(Serial myPort)
{
  String read = myPort.readStringUntil(10);
  String[] s = split(read, " ");

  if (s.length ==9)
  {
    /*
    Setpoint = float(s[1]);           // * pull the information
    Input = float(s[2]);              //   we need out of the
    Output = float(s[3]);             //   string and put it
    SPLabel.setValue(s[1]);           //   where it's needed
    InLabel.setValue(s[2]);           //
    OutLabel.setValue(trim(s[3]));    //
    PLabel.setValue(trim(s[4]));      //
    ILabel.setValue(trim(s[5]));      //
    DLabel.setValue(trim(s[6]));      //
    AMCurrent.setValue(trim(s[7]));   //
    DRCurrent.setValue(trim(s[8]));
    if(justSent)                      // * if this is the first read
    {                                 //   since we sent values to 
      SPField.setText(trim(s[1]));    //   the arduino,  take the
      InField.setText(trim(s[2]));    //   current values and put
      OutField.setText(trim(s[3]));   //   them into the input fields
      PField.setText(trim(s[4]));     //
      IField.setText(trim(s[5]));     //
      DField.setText(trim(s[6]));     //
     // mode = trim(s[7]);              //
      AMLabel.setValue(trim(s[7]));         //
      //dr = trim(s[8]);                //
      DRCurrent.setValue(trim(s[8]));         //
      justSent=false;                 //
    }                                 //

    if(!madeContact) madeContact=true;*/
  }
}
