<<<<<<< HEAD
// the timer code that drives the stepper motors is by Amanda Ghassaei
//the robot code is by Jon Rosenfeld and Phil Martel
=======
//phil
>>>>>>> 6958f574ab42770fffdb36b491e66171c2003fe9

//timer interrupts
//by Amanda Ghassaei
//June 2012
//https://www.instructables.com/id/Arduino-Timer-Interrupts/
// License from Amamda's code
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
*/

//timer setup for timer0, timer1, and timer2.
//For arduino uno or any board with ATMEL 328/168.. diecimila, duemilanove, lilypad, nano, mini...

//this code will enable all three arduino timer interrupts.
//timer0 will interrupt at 2kHz
//timer1 will interrupt at 1Hz
//timer2 will interrupt at 8kHz

#include <Streaming.h> // This lets Serial use the C++ '<<' operator
#define PHILS_ROBOT

#ifdef JONS_ROBOT
  // steps/inch
  #define wheelDiameter 2.34
  #define degPerStep 11.25                  // deg per full step   /1/5.625  //5.625/64.0
  #define stepsPerRev (360/degPerStep)/64   // full steps per rev of output shaft
  #define inchPerStep (3.1415*wheelDiameter/stepsPerRev)     //degPerStep/360.0) 
  #define stepsPerInch 1./inchPerStep
  #define wheelBase 6.4   // d8stance between wheels
  #define stepsPerDegree (wheelBase*3.1415/inchPerStep)/360
#endif

#ifdef PHILS_ROBOT
  // these dimensions (inches) are for Phil's robot
  const float wheelDiameter = 1.625; 
  const float wheelBase = 8.0;
  // these are for the 28BYJ-48 stepper motor
  const float stepsPerMotorRev = 32.0;
  const float gearRatio = 64.0;
  const float stepsPerWheelRev = stepsPerMotorRev * gearRatio;

  const float pi = 3.14159; //guess what this is
  const float rad2Deg = 180./pi;
  
  const float stepsPerInch = stepsPerWheelRev / (pi * wheelDiameter ) ;
  const float degPerStep = ( 1./(stepsPerInch * (wheelBase/2))) * rad2Deg; // small angle approxomation
  const float stepsPerDegree = 1./ degPerStep;
/*  
  #define degPerStep 11.25                  // deg per full step   /1/5.625  //5.625/64.0
  #define stepsPerRev (360/degPerStep)/64   // full steps per rev of output shaft
  #define inchPerStep (3.1415*wheelDiameter/stepsPerRev)     //degPerStep/360.0) 
  #define stepsPerInch 1./inchPerStep
  #define wheelBase 6.4   // d8stance between wheels
  #define stepsPerDegree (wheelBase*3.1415/inchPerStep)/360
*/
#endif


float test;

struct moveStruct
{
  boolean mtype;  // move type: linear or rotational TRUE for linear FALSE for rotational
  float amount;     // inches or angle if 0 then end of moves
};

moveStruct moves[20] = {

  {.mtype = true, .amount = 6}, 
  {.mtype = false, .amount = 90.0}, 
  {.mtype = true, .amount = 6}, 
  {.mtype = false, .amount = 90.0}, 
  {.mtype = true, .amount = 6}, 
  {.mtype = false, .amount = 90.0}, 
  {.mtype = true, .amount = 6}, 
  {.mtype = false, .amount = 90.0}, 
  {.mtype = false, .amount = 0.0} 
};

volatile unsigned int stepcnt;

volatile byte waveform;
volatile byte mask;
volatile int freq;
int n;
int i;          // index
volatile int d;
volatile boolean moveDone;

#define STEPPIN1 8
#define DIRPIN1 9
#define STEPPIN2 10
#define DIRPIN2 11
#define ENPIN 12

#define TESTPIN 13

void setup(){
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }  // prints title with ending line break
  Serial << "wheelDiameter " << wheelDiameter << endl;  
  Serial << "degPerStep " << degPerStep << endl;
  test = stepsPerInch;
  Serial << "stepsPerInch " << stepsPerInch << endl;

  //set pins as outputs
 pinMode(STEPPIN1, OUTPUT);
 pinMode(DIRPIN1, OUTPUT);
 pinMode(STEPPIN2, OUTPUT);
 pinMode(DIRPIN2, OUTPUT);
 pinMode(ENPIN, OUTPUT);
 pinMode(TESTPIN, OUTPUT);
 freq = 150;
 
cli();//stop interrupts

//set timer0 interrupt at 2kHz
/*  TCCR0A = 0;// set entire TCCR2A register to 0
  TCCR0B = 0;// same for TCCR2B
  TCNT0  = 0;//initialize counter value to 0
  // set compare match register for 2khz increments
  OCR0A = freq;// = (16*10^6) / (2000*64) - 1 (must be <256)
  // turn on CTC mode
  TCCR0A |= (1 << WGM01);
  // Set CS01 and CS00 bits for 64 prescaler
  TCCR0B |= (1 << CS02) | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK0 |= (1 << OCIE0A);

//set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A =  624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
*/
//set timer2 interrupt at 8kHz
  TCCR2A = 0;// set entire TCCR2A register to 0
  TCCR2B = 0;// same for TCCR2B
  TCNT2  = 0;//initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = freq;// = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS02) | (1 << CS01);  // | (1 << CS00);   
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);
//  TIMSK2 = 0;
sei();//allow interrupts

}//end setup

void loop(){
  digitalWrite (TESTPIN, LOW);
  digitalWrite (ENPIN, HIGH);              // disable drivers
  for (i=0; ; i++)
  {
    TIMSK2 = 0;
    delay(1000);
    if (moves[i].amount != 0) {
      if (moves[i].mtype) {
        digitalWrite (DIRPIN1, HIGH);
        digitalWrite (DIRPIN2, HIGH);
        stepcnt = (moves[i].amount)*stepsPerInch;
      }
      else
      {
        stepcnt = (moves[i].amount)*stepsPerDegree;
        if (moves[i].amount > 0)
        {
          digitalWrite (DIRPIN1, LOW);
          digitalWrite (DIRPIN2, HIGH);
        }
        else
        {
          digitalWrite (DIRPIN1, HIGH);
          digitalWrite (DIRPIN2, LOW);
          stepcnt = -stepcnt;
        }
      }

  test = stepcnt;
  Serial.println(test, 10);
      moveDone = false;
      digitalWrite (TESTPIN, HIGH);          
      digitalWrite (ENPIN, LOW);              // enable drivers
      TCNT2  = 0;//initialize counter value to 0
      TIMSK2 |= (1 << OCIE2A);    // enable compare interrupts
      
      while (moveDone == false){}
 
     
      TIMSK2 = 0;
      digitalWrite (ENPIN, HIGH);              // disable drivers
      digitalWrite (TESTPIN, LOW);
    }
    else
      break;
  }
  TIMSK2 = 0;
  digitalWrite (ENPIN, HIGH);              // disable drivers
  while (true) {}
}

ISR(TIMER2_COMPA_vect){
/*  if (digitalRead(STEPPIN))
    digitalWrite(STEPPIN, LOW);
  else
    digitalWrite(STEPPIN, HIGH);
*/
  digitalWrite(STEPPIN1, HIGH);
  for (d=0; d<2;)
  {
    d++;
  }
  if (stepcnt-- == 0) moveDone = true;
  digitalWrite(STEPPIN1, LOW);
  
 // OCR2A = freq; // update frequency
 // stepcnt = stepcnt - 1;
}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 1Hz toggles pin 13 (LED)
//generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)
/*  if (toggle1){
    digitalWrite(13,HIGH);
    toggle1 = 0;
  }
  else{
    digitalWrite(13,LOW);
    toggle1 = 1;
  } 
}
  
ISR(TIMER0_COMPA_vect){//timer1 interrupt 8kHz toggles pin 9
//generates pulse wave of frequency 8kHz/2 = 4kHz (takes two cycles for full wave- toggle high then toggle low)
/*  if (toggle2){
    digitalWrite(9,HIGH);
    toggle2 = 0;
  }
  else{
    digitalWrite(9,LOW);
    toggle2 = 1;
  } */
}
