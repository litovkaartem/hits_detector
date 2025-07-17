#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define LED 13
#define RF95_FREQ 434.0  

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

unsigned long lastResetTime = 0;
const unsigned long resetInterval = 30000; // 30 секунд в миллисекундах

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(LED, OUTPUT);
  
  Serial.begin(9600);
  delay(100);

  // manual reset
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
  lastResetTime = millis(); // Инициализация таймера
}

void loop()
{
  // Проверка необходимости сброса счетчика
  if (millis() - lastResetTime >= resetInterval) {
    sendResetCommand();
    lastResetTime = millis();
  }
  
  // Проверка входящих сообщений
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len)) {
      digitalWrite(LED, HIGH);
      
      // Вывод полученного сообщения
      Serial.print("Received: ");
      Serial.println((char*)buf);
      
      // Здесь можно добавить обработку полученных данных
      
      digitalWrite(LED, LOW);
    }
  }
  
  delay(10); // Небольшая задержка для стабильности
}

void sendResetCommand() {
  char command[] = "reset";
  
  Serial.print("Sending reset command: ");
  Serial.println(command);
  
  rf95.send((uint8_t *)command, strlen(command) + 1);
  rf95.waitPacketSent();
  
  // Мигание LED для индикации отправки
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
}
