#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7,8);
const byte address_master_to_slave[6] = "12345";
const byte address_slave_to_master[6] = "54321";

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0,address_slave_to_master);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening();
}

void loop() {
  if(radio.available()) {
    char text[100] = "";
    radio.read(&text,sizeof(text));
    Serial.println(text);
  }
  if(Serial.available()) {
    String s = Serial.readString();
  }
}

void radioPrint(String str) {
  radio.openWritingPipe(address_master_to_slave);
    radio.setPALevel(RF24_PA_LOW);
    radio.stopListening();
    int len = str.length();
    char* texts = new char[len];
    strcpy(texts,str.c_str());
    radio.write(texts, len);
    radio.openReadingPipe(0,address_slave_to_master);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
}
