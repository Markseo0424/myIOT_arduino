#include <avr/wdt.h>
#include <Servo.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7,8);
const byte address_master_to_slave[6] = "12345";
const byte address_slave_to_master[6] = "54321";

#define AC_SWITCH_ID "11121"
#define AC_TEMP_ID "11122"
#define ALARM_ID "11123"

#include <Arduino.h>

//#include "PinDefinitionsAndMore.h" //Define macros for input and output pin etc.
#include <IRremote.hpp>

// On the Zero and others we switch explicitly to SerialUSB
#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif

#define OFF 0
#define ON 1
#define SET 2

int temp = 24;
bool isACOn = false;
bool isAlarmOn = false;
int wind = 2;
unsigned long long alarmTimer = 0;

boolean default_bit[8] = {1,0,0,0,1,0,0,0};
boolean off_bit[20] = {1,1,0,0,
                      0,0,0,0,
                      0,0,0,0,
                      0,1,0,1,
                      0,0,0,1};

boolean* makeData(int mode, int temp, int wind){
  boolean* dat = new boolean[28];
  for(int i = 0; i < 8;i++){
    dat[i] = default_bit[i];
  }

  int sum = 0;

  if(mode == 0)  {//꺼짐일 때
    for(int i = 0; i < 20; i++){
      dat[i+8] = off_bit[i];
    }
    return dat;
  }
  
  for(int i = 0; i < 4; i++){
    dat[i+8] = 0;
  }
  
  if(mode == 1)   //켜짐 
  for(int i = 0; i < 4; i++){
    dat[i+12] = 0;
  }
  
  if(mode == 2) {   //온도설정 
    for(int i = 0; i < 4; i++){
      dat[i+12] = (bool) (int(powInt(2,3 - i)) & 8);
    }
    sum += 8;
  }

  for(int i = 0; i < 4; i++){
    dat[i+16] = (bool) (int(powInt(2,3 - i)) & (temp - 15));
  }
  sum += temp - 15;

  if(wind == 0){
    for(int i = 0; i < 4; i++){
      dat[i+20] = (bool) (int(powInt(2,3 - i)) & 0);
    }
  }
  if(wind == 1) {
    for(int i = 0; i < 4; i++){
      dat[i+20] = (bool) (int(powInt(2,3 - i)) & 2);
    }
    sum += 2;
  }
  
  if(wind == 2) {
    for(int i = 0; i < 4; i++){
      dat[i+20] = (bool) (int(powInt(2,3 - i)) & 4);
    }
    sum += 4;
  }

  for(int i = 0; i < 4; i++){
    dat[i+24] = (bool) (int(powInt(2,3 - i)) & sum);
  }
  return dat;
}


int onMicros = 500;
int offZeroMicros = 650;
int offOneMicros = 1600;

int startOn = 3300;
int startOff = 9800;

int offMicros[] = {offZeroMicros, offOneMicros};

unsigned int* makeRawData(boolean* data, int len) {
  unsigned int* rawData = new unsigned int[len];
  rawData[0] = startOn;
  rawData[1] = startOff;
  for(int i = 0; i < len; i+=2) {
    rawData[i + 2] = onMicros;
    rawData[i + 3] = offMicros[data[i / 2]];
  }
  rawData[len] = onMicros;
  return rawData;
}

void sendAC(int mode, int temp, int wind){
  boolean* data = makeData(mode,temp,wind);
  for(int i = 0; i < 28; i++) {
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println();
  unsigned int* rawData = makeRawData(data,59);
  IrSender.sendRaw(rawData, 59, 38); 
}

void sendACTemp(String requestId, String responseId) {
  radioPrint(responseId + " " + AC_TEMP_ID + " " + requestId + " OK slider " + String(temp));
}

void sendACSwitch(String requestId, String responseId) {
  radioPrint(responseId + " " + AC_SWITCH_ID + " " + requestId + " OK onoff " + (isACOn? "ON" : "OFF"));
}

void sendAlarm(String requestId, String responseId) {
  radioPrint(responseId + " " + ALARM_ID + " " + requestId + " OK onoff " + (isAlarmOn? "ON" : "OFF"));
}

void setup() {
  Serial.begin(115200);
  
  wdt_enable(WDTO_8S);
  radio.begin();
  radio.openReadingPipe(0,address_master_to_slave);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
  Serial.println("restart");
  radioPrint("RESET");

  pinMode(2,OUTPUT);
  digitalWrite(2,HIGH);
  
  IrSender.begin(3, ENABLE_LED_FEEDBACK);
}

void loop() {
  if(radio.available()) {
    char text[32] = "";
    radio.read(&text,sizeof(text));
    Serial.println(text);
    String responseId, id, requestId, val;
    if(getValues(responseId, id, requestId, val, text, sizeof(text)/sizeof(text[0]))) {
      if(id.equals(AC_TEMP_ID)) {
        if(requestId.equals("VAL") || requestId.equals("SET")) {
          int valInt = val.toInt();
          if(valInt <= 30 && valInt >= 18) {
            temp = valInt;
            if(isACOn) sendAC(SET, temp, wind);
          }
        }
        sendACTemp(requestId,responseId);
      }
      else if(id.equals(AC_SWITCH_ID)) {
        if(requestId.equals("VAL") || requestId.equals("SET")) {
          isACOn = val.equals("ON");
          if(isACOn) sendAC(ON,temp,wind);
          else sendAC(OFF,temp,wind);
        }
        sendACSwitch(requestId,responseId);
      }
      else if(id.equals(ALARM_ID)) {
        if(requestId.equals("VAL") || requestId.equals("SET")) {
          isAlarmOn = val.equals("ON");
        }
        sendAlarm(requestId,responseId);
      }
    }
  }
  wdt_reset();
  alarm();
  delay(500);
}


void alarm() {
  if(millis() - alarmTimer > 1000) {
    alarmTimer = millis();
    if(isAlarmOn) tone(6,700,500);
  }
}

int powInt(int under, int upper){
  int res = 1;
  for(int i = 0; i < upper; i++){
    res *= under;
  }
  return res;
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
