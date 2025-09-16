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
// Начальное пороговое значение (будет переопределено при калибровке)
#define DEFAULT_THRESHOLD 20
// Время блокировки после срабатывания (мс)
#define DEBOUNCE_TIME 30
// Запас над максимальным значением (%)
#define THRESHOLD_MARGIN 10

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// ID для отправки
const char* ID = "1";
int count = 0; // Переменная для отслеживания счётчика
int threshold = DEFAULT_THRESHOLD; // Переменная для хранения порогового значения
int maxValue = 0; // Максимальное зафиксированное значение
unsigned long lastTriggerTime = 0; // Время последнего срабатывания
bool isTriggered = false; // Флаг срабатывания

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(PIEZO_PIN, INPUT);
  pinMode(LED, OUTPUT);

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
  
  // Калибровка датчика
  calibrateSensor();
  
  Serial.println("START");
}

// Функция калибровки датчика
void calibrateSensor() {
  Serial.println("Starting calibration... Tap the sensor several times in 10 seconds");
  digitalWrite(LED, HIGH); // Включить светодиод на время калибровки
  
  maxValue = 0; // Сбрасываем максимальное значение
  unsigned long startTime = millis();
  
  // Сбор данных в течение 10 секунд
  while (millis() - startTime < 10000) {
    int value = analogRead(PIEZO_PIN);
    
    // Обновляем максимальное значение
    if (value > maxValue) {
      maxValue = value;
      Serial.print("New max: ");
      Serial.println(maxValue);
    }
    
    // Мигание светодиодом для индикации процесса калибровки
    digitalWrite(LED, (millis() / 500) % 2 == 0);
    
    delay(10); // Частое чтение для捕捉 максимальных значений
  }
  
  // Установка порога на 10% выше максимального значения
  threshold = maxValue * (100 + THRESHOLD_MARGIN) / 100;
  
  // Минимальный порог на случай, если максимальное значение слишком мало
  if (threshold < 50) {
    threshold = 50;
  }
  
  digitalWrite(LED, LOW); // Выключить светодиод
  
  Serial.print("Calibration complete. Max value: ");
  Serial.print(maxValue);
  Serial.print(", Threshold: ");
  Serial.println(threshold);
}

void loop()
{
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.recv(buf, &len)) {
      String received = (char*)buf;
      received.trim();
      
      // Если получена команда reset
      if (received.equals("reset")) {
        count = 0;
        Serial.println("Counter reset via radio command");
      }
      // Если получена команда recalibrate
      else if (received.equals("recalibrate")) {
        calibrateSensor();
      }
      // Если получена команда status
      else if (received.equals("status")) {
        char statusPacket[50];
        snprintf(statusPacket, sizeof(statusPacket), "STATUS:%s:%d:%d:%d", ID, count, threshold, maxValue);
        rf95.send((uint8_t *)statusPacket, strlen(statusPacket) + 1);
        rf95.waitPacketSent();
      }
    }
  }
  
  // Проверка наличия доступных данных в последовательном порте
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Чтение команды до новой строки
    command.trim(); // Удаление лишних пробелов
    
    if (command.equals("reset")) {
      count = 0; // Сброс счетчика
      Serial.println("Count reset to 0.");
    }
    else if (command.equals("recalibrate")) {
      calibrateSensor(); // Повторная калибровка
    }
    else if (command.equals("threshold")) {
      Serial.print("Current threshold: ");
      Serial.println(threshold);
      Serial.print("Max value: ");
      Serial.println(maxValue);
    }
    else if (command.equals("status")) {
      Serial.print("ID:");
      Serial.print(ID);
      Serial.print(" Count:");
      Serial.print(count);
      Serial.print(" Threshold:");
      Serial.print(threshold);
      Serial.print(" Max:");
      Serial.println(maxValue);
    }
  }

  // Чтение значения с пьезодатчика
  int piezoValue = analogRead(PIEZO_PIN);
  
  // Получаем текущее время
  unsigned long currentTime = millis();
  
  // Отладочный вывод (можно закомментировать)
  // Serial.println(piezoValue);
  
  // Проверяем, превышено ли пороговое значение и прошло ли достаточно времени с последнего срабатывания
  if (piezoValue > threshold && !isTriggered) {
    // Устанавливаем флаг срабатывания и запоминаем время
    isTriggered = true;
    lastTriggerTime = currentTime;
    
    // Увеличение счётчика
    count++;

    digitalWrite(LED, HIGH);
    Serial.print("Piezo value: ");
    Serial.print(piezoValue);
    Serial.print(" (Threshold: ");
    Serial.print(threshold);
    Serial.print(") - Send: ");
    
    // Форматирование сообщения (добавим значение датчика)
    char radiopacket[30];
    snprintf(radiopacket, sizeof(radiopacket), "%s:%d:%d", ID, count, piezoValue); // Формат ID:счётчик:значение
    Serial.println(radiopacket);
    
    // Отправка сообщения
    rf95.send((uint8_t *)radiopacket, strlen(radiopacket) + 1);
    rf95.waitPacketSent();
    
    digitalWrite(LED, LOW);
    
    // Обновляем максимальное значение, если текущее больше
    if (piezoValue > maxValue) {
      maxValue = piezoValue;
      Serial.print("New maximum value recorded: ");
      Serial.println(maxValue);
    }
  }
  
  // Сбрасываем флаг срабатывания, если прошло достаточно времени
  if (isTriggered && (currentTime - lastTriggerTime >= DEBOUNCE_TIME)) {
    isTriggered = false;
  }
  
  // Небольшая задержка между измерениями
  delay(10);
}
