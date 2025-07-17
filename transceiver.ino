#include <SPI.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Blinky on receipt
#define LED 13
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 434.0

// Пин для пьезоэлектрического датчика
#define PIEZO_PIN A0
// Пороговое значение для отправки
#define THRESHOLD 20

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// ID для отправки
const char* ID = "1";
int count = 0; // Переменная для отслеживания счётчика

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(PIEZO_PIN, INPUT);

  while (!Serial);
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
  Serial.println("START");
}

void loop()
{
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len)) {
      String received = (char*)buf;
      
      // Если получена команда reset
      if (received.equals("reset")) {
        count = 0;
        Serial.println("Counter reset via radio command");
      }
    }
  }
  // Проверка наличия доступных данных в последовательном порте
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Чтение команды до новой строки
    if (command.equals("reset")) {
      count = 0; // Сброс счетчика
      Serial.println("Count reset to 0.");
      return; // Прекращаем выполнение текущего цикла
    }
  }

  // Чтение значения с пьезодатчика
  int piezoValue = analogRead(PIEZO_PIN);
  
  // Проверка превышения порогового значения
  if (piezoValue > THRESHOLD) {
    // Увеличение счётчика только при превышении порога
    count++;

    digitalWrite(LED, HIGH);
    Serial.print("Piezo value: ");
    Serial.print(piezoValue);
    Serial.print(" - Send: ");
    
    // Форматирование сообщения (добавим значение датчика)
    char radiopacket[30];
    snprintf(radiopacket, sizeof(radiopacket), "%s:%d:%d", ID, count, piezoValue); // Формат ID:счётчик:значение
    Serial.println(radiopacket);
    
    // Отправка сообщения
    rf95.send((uint8_t *)radiopacket, strlen(radiopacket) + 1);
    rf95.waitPacketSent();
    
    digitalWrite(LED, LOW);
    
    // Короткая задержка после отправки
    delay(100);
  }
  
  // Небольшая задержка между измерениями
  delay(50);
}
  // Небольшая задержка между измерениями
  delay(50);
}
