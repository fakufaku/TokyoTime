
/*
 * (c) Fakufaku 2012
 * This is the code I use to control an oven I hacked.
 * This code is released under the beerware license, i.e. if you use it, and it is useful, pay me beer, maybe.
 * The part of the code that's used for the display on the 7-segment was ripped off the BigTime code from Nathan Seidl. I owe him a beer then.
 */

#include <math.h>

#include <PID_v1.h>

#define TRUE 1
#define FALSE 0

//Careful messing with the system color, you can damage the display if
//you assign the wrong color. If you're in doubt, set it to red and load the code,
//then see what the color is.
#define RED  1
#define GREEN 2
#define BLUE  3
#define YELLOW  4
int systemColor = GREEN;
int display_brightness = 8000; //A larger number makes the display more dim. This is set correctly below.

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Pin definitions
int digit1 = 10;    //Display pin 12
int digit2 = 13;    //Display pin 9
int digit3 = A1;    //Display pin 8
int digit4 = 3;     //Display pin 6

int segA = 11;      //Display pin 11
int segB = A2;      //Display pin 7
int segC = 5;       //Display pin 4
int segD = 7;       //Display pin 2
int segE = 8;       //Display pin 1
int segF = 12;      //Display pin 10
int segG = 4;       //Display pin 5

int colons = 6;     //Display pin 3

//Set this variable to change how long the time is shown on the watch face. In milliseconds so 1677 = 1.677 seconds
int show_time_length = 2000;

int temp_pwr = A3;  // to power the temperature sensor
int temp_sen = A0;  // to read the temperature sensor

const static int temp_out = A0;
const static int temp_out_pwr = A3;

const static int buzz = 9;
const static int theButton = 2;

unsigned long timer = 0;

// slow continuous PWM variables
#define PWM_MS 5000
#define PWM_MIN 250
#define RELAY_ON HIGH
#define RELAY_OFF LOW
unsigned long windowStartTime;
const static int pwm_pin = A5;  // SCL, brown wire

// control variables for PID
double t_incub;      // input to PID
double t_incub_N;    // number of samples in t_incub average
double pwm_duty;    // output of PID
double t_set = 0; // set point

// oven max temperature
#define MAX_TEMP 50
#define TEMP_INCREMENT 1
#define MAX_TIME (24*60*60)
#define TIME_INCREMENT 60

//Specify the links and initial tuning parameters
double kp=50,ki=0.3
,kd=1;
PID myPID(&t_incub, &pwm_duty, &t_set, kp, ki, kd, DIRECT);

// display timer (freeze parameters to display for 2 seconds)
int t_incub_disp;

// Timer variables
unsigned long timer_seconds = 0;

//The very important 32.768 interrupt handler
SIGNAL(TIMER2_OVF_vect)
{
  if (timer_seconds > 1)
  {
    timer_seconds--;
  }
  else if (timer_seconds == 1)
  {
    // reset timer
    timer_seconds = 0;
    // turn off by setting target temperature to zero
    t_set = 0;
    // Reset the PID
    myPID.SetMode(MANUAL);
    myPID.SetMode(AUTOMATIC);
  }
}

void setup()
{
  //Serial.begin(57600);

  // set ADC reference to 3.3V
  analogReference(DEFAULT);

  // initialize temperature sensor and turn on
  pinMode(temp_out_pwr, OUTPUT);
  digitalWrite(temp_out_pwr, HIGH);

  // initialize pwm drive pin, and turn it off
  pinMode(pwm_pin, OUTPUT);
  digitalWrite(pwm_pin, HIGH);

  // initialize buzzer pin
  pinMode(buzz, OUTPUT);

  // initialize the button
  pinMode(theButton, INPUT);
  digitalWrite(theButton, HIGH); // pull-up

  //Setup TIMER2
  TCCR2A = 0x00;
  TCCR2B = (1<<CS22)|(1<<CS20); //Set CLK/128 or overflow interrupt every 1s
  ASSR   = (1<<AS2); //Enable asynchronous operation
  TIMSK2 = (1<<TOIE2); //Enable the timer 2 interrupt

  //These pins are used to control the display
  pinMode(segA, OUTPUT);
  pinMode(segB, OUTPUT);
  pinMode(segC, OUTPUT);
  pinMode(segD, OUTPUT);
  pinMode(segE, OUTPUT);
  pinMode(segF, OUTPUT);
  pinMode(segG, OUTPUT);

  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
  pinMode(digit3, OUTPUT);
  pinMode(digit4, OUTPUT);
  pinMode(colons, OUTPUT);

  // setup PID stuff
  myPID.SetOutputLimits(PWM_MIN-1, PWM_MS-PWM_MIN+1);
  myPID.SetMode(AUTOMATIC);
  myPID.SetSampleTime(PWM_MS);
  windowStartTime = millis();

  // initialize pwm
  pwm_duty = 0; // off
  t_incub = read_temperature();
  t_incub_N = 1;

  // initialize slow variables for display
  t_incub_disp = t_incub;

}

void loop()
{
  
  temperatureControl();

  // display and output to serial
  displayTemperature((int)(t_incub_disp), FALSE); //Each call takes about 8ms, display the colon

  if (digitalRead(theButton) == LOW)
  {
    // set temperature first
    setTemperature();
    // then time
    setTime();
  }

}

// Simple PWM based on millis directly
void temperatureControl()
{

  unsigned long now = millis();

  t_incub += (read_temperature() - t_incub)/(++t_incub_N);

  if(now - windowStartTime > PWM_MS)
  { 
    // compute new pwm value for that window
    myPID.Compute();

    // start a new relay window
    windowStartTime = now;

    // restart averaging of oven temperature
    t_incub_disp = t_incub;
    t_incub = read_temperature();
    t_incub_N = 1;

  }

  // if t_set is zero, that means oven is off
  if (t_set < 1)
  {
    pwm_duty = 0;
  }

  if (pwm_duty < PWM_MIN)
  {
    digitalWrite(pwm_pin, RELAY_OFF);
  }
  else if (pwm_duty < PWM_MS-PWM_MIN)
  {
    if(pwm_duty > now - windowStartTime) 
      digitalWrite(pwm_pin, RELAY_ON);
    else 
      digitalWrite(pwm_pin, RELAY_OFF);
  }
  else
  {
    digitalWrite(pwm_pin, RELAY_ON);
  }
}

float read_temperature()
{
  float A = 0;
  for (int i=0 ; i < 10 ; i++)
    A += analogRead(temp_out);
  A /= 10;
  return (A/1023.*3300 - 600)/10;
}

//This routine occurs when you hold the button down
//The colon blinks indicating we are in this mode
//Holding the button down will increase the time (accelerates)
//Releasing the button for more than 2 seconds will exit this mode
void setTemperature()
{

  int idleMiliseconds = 0;
  //This is the timeout counter. Once we get to ~2 seconds of inactivity, the watch
  //will exit the setTime function and return to normal operation

  while(idleMiliseconds < 2000) {

    for(int x = 0 ; x < 7 ; x++) {
      displayTemperature(t_set, TRUE); //Each call takes about 8ms, display the colon for about 100ms
      delayMicroseconds(display_brightness); //Wait before we paint the display again
    }
    for(int x = 0 ; x < 7 ; x++) {
      displayTemperature(t_set, FALSE); //Each call takes about 8ms, turn off the colon for about 100ms
      delayMicroseconds(display_brightness); //Wait before we paint the display again
    }

    //If you're still hitting the button, then increase the time and reset the idleMili timeout variable
    if(digitalRead(theButton) == LOW) {
      idleMiliseconds = 0;

      t_set = ((int)(t_set+TEMP_INCREMENT) % MAX_TEMP); // Increase temperature
    }

    idleMiliseconds += 200;
  }
}

//This routine occurs when you hold the button 2 down
//The colon blinks indicating we are in this mode
//Holding the button down will increase the time (accelerates)
//Releasing the button for more than 2 seconds will exit this mode
void setTime()
{
  int idleMiliseconds = 0;
  //This is the timeout counter. Once we get to ~2 seconds of inactivity, the watch
  //will exit the setTime function and return to normal operation

  while(idleMiliseconds < 2000) 
  {

    int timer_disp = (timer_seconds/3600)*100 + (timer_seconds/60)%60 
      + (timer_seconds % 60 == 0 ? 0 : 1);

    for(int x = 0 ; x < 10 ; x++) {
      displayTime(timer_disp, TRUE); //Each call takes about 8ms, display the colon for about 100ms
      delayMicroseconds(display_brightness); //Wait before we paint the display again
    }
    for(int x = 0 ; x < 10 ; x++) {
      displayTime(timer_disp, FALSE); //Each call takes about 8ms, turn off the colon for about 100ms
      delayMicroseconds(display_brightness); //Wait before we paint the display again
    }

    //If you're still hitting the button, then increase the time and reset the idleMili timeout variable
    if(digitalRead(theButton) == LOW) {
      idleMiliseconds = 0;

      timer_seconds = ((int)(timer_seconds+TIME_INCREMENT) % MAX_TIME); // Increase timer
    }

    idleMiliseconds += 200;
  }
}


/************************************/
/* Routines to display on 7-segment */
/************************************/

//Given 102, we display "102.°"
//Each digit is displayed for ~2000us, and cycles through the 4 digits
//After running through the 4 numbers, the display is turned off
void displayTemperature(int toDisplay, boolean displayColon)
{

#define DIGIT_ON   LOW
#define DIGIT_OFF  HIGH

  for(int digit = 4 ; digit > 0 ; digit--) {

    //Turn on a digit for a short amount of time
    switch(digit) {
    case 1:
      digitalWrite(digit1, DIGIT_ON);
      digitalWrite(colons, LOW);
      break;
    case 2:
      digitalWrite(digit2, DIGIT_ON);
      digitalWrite(colons, LOW);
      break;
    case 3:
      digitalWrite(digit3, DIGIT_ON);
      if(displayColon == TRUE) 
        digitalWrite(colons, HIGH); //When we update digit 2, let's turn on colons as well
      else
        digitalWrite(colons, LOW);
      break;
    case 4:
      digitalWrite(digit4, DIGIT_ON);
      digitalWrite(colons, LOW);
      break;
    }

    //Now display this digit
    if (digit != 4)
    {
      if( (toDisplay/10 != 0) || (toDisplay % 10) != 0) // do not display leading zeros
        lightNumber(toDisplay % 10); //Turn on the right segments for this digit

      toDisplay /= 10;
    }
    else
    {
      lightNumber('º');
    }

    delayMicroseconds(2000); //Display this digit for a fraction of a second (between 1us and 5000us, 500-2000 is pretty good)
    //If you set this too long, the display will start to flicker. Set it to 25000 for some fun.

    //Turn off all segments
    lightNumber(10);

    //Turn off all digits
    digitalWrite(digit1, DIGIT_OFF);
    digitalWrite(digit2, DIGIT_OFF);
    digitalWrite(digit3, DIGIT_OFF);
    digitalWrite(digit4, DIGIT_OFF);
    digitalWrite(colons, DIGIT_OFF);
  }

}
//Given 1022, we display "10.22"
//Each digit is displayed for ~2000us, and cycles through the 4 digits
//After running through the 4 numbers, the display is turned off
void displayTime(int toDisplay, boolean displayColon)
{

#define DIGIT_ON   LOW
#define DIGIT_OFF  HIGH

  for(int digit = 4 ; digit > 0 ; digit--) {

    //Turn on a digit for a short amount of time
    switch(digit) {
    case 1:
      digitalWrite(digit1, DIGIT_ON);
      digitalWrite(colons, LOW);
      break;
    case 2:
      digitalWrite(digit2, DIGIT_ON);
      if(displayColon == TRUE) 
        digitalWrite(colons, HIGH); //When we update digit 2, let's turn on colons as well
      else
        digitalWrite(colons, LOW);
      break;
    case 3:
      digitalWrite(digit3, DIGIT_ON);
      digitalWrite(colons, LOW);
      break;
    case 4:
      digitalWrite(digit4, DIGIT_ON);
      digitalWrite(colons, LOW);
      break;
    }

    //Now display this digit
    lightNumber(toDisplay % 10); //Turn on the right segments for this digit

    toDisplay /= 10;

    delayMicroseconds(2000); //Display this digit for a fraction of a second (between 1us and 5000us, 500-2000 is pretty good)
    //If you set this too long, the display will start to flicker. Set it to 25000 for some fun.

    //Turn off all segments
    lightNumber(10);

    //Turn off all digits
    digitalWrite(digit1, DIGIT_OFF);
    digitalWrite(digit2, DIGIT_OFF);
    digitalWrite(digit3, DIGIT_OFF);
    digitalWrite(digit4, DIGIT_OFF);
    digitalWrite(colons, DIGIT_OFF);
    //digitalWrite(ampm, DIGIT_OFF);
  }

}

//Takes a string like "gren" and displays it, left justified
//We don't use the colons, or AMPM dot, so they are turned off
void displayLetters(char *colorName)
{
#define DIGIT_ON  HIGH
#define DIGIT_OFF  LOW

  digitalWrite(digit4, DIGIT_OFF);
  digitalWrite(colons, DIGIT_OFF);
  //digitalWrite(ampm, DIGIT_OFF);

  for(int digit = 0 ; digit < 4 ; digit++) {
    //Turn on a digit for a short amount of time
    switch(digit) {
    case 0:
      digitalWrite(digit1, DIGIT_ON);
      break;
    case 1:
      digitalWrite(digit2, DIGIT_ON);
      break;
    case 2:
      digitalWrite(digit3, DIGIT_ON);
      break;
    case 3:
      digitalWrite(digit4, DIGIT_ON);
      break;
    }

    //Now display this letter
    lightNumber(colorName[digit]); //Turn on the right segments for this letter

    delayMicroseconds(2000); //Display this digit for a fraction of a second (between 1us and 5000us, 500-2000 is pretty good)
    //If you set this too long, the display will start to flicker. Set it to 25000 for some fun.

    //Turn off all segments
    lightNumber(10);

    //Turn off all digits
    digitalWrite(digit1, DIGIT_OFF);
    digitalWrite(digit2, DIGIT_OFF);
    digitalWrite(digit3, DIGIT_OFF);
  }
}

//Given a number, turns on those segments
//If number == 10, then turn off all segments
void lightNumber(int numberToDisplay)
{

#define SEGMENT_ON  HIGH
#define SEGMENT_OFF LOW

  /*
Segments
   -  A
   F / / B
   -  G
   E / / C
   -  D
   */

  switch (numberToDisplay)
  {

  case 0:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    break;

  case 1:
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    break;

  case 2:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 3:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 4:
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 5:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 6:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 7:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    break;

  case 8:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 9:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 10:
    digitalWrite(segA, SEGMENT_OFF);
    digitalWrite(segB, SEGMENT_OFF);
    digitalWrite(segC, SEGMENT_OFF);
    digitalWrite(segD, SEGMENT_OFF);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_OFF);
    break;

    /*
Segments
     -  A
     F / / B
     -    G
     E / / C
     - D
     */

    //Letters
  case 'b': //cdefg
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;
  case 'L': //def
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    break;
  case 'u': //cde
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    break;

  case 'g': //abcdfg
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;
  case 'r': //eg
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;
  case 'n': //ceg
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

    //case r
  case 'e': //adefg
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;
  case 'd': //bcdeg
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;
  case ' ': //None
    digitalWrite(segA, SEGMENT_OFF);
    digitalWrite(segB, SEGMENT_OFF);
    digitalWrite(segC, SEGMENT_OFF);
    digitalWrite(segD, SEGMENT_OFF);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_OFF);
    break;

  case 'y': //bcdfg
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;
    //case e 
    //case L
  case 'o': //cdeg
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  case 'º': // abfg
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    break;

  }
}

