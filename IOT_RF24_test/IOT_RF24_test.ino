#include <avr/wdt.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7,8);
const byte address_master_to_slave[6] = "12345";
const byte address_slave_to_master[6] = "54321";

#define SERVOID "1111"
#define LEDID "2222"
#define POTID "3333"

#define SERVOPIN 10
#define LEDPIN 9
#define POTPIN A0

#define SERVO_MIN 0
#define SERVO_MAX 180

Servo servo;
int servoPos = 0;
boolean ledState = true;
unsigned long lastMillis = 0;
int lastPotVal = 0;
boolean servoAttached = false;


void sendServo(String requestId, String responseId) {
  radioPrint(responseId + " " + SERVOID + " " + requestId + " OK slider " + String(map(servoPos,0,180,0,100)));
}

void sendLed(String requestId, String responseId) {
  radioPrint(responseId + " " + LEDID + " " + requestId + " OK onoff " + (ledState? "ON" : "OFF"));
}

void sendPot(String requestId, String responseId, int val) {
  radioPrint(responseId + " " + POTID + " " + requestId + " OK value " + String(val));
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
  wdt_enable(WDTO_4S);
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0,address_master_to_slave);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
  Serial.println("restart");
  radioPrint("RESET");
  pinMode(LEDPIN,OUTPUT);
}

void loop() {
  if(radio.available()) {
    char text[32] = "";
    radio.read(&text,sizeof(text));
    Serial.println(text);
    String responseId, id, requestId, val;
    if(getValues(responseId, id, requestId, val, text, sizeof(text)/sizeof(text[0]))) {
      if(id.equals(SERVOID)) {
        if(requestId.equals("VAL") || requestId.equals("SET")) {
          int valInt = val.toInt();
          servoPos = map(valInt,0,100,SERVO_MIN,SERVO_MAX);
          if(!servoAttached) {
            servo.attach(SERVOPIN);
            servoAttached = true;
          }
          servo.write(servoPos);
        }
        sendServo(requestId,responseId);
      }
      else if(id.equals(LEDID)){
        if(requestId.equals("VAL") || requestId.equals("SET")) ledState = val.equals("ON");
        digitalWrite(LEDPIN, ledState);
        sendLed(requestId,responseId);
      }
      else if(id.equals(POTID)) {
        sendPot(requestId,responseId,analogRead(POTPIN));
      }
      //upDate();
    }
  }
  sendPotInterval(1000);
  wdt_reset();
  delay(500);
  //Serial.println("Alive");
}

boolean getValues(String &responseId, String &id, String &requestId, String &val, char* text, int len) {
  boolean isOk = false;
  boolean isEnd = false;
  int count = 0;
  for(int i = 0; i < len; i++){
    if(text[i] == '|') isOk = true;
    if(isOk && text[i] == ' ') count++;
    if(text[i] == ';') {
      isEnd = true;
      break;
    }
  }
  if(!(isEnd && count == 2)) return false;

  
  int n = 0;
  int index = 0;
  String full = text;
  for(int i = 0; i < len; i++){
    if(text[i] == '|') {
      n = 1;
      responseId = full.substring(index,i);
      index = i + 1;
    }
    if(text[i] == ' ') {
      String data = full.substring(index,i);
      switch(n) {
        case 1: id = data;break;
        case 2: requestId = data;break;
      }
      n++;
      index = i + 1;
    }
    if(text[i] == ';'){
      String data = full.substring(index,i);
      val  = data;
      break;
    }
  }
  return true;
}

void radioPrint(String str) {
  radio.openWritingPipe(address_slave_to_master);
    radio.setPALevel(RF24_PA_LOW);
    radio.stopListening();
    int len = str.length();
    char* texts = new char[len];
    strcpy(texts,str.c_str());
    radio.write(texts, len);
    radio.openReadingPipe(0,address_master_to_slave);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
}
