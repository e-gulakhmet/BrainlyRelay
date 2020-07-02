#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#include "main.h"

WiFiClient espClient;
PubSubClient client(espClient);

uint8_t id = EEPROM.read(0);

const String topics[] = {
  "ralay/" + String(id) + "/id",
  "relay/" + String(id) + "/tx",
  "relay/" + String(id) + "/rx"
};

bool is_on;


void callBack(char* topic, byte* payload, unsigned int length) { // Функция в которой обрабатываются все присланные команды
  static unsigned long timer;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");


  String strTopic = String(topic); // Строка равная топику
  String strPayload = ""; // Создаем пустую строку, которая будет хранить весь payload


  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    strPayload = strPayload + ((char)payload[i]); // присваеваем значение по столбикам, чтобы получить всю строку
  }
  Serial.println();


  if (strTopic == topics[1]){ // Полив по команде
    is_on = strPayload == "on" ? true : false;
  }

  // Отправляем id модуля раз в минуту
  if (millis() - timer > 30*1000) {
    client.publish(topics[0].c_str(), String(id).c_str());
    timer = millis();
  }


  if (strTopic == topics[1]) { // Если прилетела команда статус,
    // отправляем состояние реле
    if (strPayload == "status") {
      client.publish(topics[2].c_str(), is_on ? "on" : "off");
    }
  }
}


void connect() {
  client.setServer(mqtt_server, 1883);
  delay(1000);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(name)) {
      for (int i = 0; i < 4; i++) {
        client.subscribe(topics[i].c_str());
      }
      client.setCallback(callBack);
      Serial.println("connected");
      Serial.println('\n');
    }
    else {
      Serial.println(" try again in 3 seconds");
      delay(3000);
    }
  }
}



void initWifi(){ // Настройка WIFI
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}



void initWifiUpd() {
  ArduinoOTA.setHostname(("RELAY" + String(id)).c_str()); // Задаем имя сетевого порта
  //ArduinoOTA.setPassword((const char *)"0000"); // Задаем пароль доступа для удаленной прошивки
  ArduinoOTA.begin(); // Инициализируем OTA
}



void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  
  initWifi();
  delay(100);
  connect();
  
  initWifiUpd();

  // Задаем рандомное id модулю и сохраняем его,
  // чтобы затем была возможность управлять сразу несколькими модулями
  if (id == 255 || id == 0) {
    id = random(100, 200);
    EEPROM.write(0, id);
  }
}



void loop() {
  if (!client.connected()) // Если потеряли подключение
    connect(); // Переподключаемся
  
  client.loop();

  ArduinoOTA.handle(); // Всегда готовы к прошивке

  digitalWrite(RELAY_PIN, is_on ? LOW : HIGH);
}