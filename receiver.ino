#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

#define LED 13
#define RF95_FREQ 434.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  Serial.begin(115200); // Увеличиваем скорость для Orange Pi
  delay(100);

  // Ручной сброс
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
  Serial.println("Receiver ready");
}

void loop()
{
  if (rf95.available())
  {
    // Должен быть сообщение
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      
      // Преобразуем полученные данные в строку
      String receivedData = (char*)buf;
      
      // Выводим сырые данные в Serial (для Orange Pi)
      Serial.println(receivedData);

      // Также выводим в монитор порта для отладки
      Serial.print("Received: ");
      Serial.print(receivedData);
      Serial.print(" with RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      
      digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}