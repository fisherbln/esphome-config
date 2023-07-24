// Vincent Fix 2023/5/24
// Need Choice Sensor Type !
// mqtt/wifi setup from here: https://docs.arduino.cc/tutorials/uno-wifi-rev2/uno-wifi-r2-mqtt-device-to-device

#define SENSOR_5_PIN

#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>
#include <ArduinoMqttClient.h>
//#include <WiFiNINA.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "pin_config.h"
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
char mqtt_server[] = SECRET_MQTT_SERVER;    // your mqtt address
char mqtt_user[] = SECRET_MQTT_USER;    // your mqtt user
char mqtt_pass[] = SECRET_MQTT_PASS;    // your mqtt password

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = SECRET_MQTT_SERVER;
int        port     = 1883;
const char topicP[]  = "soil/front_yard/P";
const char topicN []  = "soil/front_yard/N";
const char topicK[]  = "soil/front_yard/K";
const char topicH[]  = "soil/front_yard/H";
const char topicT[]  = "soil/front_yard/T";
const char topicPH[]  = "soil/front_yard/Ph";

//set interval for sending messages (milliseconds)
//const long interval = 8000;
//unsigned long previousMillis = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
HardwareSerial MySerial(1);

unsigned char resp[80] = {0};

int humidity = 0;
int tem = 0;
int ph = 0;
float humidity_value = 0.0;
float tem_value = 0.0;
float ph_value = 0.0;
int P_value = 0;
int N_value = 0;
int K_value = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(4800);
  MySerial.begin(4800, SERIAL_8N1, 23, 22);

  pinMode(Display_power, OUTPUT);
  pinMode(RS485_power, OUTPUT);
  delay(50);
  digitalWrite(Display_power, HIGH);
  digitalWrite(RS485_power, HIGH);
  delay(1000);

  Serial.println(" Test Begin!");

  Wire.begin(ESP32_SDA, ESP32_SCL);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println("SSD1306 found");
  
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  mqttClient.setId("clientid");
  mqttClient.setUsernamePassword(mqtt_user, mqtt_pass);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}


void loop()
{
  Serial.println("Send loop");

#ifdef SENSOR_5_PIN
  sensor_read_5pin();
  value_log();
  value_show_5pin(humidity_value, tem_value, ph_value);
  delay(3000);
  NPK_Show(N_value, P_value, K_value);
  delay(3000);
  mqtt_publish_5pin(N_value, P_value, K_value, humidity_value, tem_value, ph_value);
#endif
}



void sensor_read_5pin()
{
  unsigned char ask_cmd[8] = {0X01, 0X04, 0X00, 0X00, 0X00, 0X07, 0XB1, 0XC8};
  MySerial.write(ask_cmd, 8);
  int i = 0;

  while (MySerial.available() > 0 && i < 80)
  {
    resp[i] = MySerial.read();
    i++;

    yield();
  }

  Serial.print("Answer Length:");
  Serial.println(i);

  char temp[20];
  for (int j = 0; j < 19; j++)
  {
    sprintf(temp, "%02X ", (int)resp[j]);
    Serial.printf(temp);
  }

  Serial.print("\n");

  humidity = CaculateValue((int)resp[3], (int)resp[4]);
  humidity_value = humidity * 0.1;
  tem = CaculateValue((int)resp[5], (int)resp[6]);
  tem_value = tem * 0.1;
  ph = CaculateValue((int)resp[9], (int)resp[10]);
  ph_value = ph * 0.1;
  N_value = CaculateValue((int)resp[11], (int)resp[12]);
  P_value = CaculateValue((int)resp[13], (int)resp[14]);
  K_value = CaculateValue((int)resp[15], (int)resp[16]);
}

int CaculateValue(int x, int y)
{
  int t = 0;
  t = x * 256;
  t = t + y;
  return t;
}

void value_show_5pin(float h, float t, float ph_f)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); // Draw 2X-scale text
  display.setCursor(2, 0);
  display.print(F("T:"));
  display.print(t, 1);
  display.print(F("C"));

  display.setCursor(66, 0);
  display.print(F("H:"));
  display.print(h, 1);
  display.print(F("%"));

  display.setCursor(2, 16);
  display.print(F("PH:"));
  display.print(ph_f, 1);

  display.display(); // Show initial text
}

void NPK_Show(int N, int P, int K)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); // Draw 2X-scale text
  display.setCursor(2, 0);
  display.print(F("N:"));
  display.print(N);
  // display.print(F("C"));

  display.setCursor(66, 0);
  display.print(F("P:"));
  display.print(P);
  // display.print(F("%"));

  display.setCursor(2, 16);
  display.print(F("K:"));
  display.print(K);
  display.print(F(" mg/kg"));

  display.display(); // Show initial text
}

void mqtt_publish_5pin(int N, int P, int K, float h, float t, float ph_f)
{
  // call poll() regularly to allow the library to send MQTT keep alive which
  // avoids being disconnected by the broker
  mqttClient.poll();

  //unsigned long currentMillis = millis();

  //if (currentMillis - previousMillis >= interval) {
  //  // save the last time a message was sent
  //  previousMillis = currentMillis;

  Serial.print("Sending message to topic: ");
  Serial.println(topicP);
  Serial.println(P);
  Serial.println(N);
  Serial.println(K);
  Serial.println(h,1);
  Serial.println(t,1);
  Serial.println(ph_f,1);

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage(topicP);
  mqttClient.print(P);
  mqttClient.endMessage();

  mqttClient.beginMessage(topicN);
  mqttClient.print(N);
  mqttClient.endMessage();

  mqttClient.beginMessage(topicK);
  mqttClient.print(K);
  mqttClient.endMessage();

  mqttClient.beginMessage(topicH);
  mqttClient.print(h);
  mqttClient.endMessage();

  mqttClient.beginMessage(topicT);
  mqttClient.print(t);
  mqttClient.endMessage();

  mqttClient.beginMessage(topicPH);
  mqttClient.print(ph_f);
  mqttClient.endMessage();

  //}
}

void value_log()
{
  Serial.print("humidity:");
  Serial.println(humidity);
  Serial.print("humidity_value:");
  Serial.println(humidity_value);
  Serial.print("tem_value:");
  Serial.println(tem_value);

#ifdef SENSOR_5_PIN
  Serial.print("ph_value:");
  Serial.println(ph_value);

  Serial.print("N= ");
  Serial.print(N_value);
  Serial.println(" mg/kg");
  Serial.print("P= ");
  Serial.print(P_value);
  Serial.println(" mg/kg");
  Serial.print("K= ");
  Serial.print(K_value);
  Serial.println(" mg/kg");

#endif
}