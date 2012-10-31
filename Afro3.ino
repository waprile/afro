//Afro Arduino FRObbler
//WA Aprile and AJC van der Helm
//Studiolab, TUDelft

//v 0.1.2 added register file, with an eye to condition->action patterns
//v 0.1.1 added power voltage meter code. Now writing tests.
//v 0.1.0 1 byte added to payload
//v 0.0.7 userTask hook added
//v 0.6.1 fixed bug in broadcast code (cast into signed char, ouch)
//v 0.0.6 added hashed delay before responding to a broadcast to reduce collisions
//v 0.0.5 set one before highest bit of MSB in response to indicate analog read
//v 0.0.4 conceptual automaton mistake fixed (palindrome)
//v 0.0.3 output is more quiet, added ping and broadcast
//v 0.0.2 fixes, added timed pulse and better semantics for PWM
//v 0.0.1 two hour hack
//v 0.0.0 you want WHAT in four days? October 2012

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
//AFZUX/xFF/xFFFA start user task, repeat forever
//AFZUx/x00/x00  FA start the user task, repeat 8224 times
//AFZUX/x00/x00FA stop user task
//AFZVxxxFA measures the supply voltage with a 10% uncertainty. Result is an unsigned int in millivolts.

//AFZQAxxFA queries the Arduino. Are you there?
//AFÿQAxxFA queries any Arduino out there.
//AFÿWNxxFA turn on all pin 13s on the network (disco)

//the ID of this Arduino. Important only if you are sharing a port with others
//as in the ZigBee case.

unsigned char id = 'O'; //

//enable to show debugging information about parsing and operations
//not so good on broadcast channels
boolean debug=false; 
#define REGISTERSIZE 10

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
//long int userTaskCounter=0;    //don't execute
long int userTaskCounter=-1;   //repeat forever

unsigned char userTaskOperand1; //a copy for the use of the userTask;

long int reg[REGISTERSIZE];

void userTask(long int t){
  static boolean blinkState=true;
  digitalWrite(13,blinkState);
  blinkState= !blinkState;
  /*
  Serial.print(userTaskCounter);
   Serial.print(" ");
   Serial.print(userTaskOperand1);
   Serial.print(" ");
   Serial.print(readVcc());
   Serial.println(t);
   */
}




void setup() {
  Serial.begin(57600);
  pinMode(13,OUTPUT);
  for(int i=0;i<REGISTERSIZE;i++) reg[i]=-1;
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
      Serial.write(res0);           //secondary result, one byte
      Serial.write(highByte(res));  //primary result, two bytes
      Serial.write(lowByte(res));
      Serial.print("RS");
    }
  }
  else{
    //no character to process, we check if we need to run the user task
    if ((userTaskCounter!=0) && ((millis()-t0)>userTaskInterval)){
      if (userTaskCounter>0) {
        userTaskCounter--;
      }
      t0=millis();
      userTask(t0);
    }
  }
}

int processBuffer(){
  static short int state=0;
  static short int oldstate=0;
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
  pinMode(state+3,OUTPUT);
  digitalWrite(state+3,HIGH);
  digitalWrite(oldstate+3,LOW);
  oldstate=state;
  
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
    else if (c=='I') { //state of all digital pins
      op=7;
    }
    else if (c=='V') { //measure the voltage input, good for battery testing
      op=8;
    }
    else if (c=='S') { //set a register
      op=9;
    }
    else if (c=='G') { //get a register
      op=10;
    }
    else {
      state=0;
      return -4;
    } //wrong operation
    break;

  case 4: //read first operand in
    operand1=c;
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

  default:
    return -7; //automaton hit a state it does not know. Very bad.

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
    operand1-=65; //A is pin 0
    pinMode(operand1,INPUT);
    digitalWrite(operand1,HIGH); //use contemporary method, connect pin to ground through switch.
    r=digitalRead(operand1);
    if (debug) {
      Serial.print("Digital read returned ");
      Serial.println(r);
    }
    if (r==0) r=-99;
    res0=operand1+97;
    return r ;
    break;

  case 1: //write operation
    operand1-=65; //A is pin 0
    pinMode(operand1,OUTPUT);
    digitalWrite(operand1,(operand2!=0));
    res0=operand1+97;
    return 0;
    break;

  case 2: //analog read
    res0=operand1;
    operand1-=65;
    r=analogRead(operand1);
    if (debug) {
      Serial.print("Analog read returned ");
      Serial.println(r);
    }
    if (r==0) r=-99;
    return r;
    break;

  case 3: //set PWM
    operand1-=65;
    analogWrite(operand1,operand2%256);
    return 0;
    break;

  case 4: //pulse
    operand1-=65;
    pinMode(operand1,OUTPUT);
    digitalWrite(operand1,HIGH);
    if (debug)    Serial.print("Pulse on");
    delay(operand2);
    if (debug)    Serial.print("Pulse off");
    digitalWrite(operand1,LOW);
    return 0;
    break;

  case 5: //PING
    return broadcastDelay;
    break;

  case 6: //User task control
    if (operand2==65535) {
      userTaskCounter=-1; 
    }
    else  {
      userTaskCounter=operand2; 
    }    
    userTaskOperand1=operand1;
    return 0;
    break;

  case 7: //Read all pins
    return PORTB * 256 + PORTD;
    break;

  case 8: //measure supply voltage
    if (debug) {
      Serial.println(readVcc());
    }
    return readVcc();
    break;

  case 9: //register set
    res0=operand1;
    operand1-=65;
    reg[operand1]=operand2;
    return 0;
    break;  

  case 10: //register get
    res0=operand1;
    operand1-=65;
    /*
    Serial.print("Porcodiobegin>");
    Serial.print(reg[operand1]);
    Serial.print("<Porcodioend");
    */
    r=reg[operand1];
    if (r==0) r=-99;
    return r;
    break;
    
  default:
    return -1; //can't happen
    break;
  }
}

unsigned int readVcc() {
  // from http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  // by Scott Daniels
  //
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

















