#include <avr/wdt.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7,8);
const byte address_master_to_slave[6] = "12345";
const byte address_slave_to_master[6] = "54321";

#define SERVO_1_ID "11111"
#define SERVO_2_ID "11112"
#define SERVO_3_ID "11113"

#define SERVO_1_PIN 3
#define SERVO_2_PIN 5
#define SERVO_3_PIN 6

Servo servo_1;
Servo servo_2;
Servo servo_3;

int servo_1_pos = 90;
int servo_2_pos = 90;
int servo_3_pos = 90;

int servo_first_pos = 60;
int servo_second_pos = 120;
int servo_pos_shift = 15;

boolean servo_1_state = false;
boolean servo_2_state = false;
boolean servo_3_state = false;

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
  //Serial.println(str);
  //return;
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

void moveServo(boolean onOff, int num) {
  if(onOff) {
    if(num == 1) {
      servo_1.write(servo_first_pos-15);
      delay(500);
      servo_1.write(90);
    }
    if(num == 2) {
      servo_2.write(servo_first_pos-servo_pos_shift);
      delay(500);
      servo_2.write(90);
    }
    if(num == 3){
      servo_3.write(servo_second_pos+servo_pos_shift);
      delay(500);
      servo_3.write(90);
    }
  }
  else {
    if(num == 1) {
      servo_1.write(servo_second_pos+5);
      delay(500);
      servo_1.write(90);
    }
    if(num == 2) {
      servo_2.write(servo_second_pos+servo_pos_shift);
      delay(500);
      servo_2.write(90);
    }
    if(num == 3){
      servo_3.write(servo_first_pos-servo_pos_shift);
      delay(500);
      servo_3.write(90);
    }
  }
}

void sendServo(String requestId, String responseId, int num){
  int state = false;
  String servo_id = "";
  
  if(num == 1) {
    state = servo_1_state;
    servo_id = SERVO_1_ID;
  }
  if(num == 2) {
    state = servo_2_state;
    servo_id = SERVO_2_ID;
  }
  if(num == 3) {
    state = servo_3_state;
    servo_id = SERVO_3_ID;
  }
  
  radioPrint(responseId + " " + servo_id + " " + requestId + " OK onoff " + (state? "ON" : "OFF"));
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

  servo_1.attach(SERVO_1_PIN);
  servo_2.attach(SERVO_2_PIN);
  servo_3.attach(SERVO_3_PIN);
  servo_1.write(90);
  servo_2.write(90);
  servo_3.write(90);
}

void loop() {
  if(radio.available()) {
    char text[32] = "";
    radio.read(&text,sizeof(text));
    Serial.println(text);
    String responseId, id, requestId, val;

    if(getValues(responseId, id, requestId, val, text, sizeof(text)/sizeof(text[0]))) {
      if(id.equals(SERVO_1_ID)) {
        if(requestId.equals("VAL")||requestId.equals("SET")){
          moveServo(servo_1_state = val.equals("ON"), 1);
        }
        /*
        if(requestId.equals("SET")) {
          servo_1_state = val.equals("ON");
          servo_1.write(90);
        }
        */
        sendServo(requestId,responseId,1);
      }
      else if(id.equals(SERVO_2_ID)) {
        if(requestId.equals("VAL")||requestId.equals("SET")){
          moveServo(servo_2_state = val.equals("ON"), 2);
        }
        /*
        if(requestId.equals("SET")) {
          servo_2_state = val.equals("ON");
          servo_2.write(90);
        }
        */
        sendServo(requestId,responseId,2);
      }
      else if(id.equals(SERVO_3_ID)) {
        if(requestId.equals("VAL")||requestId.equals("SET")){
          moveServo(servo_3_state = val.equals("ON"), 3);
        }
        /*
        if(requestId.equals("SET")) {
          servo_3_state = val.equals("ON");
          servo_3.write(90);
        }
        */
        sendServo(requestId,responseId,3);
      }
    }
    
  }
  wdt_reset();
  delay(500);
}
