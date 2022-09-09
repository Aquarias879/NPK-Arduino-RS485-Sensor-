#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
//#include "DHTStable.h"
#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include "AdafruitIO_WiFi.h"


DHTesp dht,dht2;
SoftwareSerial mySerial(D7, D6); // RX, TX

#define MAX485_DE  D4
#define MAX485_RE  D5
#define Slave_ID 2

ModbusMaster node;
const char* ssid = "mLab";   
const char* password = "311208211";

//const char* resource = "https://maker.ifttt.com/trigger/test/with/key/dl0lUFDJzYRsWY5sSUi199";
//const char* server1 = "maker.ifttt.com";

// MQTT configuration
const char* server = "203.64.131.98";
const char* humidity_topic = "greenhouse/wemos1/humidity";
const char* temperature_topic = "greenhouse/wemos1/temperature";
const char* nitrogen_topic = "greenhouse/wemos1/nitrogen";
const char* phosphorus_topic = "greenhouse/wemos1/phosphorus";
const char* pottasium_topic = "greenhouse/wemos1/pottasium";
const char* SoilMoisture_topic = "greenhouse/wemos1/SoilMoisture";
const char* SoilMoistureRAW_topic = "greenhouse/wemos1/RAWSoilMoisture";
const char* humidityOut_topic = "greenhouse/wemos2/humidityOutside";
const char* temperatureOut_topic = "greenhouse/wemos2/temperatureOutside";
const char* test = "test";
const char* clientID = "greenhouse";
const char* mqtt_server = "203.64.131.98"; 
const char* mqtt_username = "admin"; // MQTT username
const char* mqtt_password = "12345678"; // MQTT password
String payload;

WiFiClient wifiClient;
PubSubClient client(mqtt_server,1883,wifiClient);

float t,h,n,p,k,t1,h1;
int Soil_value,Soil_valueR;
String humidstr,tempstr;
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
  dht.setup(D8, DHTesp::DHT22);
  dht2.setup(D2, DHTesp::DHT11);
  pinMode(MAX485_RE, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE, 0);
  digitalWrite(MAX485_DE, 0);
  Serial.println("start init serial 0");
  while (!Serial) {
    Serial.println("loop for init serial 0"); 
    Serial.print("Connecting to Adafruit IO");
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

void loop(){
  sensor();
  msg();
  sendmsg();
  delay(100);
  //makeIFTTTRequest();
  delay(200);
  Serial.print("==============\n");
}

void sensor()
{
  uint8_t result;
  uint16_t data[3];
  h = dht.getHumidity();
  t = dht.getTemperature();
  if (isnan(h) || isnan(t)){h=0;t=0;}
  result = node.readHoldingRegisters(0x00,3);
  if (result == node.ku8MBSuccess)
  {
     n = node.getResponseBuffer(0x00);
     p = node.getResponseBuffer(0x01);
     k = node.getResponseBuffer(0x02);
  }
  delay(1000);
}

void soil_moisture(){
  Soil_valueR = analogRead(A0);  //put Sensor insert into soil
  Soil_value= map(Soil_valueR, 0, 1023, 120, 10);
  delay(5000); 
  h1 = dht2.getHumidity();
  t1 = dht2.getTemperature();
  if (isnan(h1) || isnan(t1)){h1=0;t1=0;}
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
      delay(3000);
    }
  }
}

void sendmsg(){
  sensor();
  soil_moisture();
  if (!client.connected()) {
    mqttReConnect();
  }
   client.publish(humidity_topic, String(h).c_str());
   client.publish(temperature_topic, String(t).c_str());
   client.publish(nitrogen_topic, String(n).c_str());
   client.publish(phosphorus_topic, String(p).c_str());
   client.publish(pottasium_topic, String(k).c_str());
   client.publish(SoilMoisture_topic, String(Soil_value).c_str());
   client.publish(SoilMoistureRAW_topic, String(Soil_valueR).c_str());
   client.publish(humidityOut_topic, String(h1).c_str());
   client.publish(temperatureOut_topic, String(t1).c_str());
   client.publish(test,String(t).c_str());
   client.disconnect();
   delay(3000);
}

void msg(){
  sensor();
  soil_moisture();
  Serial.print("humidity: ");
  Serial.print(h);
  Serial.print("\ntemp: ");
  Serial.print(t);
  Serial.print("\n");
  Serial.print("N: ");
  Serial.println(n);
  Serial.print("P: ");
  Serial.println(p);
  Serial.print("K: ");
  Serial.println(k);
  Serial.print("Soil_Moisture: ");
  Serial.println(Soil_value);
  Serial.print("%");
  Serial.print("\nhumidityOut: ");
  Serial.println(h1);
  Serial.print("\ntempOut: ");
  Serial.println(t1);
  Serial.print("\n");
  delay(100);
  }

/*void makeIFTTTRequest() {
  Serial.print("Connecting to ");
  Serial.print(server1);

  WiFiClient client;
  int retries = 5;
  while (!!!client.connect(server1, 80) && (retries-- > 0)) {
    Serial.print(".");
  }
  Serial.println();
  if (!!!client.connected()) {
    Serial.println("Failed to connect...");
  }

  Serial.print("Request resource: ");
  Serial.println(resource);
  String jsonObject = String("{\"value1\":\"") + t + "\",\"value2\":\"" + h+ "\",\"value3\":\"" + Soil_value + "\"}";*/
  /*
 // Temperature in Celsius
 String jsonObject = String("{\"value1\":\"") + bme.readTemperature() + "\",\"value2\":\"" +
(bme.readPressure()/100.0F)
 + "\",\"value3\":\"" + bme.readHumidity() + "\"}";
*/
  // Comment the previous line and uncomment the next line to publish temperature readings in Fahrenheit
    /*String jsonObject = String("{\"value1\":\"") + (1.8 * bme.readTemperature() + 32) +
"\",\"value2\":\""
 + (bme.readPressure()/100.0F) + "\",\"value3\":\"" + bme.readHumidity() + "\"}";*/

 /* client.println(String("POST ") + resource + " HTTP/1.1");
  client.println(String("Host: ") + server1);
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);

  int timeout = 5 * 10;  // 5 seconds
  while (!!!client.available() && (timeout-- > 0)) {
    delay(100);
  }
  if (!!!client.available()) {
    Serial.println("No response...");
  }
  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("\nclosing connection");
  client.stop();
}*/
