#include <Servo.h>

/////////////////////////////
// Configurable parameters //
/////////////////////////////

// Arduino pin assignment
#define PIN_LED 9
#define PIN_SERVO 10
#define PIN_IR A0

// Framework setting
#define _DIST_TARGET 255
#define _DIST_MIN 100
#define _DIST_MAX 410

// Distance sensor
#define _DIST_ALPHA 0.5

// Servo range
#define _DUTY_MIN 950
#define _DUTY_NEU 1400
#define _DUTY_MAX 1900

// Servo speed control
#define _SERVO_ANGLE 30
#define _SERVO_SPEED 400

// Event periods
#define _INTERVAL_DIST 20
#define _INTERVAL_SERVO 20
#define _INTERVAL_SERIAL 100

// PID parameters
#define _KP 3
#define _KI 0.01
#define _KD 120

#define _ITERM_MAX 10

//////////////////////
// global variables //
//////////////////////

// Servo instance
Servo myservo;

// Distance sensor
float dist_target;
float dist_raw, dist_ema, dist_cali;
float alpha;
int a, b;

// Event periods
unsigned long last_sampling_time_dist, last_sampling_time_servo,
last_sampling_time_serial;

bool event_dist, event_servo, event_serial;

// Servo speed control
int duty_chg_per_interval;
int duty_target; 
int duty_curr;

// PID variables
float error_curr, error_prev, control, pterm, dterm, iterm;


void setup() {
// initialize GPIO pins for LED and attach servo 
  pinMode(PIN_LED, OUTPUT);
  myservo.attach(PIN_SERVO);

// initialize global variables
  duty_target, duty_curr = _DUTY_NEU;
  last_sampling_time_dist, last_sampling_time_servo, last_sampling_time_serial = 0;
  dist_ema = 0;
  pterm = iterm = dterm = 0;
  error_prev = 0;
  
  alpha = _DIST_ALPHA;
  
  a = 67.5; //70;
  b = 298; //300;


// move servo to neutral position
  myservo.writeMicroseconds(_DUTY_NEU);

// initialize serial port
  Serial.begin(57600);

// convert angle speed into duty change per interval.
  duty_chg_per_interval = (_DUTY_MAX - _DUTY_MIN) * (_SERVO_SPEED / 180.0f) * (_INTERVAL_SERVO / 1000.0f);
}
  

void loop() {
    /////////////////////
    // Event generator //
    /////////////////////
    unsigned long time_curr = millis();
    if(time_curr >= last_sampling_time_dist + _INTERVAL_DIST) {
        last_sampling_time_dist += _INTERVAL_DIST;
        event_dist = true;
    }
    if(time_curr >= last_sampling_time_servo + _INTERVAL_SERVO) {
        last_sampling_time_servo += _INTERVAL_SERVO;
        event_servo = true;
    }
    if(time_curr >= last_sampling_time_serial + _INTERVAL_SERIAL) {
        last_sampling_time_serial += _INTERVAL_SERIAL;
        event_serial = true;
    }

    ////////////////////
    // Event handlers //
    ////////////////////

    if(event_dist) {
        event_dist = false;
        // get a distance reading from the distance sensor
        dist_raw = ir_distance_filtered();
        dist_cali = 100 + 300.0 / (b - a) * (dist_raw - a);
        dist_ema = alpha * dist_cali + (1-alpha) * dist_ema;

        // PID control logic
        error_curr = _DIST_TARGET - dist_ema;
        pterm = _KP * error_curr;
        dterm = _KD * (error_curr - error_prev);
        iterm += _KI * error_curr;

        if(iterm > _ITERM_MAX) iterm = _ITERM_MAX;
        if(iterm < - _ITERM_MAX) iterm = - _ITERM_MAX;
        
        control = pterm + dterm + iterm;
        
        duty_target = _DUTY_NEU + control; 

  // keep duty_target value within the range of [_DUTY_MIN, _DUTY_MAX]
        if(duty_target < _DUTY_MIN)
        {
            duty_target = _DUTY_MIN;
        }
        if(duty_target > _DUTY_MAX)
        {
            duty_target = _DUTY_MAX;
        }

        error_prev = error_curr;
  
  
    if(event_servo) {
        event_servo = false;
        // adjust duty_curr toward duty_target by duty_chg_per_interval
         if(duty_target > duty_curr) {
           duty_curr += duty_chg_per_interval;
           if(duty_curr > duty_target) duty_curr = duty_target;
         }
         else {
           duty_curr -= duty_chg_per_interval;
           if(duty_curr < duty_target) duty_curr = duty_target;
         }
        // update servo position
        myservo.writeMicroseconds(duty_curr);
      }


    }
  
    if(event_serial) {
        event_serial = false;
        Serial.print("IR:");
        Serial.print(dist_ema);
        Serial.print(",T:");
        Serial.print(dist_target);
        Serial.print(",P:");
        Serial.print(map(pterm,-1000,1000,510,610));
        Serial.print(",D:");
        Serial.print(map(dterm,-1000,1000,510,610));
        Serial.print(",I:");
        Serial.print(map(iterm,-1000,1000,510,610));
        Serial.print(",DTT:");
        Serial.print(map(duty_target,1000,2000,410,510));
        Serial.print(",DTC:");
        Serial.print(map(duty_curr,1000,2000,410,510));
        Serial.println(",-G:245,+G:265,G:255,m:0,M:800");  
    }
}

float ir_distance(void){ // return value unit: mm
    float val;
    float volt = float(analogRead(PIN_IR));
    val = ((6762.0/(volt - 9.0)) - 4.0) * 10.0;
    return val;
}

float under_noise_filter(void){
  int currReading;
  int largestReading = 0;
  for (int i = 0; i < 3; i++) {
    currReading = ir_distance();
    if (currReading > largestReading) { largestReading = currReading; }
    delayMicroseconds(1500);
  }
  return largestReading;
}

float ir_distance_filtered(void){
  int currReading;
  int lowestReading = 500;
  for (int i = 0; i < 3; i++) {
    currReading = under_noise_filter();
    if (currReading < lowestReading) { lowestReading = currReading; }
  }
  return lowestReading;
}
