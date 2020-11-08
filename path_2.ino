// the timer code that drives the stepper motors is by Amanda Ghassaei
//the robot code is by Jon Rosenfeld and Phil Martel

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

//this code will enable timer2
//timer2 will interrupt at 8kHz

#include <Streaming.h> // This lets Serial use the C++ '<<' operator
#include <math.h>  //for atan(), atan2()

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
  const float wheelDiameter = 1.5625; // measured on 11/7
  const float wheelBase = 8.0;
  // these are for the 28BYJ-48 stepper motor
  const float stepsPerMotorRev = 32.0;
  const float gearRatio = 64.0;
  const float stepsPerWheelRev = stepsPerMotorRev * gearRatio;

  //const float pi = 3.14159; //guess what this is
  const float pi = 4. * atan(1.);
  const float rad2Deg = 180./pi;
  
  const float stepsPerInch = stepsPerWheelRev / (pi * wheelDiameter ) ;
  //const float degPerStep = ( 1./(stepsPerInch * (wheelBase/2))) * rad2Deg; // small angle approximation
  const float degPerStep = atan2( 1./(stepsPerInch), (wheelBase/2)) * rad2Deg; // exact
  const float stepsPerDegree = 1./ degPerStep;
#endif

////////////////////////////////////////////////////////////////
// Object to draw
////////////////////////////////////////////////////////////////

#define LIN true
#define ROT false

struct moveStruct
{
  boolean mtype;  // move type: linear or rotational TRUE for linear FALSE for rotational
  float amount;     // inches or angle if 0 then end of moves
};

// for constant array like this you don't need to specify a size,
// it's implicit
moveStruct moves[] = {  
  //#include "square.h"
  #include "yard.h"
  //#include "540cw.h"
  //#include "540ccw.h"
};

volatile unsigned int stepcnt;

volatile byte waveform;
volatile byte mask;
int freq;
int n;
int i;          // index
volatile int d;
volatile boolean moveDone;

// original names
#define STEPPIN1 8
#define DIRPIN1 9
#define STEPPIN2 10
#define DIRPIN2 11


// new names below
// this is how Phil's robot is set up on 11/5 
// it will probably change
// by using the old names, we get temporaroy compatibility
#define LEFT_STEP_PIN   STEPPIN2
#define LEFT_DIR_PIN    DIRPIN2
#define RIGHT_STEP_PIN  STEPPIN1
#define RIGHT_DIR_PIN   DIRPIN1

/* this is Phil's motor controller wiring 11/5 it will change 
 *  aming other issues, the wiring for the left motor is reversed
 *  eventually the code will handdle it
 *  Left Controller Left motor  Right controller  Right Motor
 *    3               blue          3               pink
 *    4               yellow        4               orange
 *    5               orange        5               yellow
 *    6                pink         6               blue
 */

// both motors enabled together
#define ENPIN 12


#define TESTPIN 13

void setup(){
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }  // prints title with ending line break
  Serial << "wheelDiameter " << wheelDiameter << endl;  
  Serial << "wheelBase " << wheelBase << endl; 
  Serial << "stepsPerWheelRev " << stepsPerWheelRev << endl;
  Serial << "degPerStep " << _FLOAT(degPerStep,6) << endl;
  Serial << "stepsPerInch " << stepsPerInch << endl;

  //set pins as outputs
  pinMode(RIGHT_STEP_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(LEFT_STEP_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(ENPIN, OUTPUT);
  pinMode(TESTPIN, OUTPUT);
  freq = 200;
 
  cli();//stop interrupts

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
  
  sei();//allow interrupts

}//end setup

void loop(){
  digitalWrite (TESTPIN, LOW);
  digitalWrite (ENPIN, HIGH);              // disable drivers
  for (i=0; ; i++)
  {
    TIMSK2 = 0;
    delay(1000);
    Serial << "Move[" << i <<"] " << (moves[i].mtype ? "linear " : "rotation ") << moves[i].amount ;
    if (moves[i].amount != 0) {
      if (moves[i].mtype == LIN) {
        digitalWrite (RIGHT_DIR_PIN, HIGH);
        digitalWrite (LEFT_DIR_PIN, HIGH);
        stepcnt = ((moves[i].amount)*stepsPerInch+ 0.5);
      }
      else
      {
        stepcnt = ((moves[i].amount)*stepsPerDegree + 0.5);
        if (moves[i].amount > 0)
        {
          digitalWrite (RIGHT_DIR_PIN, LOW);
          digitalWrite (LEFT_DIR_PIN, HIGH);
        }
        else
        {
          digitalWrite (RIGHT_DIR_PIN, HIGH);
          digitalWrite (LEFT_DIR_PIN, LOW);
          stepcnt = -stepcnt;
        }
      }

  Serial << " stepcnt = " << stepcnt << endl;
      moveDone = false;
      digitalWrite (TESTPIN, HIGH);          
      digitalWrite (ENPIN, LOW);              // enable drivers
      TCNT2  = 0;//initialize counter value to 0
      TIMSK2 |= (1 << OCIE2A);    // enable compare interrupts
      
      while (moveDone == false){}
      Serial << "Program done" <<endl;
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
  digitalWrite(RIGHT_STEP_PIN, HIGH);
  for (d=0; d<2;)
  {
    d++;
  }
  if (stepcnt-- == 0) moveDone = true;
  digitalWrite(RIGHT_STEP_PIN, LOW);
  
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
