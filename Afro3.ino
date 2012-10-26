//Afro Arduino FRObbler
//WA Aprile and AJC van der Helm
//Studiolab, TUDelft

//v 0.1.0 1 byte added to payload
//v 0.0.7 userTask hook added
//v 0.6.1 fixed bug in broadcast code (cast into signed char, ouch)
//v 0.0.6 added hashed delay before responding to a broadcast to reduce collisions
//v 0.0.5 set one before highest bit of MSB in response to indicate analog read
//v 0.0.4 conceptual automaton mistake fixed (palindrome)
//v 0.0.3 output is more quiet, added ping and broadcast
//v 0.0.2 fixes, added timed pulse and better semantics for PWM
//v 0.0.1 two hour hack

//XX        1111     
//01234567890123
//ABCDEFGHIJKLMn

//RWAPTQU

//AFZAAxxFA read analog pin 0
//AFZRDxxFA read digital pin 3
//AFZWDxxFA write digital pin 3
//AFZPD @FA set digital pin 3 to 50% PWM
//AFZRHxxFA read digital pin 7 
//AFZTD  FA 8224 milliseconds pulse to pin 3
//AFZ
//AFZQAxxFA queries the Arduino. Are you there?
//AFÿQAxxFA queries any Arduino out there.
//AFÿWNxxFA turn on all pin 13s on the network (disco)



//the ID of this Arduino. Important only if you are sharing a port with others
//as in the ZigBee case.
unsigned char id = 'Z'; //

//enable to show debugging information about parsing and operations
//not so good on broadcast channels
boolean debug=false; 

//debug mode will toggle this pin regularly, to let you know the parser is working
int beeperPin=12;

//Afro will delay this much before answering a broadcast message, to make collisions less likely
int broadcastDelay = (id*79)%255;

unsigned char res0='Z'; //the secondary result from the last query
int res=0;              //the result from the last query

unsigned char requestID=0; //the last query the arduino has seen.
//Can be either the arduino ID or 255 (broadcast)

long int t0=0; //the last time the user task was run
long int userTaskInterval=100; //the time interval between calls to the user task
boolean doUserTask=false;


void userTask(){
  static boolean blinkState=true;
  digitalWrite(13,blinkState);
  blinkState= !blinkState;
}



void setup() {
  Serial.begin(57600);
  pinMode(13,OUTPUT);
}

void loop() {
  if (Serial.available()>0){ //fixed length messages
    res=processBuffer();
    if ((res>0)|(res==-99)) {
      //disgusting but necessary if some functions really need to return a zero
      //as a zero not as an indication that everything is peachy
      if (res==-99) {
        res=0;
      }
      if (requestID==255){
        delay(broadcastDelay);
      }
      Serial.print("SR");
      Serial.write(id);
      Serial.write(res0);
      Serial.write(highByte(res));
      Serial.write(lowByte(res));
      Serial.print("RS");
    }
  }
  else{
    if (doUserTask && ((millis()-t0)>userTaskInterval)){
      t0=millis();
      userTask();
    }
  }
}

int processBuffer(){
  static short int state=0;
  static unsigned char op=255;
  static unsigned char operand1=255;
  static int operand2=-1;
  char c;
  c= Serial.read();
  if (debug){
    digitalWrite(beeperPin,HIGH);
    delay(5);
    digitalWrite(beeperPin,LOW);
    Serial.print(c);
    Serial.print(" ");
    Serial.println(state);
  }
  switch(state) {

  case 0:
    if (c=='A') {
      state=1;
    }
    else {
      state=0; //reset automaton, go back to beginning and be ready for more input
      return -1;
    }
    break;

  case 1:
    if (c=='F') {
      state=2;
    }
    else if (c=='A') {
      state=1; //stay right were you are
    }
    else {
      state=0;
      return -2;
    }             
    break;

  case 2:
    requestID=c;
    if ((requestID!=id)&(requestID!=255)) {
      state=0;
      return -3;
    } //wrong ID
    else {
      state=3;
    }
    break;

  case 3:
    state=4;
    if (c=='R') { //digital Read
      op=0;
    }
    else if (c=='W') { //digital Write
      op=1;
    }
    else if (c=='A') { //Analog read
      op=2;
    }
    else if (c=='P') { //PWM Output
      op=3;
    }
    else if (c=='T') { //timed pulse
      op=4;
    }
    else if (c=='Q') { //Query ping
      op=5;
    }
    else if (c=='U') { //Control user task
      op=6;
    }

    else {
      state=0;
      return -4;
    } //wrong operation
    break;

  case 4: //read first operand in
    operand1=c-65;
    state=5;
    break;

  case 5: //read first byte of second operand
    operand2=256*c;
    state=6;
    break;

  case 6: //read second byte of second operand
    operand2+=c;
    state=7;
    break;

  case 7:
    if (c=='F')
      state=8;
    else{
      state=0;
      return -5; //bad finish
    }
    break;

  case 8:
    if (c=='A'){
      state=0;
      return execute(op,operand1,operand2);  //successful
    }
    else{
      state=0;
      return -6; //very bad finish at the very end
    }
    break;

  }
  return 0;

}

int execute(unsigned char operation,unsigned char operand1,int operand2){
  int r;
  res0=0;
  if (debug){
    Serial.println("----");
    Serial.print(operation);
    Serial.print(" pin=");
    Serial.print(operand1);
    Serial.print("  data=");
    Serial.println(operand2);
    Serial.println("----");
  }

  switch (operation) {
  case 0: //read
    pinMode(operand1,INPUT);
    digitalWrite(operand1,HIGH); //use contemporary method, connect pin to ground through switch.
    r=digitalRead(operand1);
    if (debug) {
      Serial.print("Digital read returned ");
      Serial.println(r);
    }
    if (r==0) r=-99;
    res0=operand1+97;
    return r;
    break;

  case 1: //write
    pinMode(operand1,OUTPUT);
    digitalWrite(operand1,(operand2!=0));
    return 0;
    break;

  case 2: //analog read
    r=analogRead(operand1);
    res0=operand1+65;
    if (debug) {
      Serial.print("Analog read returned ");
      Serial.println(r);
    }
    return r;
    break;

  case 3: //set PWM
    analogWrite(operand1,operand2%256);
    return 0;
    break;

  case 4: //pulse
    pinMode(operand1,OUTPUT);
    digitalWrite(operand1,HIGH);
    //Serial.print("Pulse on");
    delay(operand2);
    digitalWrite(operand1,LOW);
    return 0;
    break;

  case 5: //PING
    return broadcastDelay;
    break;

  case 6: //User task toggle
    doUserTask=(operand2!=0);
    if (doUserTask) {
      return 1;
    }
    else {
      return 0;
    }
    break;

  default:
    return -1; //can't happen
    break;
  }
}









