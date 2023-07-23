#include <avr/wdt.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7,8);
const byte address_master_to_slave[6] = "12345";
const byte address_slave_to_master[6] = "54321";

void setup() {
  wdt_enable(WDTO_4S);
  Serial.begin(9600);
  Serial.println("RESET");
  radio.begin();
  radio.openReadingPipe(0,address_slave_to_master);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

unsigned long lastMillis = 0;
unsigned long serialInterval = 1000;
boolean timerReset = true;


void loop() {
  if(radio.available()) {
    char text[32] = "";
    radio.read(&text,sizeof(text));
    String str = text;
    Serial.println(str);
  }
  if(Serial.available()) {
    if(millis() - lastMillis > serialInterval) {
      String s = Serial.readStringUntil(';') + ";";
      if(s.length() < 5) timerReset = false;
      radioPrint(s);
      lastMillis = millis();
    }
  }
  if(timerReset) wdt_reset();
  delay(500);
}

void radioPrint(String str) {
    radio.openWritingPipe(address_master_to_slave);
    radio.setPALevel(RF24_PA_LOW);
    radio.stopListening();
    delay(10);
    int len = str.length();
    char* texts = new char[len];
    strcpy(texts,str.c_str());
    radio.write(texts, len);
    radio.openReadingPipe(0,address_slave_to_master);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
}
