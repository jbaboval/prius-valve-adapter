#define MOTOR_IN1 A2
#define MOTOR_IN2 A1
#define POS_IN    A0
#define REQ_IN    A3

#define POS_CURRENT 0
#define POS_OPEN 1
#define POS_CLOSED 2

typedef struct {
  int position;
  int plusTolerance;
  int minusTolerance;
} goal_t;

goal_t goal_positions[] = {
  { 512, 1024, 1024}, // Always matches current position
  { 80, 30, 80 },      // Valve is open to heater core
  { 950, 75, 0 },     // Valve is closed, bypass heater core
};

int goal;

void setup() {
  // put your setup code here, to run once:

  pinMode(POS_IN, INPUT);
  pinMode(REQ_IN, INPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
 
//  Serial.begin(9600);

  goal = POS_CURRENT;
}

void openValve() {
  goal = POS_OPEN;
}

void closeValve() {
  goal = POS_CLOSED;
}

void stopValve() {
  goal = POS_CURRENT;
}

void setSpeed(int pwm) {
  // Negative direction is on MOTOR_IN2, Positive direction is on MOTOR_IN1
  if (pwm == 0) {
      digitalWrite(MOTOR_IN1, LOW);
      digitalWrite(MOTOR_IN2, LOW);
  } else if (pwm < 0) {
      digitalWrite(MOTOR_IN1, LOW);
      analogWrite(MOTOR_IN2, abs(pwm));
  } else {
      digitalWrite(MOTOR_IN2, LOW);
      analogWrite(MOTOR_IN1, pwm);
  }
}


int distanceFromGoal(int position, goal_t goalPos) {

  if (position == goalPos.position) {
    return 0;
  }
  
  if (position > (goalPos.position - goalPos.minusTolerance)) {
    if (position <= (goalPos.position + goalPos.plusTolerance)) {
      return 0;
    }

    return goalPos.position - position;
  }

  if (position >= (goalPos.position - goalPos.minusTolerance)) {
    return 0;
  }

  return goalPos.position - position;
}

void loop() {
  static int position = 0;
  static int currentPWM = 0;
  static int targetPWM = 0;
  static int accel = 25;
  static int tick = 0;
  static int request = 0;
  static int previous_req = 0;
  static unsigned long millis_powered = millis();

  unsigned long current_millis = millis();

  // put your main code here, to run repeatedly:
  position = analogRead(A0);
  request = digitalRead(A3);

  if (request != previous_req) {
    // Protect against stuck. Remember how long we've been trying 
    // to obtain the requested position. Stop after a while. 
    // Retry when the request changes.
    millis_powered = current_millis;
  }

  goal = request ? POS_OPEN : POS_CLOSED;

  goal_t goalPos = goal_positions[goal];

  int distance = distanceFromGoal(position, goalPos);

  if (distance > 0) {
    targetPWM = 255;
  } else if (distance < 0) {
    targetPWM = -255;
  } else {
    targetPWM = 0;
  }

  if (targetPWM < currentPWM) {
    if ((currentPWM - targetPWM) < accel) {
      currentPWM = targetPWM;
    } else {
      currentPWM -= accel;
    }
  } else if (targetPWM > currentPWM) {
    if ((targetPWM - currentPWM) < accel) {
      currentPWM = targetPWM;
    }
    currentPWM += accel;
  }

  if (currentPWM == 0 || (millis_powered > current_millis)) {
    millis_powered = current_millis;
  }

  if ((current_millis - millis_powered) < 30000) { // Don't let the motor be on for more than 30s
    setSpeed(currentPWM);
  } else {
    setSpeed(0);
  }

//  setSpeed(currentPWM);

  delay(10);
}
