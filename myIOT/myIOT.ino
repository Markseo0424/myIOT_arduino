#include <Servo.h>
#define SERVOID "1111"
#define LEDID "2222"
#define POTID "3333"

#define SERVOPIN 10
#define LEDPIN 8
#define POTPIN A0

#define SERVO_MIN 0
#define SERVO_MAX 180

Servo servo;
int servoPos = 0;
boolean ledState = true;
unsigned long lastMillis = 0;
int lastPotVal = 0;

void sendServo(String requestId, String responseId) {
  Serial.print(responseId);
  Serial.print(" ");
  Serial.print(SERVOID);
  Serial.print(" ");
  Serial.print(requestId);
  Serial.print(" OK slider ");
  Serial.println(map(servoPos,0,180,0,100));
}

void sendLed(String requestId, String responseId) {
  Serial.print(responseId);
  Serial.print(" ");
  Serial.print(LEDID);
  Serial.print(" ");
  Serial.print(requestId);
  Serial.print(" OK onoff ");
  Serial.println(ledState? "ON" : "OFF");
}

void sendPot(String requestId, String responseId, int val) {
  Serial.print(responseId);
  Serial.print(" ");
  Serial.print(POTID);
  Serial.print(" ");
  Serial.print(requestId);
  Serial.print(" OK value ");
  Serial.println(val);
}

void sendPotInterval(int ms) {
  if(millis() - lastMillis >= ms) {
    int val = analogRead(POTPIN);
    if(lastPotVal != val) {
      sendPot("UPDATE", "0", val);
      lastPotVal = val;
    }
    lastMillis = millis();
  }
}

void upDate() {
  servo.write(servoPos);
  digitalWrite(LEDPIN, ledState);
}

void setup() {
  Serial.begin(9600);
  servo.attach(SERVOPIN);
  pinMode(LEDPIN,OUTPUT);
}

void loop() {
  if(Serial.available()) {
    String responseId = Serial.readStringUntil('|');
    String id = Serial.readStringUntil(' ');
    String requestId = Serial.readStringUntil(' ');
    String val = Serial.readStringUntil(';');
    if(id.equals(SERVOID)) {
      if(requestId.equals("VAL") || requestId.equals("SET")) {
        int valInt = val.toInt();
        servoPos = map(valInt,0,100,SERVO_MIN,SERVO_MAX);
      }
      sendServo(requestId,responseId);
    }
    else if(id.equals(LEDID)){
      if(requestId.equals("VAL") || requestId.equals("SET")) ledState = val.equals("ON");
      sendLed(requestId,responseId);
    }
    else if(id.equals(POTID)) {
      sendPot(requestId,responseId,analogRead(POTPIN));
    }
  }
  upDate();
  sendPotInterval(1000);
}
