#include<ESP8266WiFi.h>
#include<ArduinoJson.h>
#include<PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
DHT sensor(D1, DHT11);
DynamicJsonDocument status(16);//接收继电器
DynamicJsonDocument json(32);//接收灯
DynamicJsonDocument dhtjson(32);//dht
WiFiClient client;
PubSubClient mqttclient(client);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, D7, NEO_GRB + NEO_KHZ800);
const char* light = "xaut/light";
const char* dht = "xaut/temp/dhtsensor";
const char* will_topic = "xaut/temp/connectstatus";
const char* switch1 = "xaut/hlswitch";
int r, g, b;
void callback(char* topic, byte* payload, unsigned int length)
{
    //灯
    Serial.println(topic);
    if (!strcmp(topic, light)) {
        deserializeJson(json, payload, length);
        //Serial.println(status);
        if (json["status"].as<String>() == "party") {
            for(int k=0; k<5; k++){
                for (int i = 0; i < 8; i++) {
                    r = random(0, 100);
                    g = random(0, 100);
                    b = random(0, 100);
                    pixels.setPixelColor(i, pixels.Color(r, g, b));
                    pixels.show();
                    delay(100);
                }
                for (int i = 0; i < 8; i++) {
                    r=g=b=0;
                    pixels.setPixelColor(i, pixels.Color(r, g, b));
                    pixels.show();
                    //delay(100);
                }
                
            }

        } if (json["status"].as<String>() == "sleep") {
            for (int i = 0; i < 8; i++) {
                r = 5;
                g = 5;
                b = 5;
                pixels.setPixelColor(i, pixels.Color(r, g, b));
                pixels.show();
                delay(100);
            }
        }
    }
    //继电器
    if (!strcmp(topic, switch1)) {
        deserializeJson(status, payload, length);
        //Serial.println(status);
        if (status["status"].as<String>() == "on") {

            digitalWrite(D6, LOW);
        } if (status["status"].as<String>() == "off") {
            digitalWrite(D6, HIGH);
        }
    }

}

void setup() {
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);
    pinMode(D6, OUTPUT);
    digitalWrite(D6, HIGH);
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin("Honor 8 Lite", "9804120109zyy.");
    while (!WiFi.isConnected()) {
        delay(500);
        Serial.print(".");
    }
    pixels.begin();     //启动灯
    sensor.begin();     //启动DHT
    Serial.println(WiFi.localIP());
    configTime(8 * 3600, 0, "pool.ntp.org"); //启动时获取一次
    mqttclient.setServer("192.168.43.192", 1883);
    mqttclient.setCallback(callback);
}

void loop() {
    time_t now = time(nullptr);
    Serial.print(ctime(&now));
    if (!mqttclient.connected()) {
        //mqttclient.connect("general");//客户端id
        String clientID = "general";
        if (mqttclient.connect(clientID.c_str(), will_topic, 1, true, "{\"online\":\"false\"}")) {
            mqttclient.publish(will_topic, "{\"online\":\"true\"}", true);
        }
        else
        {
            Serial.println("connect failed code = " + mqttclient.state());
        }
        mqttclient.subscribe(light, 1);
        mqttclient.subscribe(switch1, 1);
    }
    dhtjson["temperature"] = sensor.readTemperature();
    dhtjson["humidity"] = sensor.readHumidity();
    String jsonstr;
    serializeJsonPretty(dhtjson, jsonstr);
    Serial.println("DHTSensor" + jsonstr);
    mqttclient.publish(dht, jsonstr.c_str());
    mqttclient.loop();//自动发送心跳，保持和服务器的连接
    delay(1000);
}
