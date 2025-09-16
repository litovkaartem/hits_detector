#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define LED 13
#define RF95_FREQ 434.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(LED, OUTPUT);
  
  Serial.begin(9600);
  delay(100);

  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  
  rf95.setTxPower(18);
  Serial.println("Receiver started. Waiting for messages...");
}

void loop() {
  // Проверка команды сброса из Serial (от OrangePI)
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command.equals("reset")) {
      sendResetCommand();
    }
  }
  
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len)) {
      digitalWrite(LED, HIGH);
      Serial.println((char*)buf);
      digitalWrite(LED, LOW);
    }
  }
  
  delay(10);
}

void sendResetCommand() {
  char command[] = "reset";
  rf95.send((uint8_t *)command, strlen(command) + 1);
  rf95.waitPacketSent();
  
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  
  Serial.println("Reset command sent");
}
