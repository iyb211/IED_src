#include <Servo.h>

// Arduino pin assignment
#define PIN_IR A0
#define PIN_LED 9
#define PIN_SERVO 10

#define _DUTY_MIN 553
#define _DUTY_NEU 1476
#define _DUTY_MAX 2399

#define _D 1350
#define _E 1900
#define _F 950

#define _SERVO_SPEED 30
#define INTERVAL 20  // servo update interval
#define _DIST_ALPHA 0.3

int a, b; // unit: mm
Servo myservo;
float dist_ema, alpha;
unsigned long last_sampling_time; // unit: ms
int duty_chg_per_interval;
int duty_target, duty_curr;

void setup() {
  myservo.attach(PIN_SERVO);
  duty_curr = _D;
  myservo.writeMicroseconds(duty_curr);

// initialize serial port
  Serial.begin(57600);

  duty_chg_per_interval = (_DUTY_MAX - _DUTY_MIN) * ((float)_SERVO_SPEED / 180) * ((float)INTERVAL / 1000);


  a = 67.5; //70;
  b = 298; //300;

  last_sampling_time = 0;
  alpha = _DIST_ALPHA;
}

float ir_distance(void){ // return value unit: mm
  float val;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
  return val;
}

void loop() {
  if(millis() < last_sampling_time + INTERVAL) return;

  float raw_dist = ir_distance();
  float dist_cali = 100 + 300.0 / (b - a) * (raw_dist - a);
  dist_ema = alpha * dist_cali + (1-alpha) * dist_ema;

  Serial.print("min:0,max:500,dist:");
  Serial.print(raw_dist);
  Serial.print(",dist_ema:");
  Serial.println(dist_ema);

  if (dist_ema > 255) {
    duty_target = _F;
  }
  else {
    duty_target = _E;
  }

  if(duty_target > duty_curr) {
    duty_curr += duty_chg_per_interval;
    if(duty_curr > duty_target) duty_curr = duty_target;
  }
  else {
    duty_curr -= duty_chg_per_interval;
    if(duty_curr < duty_target) duty_curr = duty_target;
  }

   myservo.writeMicroseconds(duty_curr);
  
  last_sampling_time += INTERVAL;
}
