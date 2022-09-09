#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

SoftwareSerial mySerial(D3, D2); // RX, TX

#define MAX485_DE  D11
#define MAX485_RE  D12
#define Slave_ID 253

ModbusMaster node;

const char* ssid = "mLab";   
const char* password = "311208211";

// MQTT configuration
const char* server = "203.64.131.98";
const char* ph_topic = "greenhouse/wemos2/ph";
const char* conductivity_topic = "greenhouse/wemos2/conductivity";
const char* clientID = "greenhouse";
const char* mqtt_server = "192.168.1.89"; 
const char* mqtt_username = "admin"; // MQTT username
const char* mqtt_password = "12345678"; // MQTT password
String payload;

WiFiClient wifiClient;
PubSubClient client(mqtt_server,1883,wifiClient);

float ph,ec;

void preTransmission()
{
  digitalWrite(MAX485_RE, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE, 0);
  digitalWrite(MAX485_DE, 0);
}

void setup()
{
  Serial.begin(9600);
  pinMode(MAX485_RE, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE, 0);
  digitalWrite(MAX485_DE, 0);
  Serial.println("start init serial 0");
  while (!Serial) {
    Serial.println("loop for init serial 0"); 
  }
  Serial.println("start init software serial");
  mySerial.begin(9600);
  while (!mySerial) {
    Serial.println("loop for init software serial");
  }
  node.begin(Slave_ID, mySerial);  
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  wifiConnect();
  client.setServer(server, 1883);
}
void ec_sensor(){
  uint8_t result;
  uint16_t data[1];
  result = node.readHoldingRegisters(0x15,1);
  if (result == node.ku8MBSuccess)
  {
    ec = node.getResponseBuffer(0x00);
  }
  }

void ph_sensor(){
  uint8_t result;
  uint16_t data[1];
  result = node.readHoldingRegisters(0x06,1);
  if (result == node.ku8MBSuccess)
  {
    ph = node.getResponseBuffer(0x00);
  }
  }

void wifiConnect() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}

void mqttReConnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientID,mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop(){
  ph_sensor();
  ec_sensor();
  Serial.print("Conductivity: ");
  Serial.println(ec);
  Serial.print("pH: ");
  Serial.println(ph/100);
  Serial.println();
  delay(1000);
  if (!client.connected()) {
    mqttReConnect();
  }
  client.publish(ph_topic, String(ph/100).c_str());
  client.publish(conductivity_topic, String(ec).c_str());
  client.disconnect();
  delay(5000);
}
