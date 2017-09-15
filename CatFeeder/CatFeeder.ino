/*
 * CatFeeder.ino
 * Nick Stanley
 * 
 * Button presses rotate a motor to allow cat food to be dispensed
 * Food is dispensed every 12 hours 
 */

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <ESP8266WiFi.h>
//Nick's WiFi credentials, Adafruit IO Key, etc
#include <NicksCreds.h>

/* Pins
 * Technically shield is intitialized in Adafruit's files
 * But good to know who's in use for future pin assignments
 */
#define PIN_MOTOR_SDA 4
#define PIN_MOTOR_SCL 5
#define PIN_BUTTON    12

/* Motor setup
 * Using Adafruit's motor driver on I2C interface  
 * Our stepper motor has 200 steps and is on the M1,M2 side
 * of the motor shield
 */
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
Adafruit_StepperMotor *motor = AFMS.getStepper(200,1);

/* Button setup
 * Press the button to feed and start 12 hour cycle
 * Active low button
 * Standard debouncing logic
 */
#define MS_BOUNCE       100       //100ms delay to let bounce settle
#define TWELVE_HOURS_MS 43200000  //12 hours * 60min/hour * 60sec/min * 1000ms/sec
int btn_currentState;
int btn_lastState;
unsigned long ms_lastBounce;
unsigned long ms_nextFeeding = TWELVE_HOURS_MS;
unsigned long ms_lastFeeding;
bool overflow = false;

/* FeedCharlie()
 * Rotate the motor halfway (we have a 200 step motor)
 * Update next time to be fed again in 12 hours
 * Watch for millis() overflow, to be handled in loop()
 */
void FeedCharlie()
{
  ms_lastFeeding = ms_nextFeeding;
  motor->step(100,FORWARD,DOUBLE);
  ms_nextFeeding = millis() + TWELVE_HOURS_MS;
  motor->release();
  overflow = (ms_lastFeeding > ms_nextFeeding);
}

/* setup()
 * Setup motor shield communication and set RPM
 * NOTE: no default feeding action; feeding doesn't
 * start until button is pressed
 */
void setup() 
{
  WiFi.begin(NICKS_SSID, NICKS_PASS);
  AFMS.begin();  
  motor->setSpeed(30);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  motor->release();
}

/* loop()
 * First, check if the button has been pressed.
 * Debounce it, and feed Charlie. Once fed, timer
 * is updated.
 * Second, check timer. If experired, feed and
 * update timer
 */
void loop() 
{
  /* Button Feeding */
  //Read button
  int btn_state = digitalRead(PIN_BUTTON);

  //Transistion state, button has been pressed (or depressed)
  if(btn_state != btn_lastState)
    ms_lastBounce = millis();
  //Don't check again until we've waited minimum time for bounce to settle
  if((millis() - ms_lastBounce) > MS_BOUNCE)
  {
    
    if(btn_state != btn_currentState)
    {
      btn_currentState = btn_state;
      if(btn_state == LOW)
        FeedCharlie();
    }
  }
  btn_lastState = btn_state;

  /* Timed Feeding */
  //If our next feeding happens over the millis() overflow,
  //wait until it loops back before feeding logic processing
  if(overflow)
    overflow = (millis() < ms_lastFeeding);
  if(millis() > ms_nextFeeding && !overflow)
    FeedCharlie();
}
