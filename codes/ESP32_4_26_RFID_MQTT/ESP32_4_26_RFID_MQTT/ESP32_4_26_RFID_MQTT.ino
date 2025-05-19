// enable or disable GxEPD2_GFX base class
/* Wiring:
  E-Paper ke ESP32:
  CS 	  --> G5
  SCL 	--> G18
  SDA 	--> G23
  BUSY	--> G15
  RST	  --> G2
  DC	  --> G0

  RFID Reader RC522 ke ESP32:
  VCC 	--> VCC 3v
  GND 	--> GND
  RST	  --> G4
  MISO	--> G19
  MOSI	--> G23
  SCK	  --> G18
  SS	  --> G21
*/

#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold30pt7b.h>
#include <Fonts/FreeMonoBold36pt7b.h>
#include <Fonts/FreeMonoBold42pt7b.h>

#include <WiFi.h>

//For MQTT
#include <PubSubClient.h>
//Library for Json format using version 7, please update ArduinoJson Library if you still in version 5 or 6
#include <ArduinoJson.h>

//declare topic for publish message
const char* topic_pub = "101/pub";
//declare topic for subscribe message
const char* topic_sub = "101/sub";

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqtt_server = "105.261.134.107";  //without port

WiFiClient espClient;
PubSubClient client(espClient);

// For RFID-----------------------------------
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21  //D4
#define RST_PIN 4  //D3

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
unsigned long lastUID = 0;         // Stores the last UID read
bool cardDetected = false;         // Status whether there is a card in range
unsigned long uidDec = 0;
//---------------------------------------------

// ESP32 CS(SS)=5,SCL(SCK)=18,SDA(MOSI)=23,BUSY=15,RES(RST)=2,DC=0
// 4.26'' EPD Module
GxEPD2_BW<GxEPD2_426_GDEQ0426T82, GxEPD2_426_GDEQ0426T82::HEIGHT> display(GxEPD2_426_GDEQ0426T82(/*CS=*/5, /*DC=*/0, /*RES=*/2, /*BUSY=*/15));  // DEPG0213BN 122x250, SSD1680
//variable for header
String store_no = "101";
String store_name = "AFTER PLATING";
String part_name = "BODY ABC-1234";  //max 15 character
String part_no = "ABC-1234-01-PL";   //mac  16 character
String total = "0";

void callback(char* topic, byte* payload, unsigned int length) {
  //Receiving message as subscriber
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Received:");
  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println(json_received);

  //Parse json
  JsonDocument root;
  deserializeJson(root, json_received);

  //get json parsed value
  //sample of json: {"command":"change","part_no":"BODY ABC-1234","part_name":"ABC-1234-01-PL","total":5}
  //sample of json: {"command":"total","total":5}
  //sample of json: {"command":"lamp","status":true}
  if (root["command"] == "change") {
    part_name = root["part_name"].as<String>();
    part_no = root["part_no"].as<String>();
    total = root["total"].as<String>();
    showTotal();
    showContent();
  }
  if (root["command"] == "total") {
    total = root["total"].as<String>();
    showTotal();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      //once connected to MQTT broker, subscribe command if any
      client.subscribe(topic_sub);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  delay(1000);
  //MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //subscribe topic
  client.subscribe(topic_sub);

  //For E-Paper--------------------------------------------------------
  display.init(115200, true, 200, false);
  showHeader();
  showContent();
  showTotal();
  delay(1000);

  //For RFID------------------------------------------------------------
  SPI.begin();         // Initiate SPI bus
  mfrc522.PCD_Init();  // Initiate MFRC522
  delay(500);
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void showHeader() {
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    //create line top
    display.fillRect(0, 0, 800, 6, GxEPD_BLACK);  //x,y,width, height
    //create line bottom
    display.fillRect(0, 474, 800, 6, GxEPD_BLACK);
    //create line left
    display.fillRect(0, 0, 6, 480, GxEPD_BLACK);
    //create line right
    display.fillRect(794, 0, 6, 480, GxEPD_BLACK);
    //create line horizontal
    display.fillRect(0, 120, 800, 6, GxEPD_BLACK);
    display.fillRect(0, 310, 800, 6, GxEPD_BLACK);
    //create line vertical
    display.fillRect(250, 0, 6, 120, GxEPD_BLACK);
    display.fillRect(590, 316, 6, 160, GxEPD_BLACK);

    //header
    display.setFont(&FreeMonoBold18pt7b);
    display.setCursor(12, 30);
    display.print("Address:");
    display.setCursor(12, 150);
    display.print("Part Name:");
    display.setCursor(12, 340);
    display.print("Part No:");
    display.setCursor(262, 30);
    display.print("Store Name:");
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(600, 340);
    display.print("Total Kanban:");
  } while (display.nextPage());

  //Store No and Store Name
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  int16_t x, y;
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  //Store No Partial Screen
  display.setFont(&FreeMonoBold36pt7b);
  display.setPartialWindow(20, 40, 215, 70);  //based on box area
  display.firstPage();
  do {
    display.fillRect(20, 40, 215, 70, GxEPD_WHITE);  //x,y,width, height
    display.getTextBounds(store_no, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = 20 + (215 - tbw) / 2;  //x = box_x + (box_w - tbw) / 2
    y = 40 + (70 - tbh) / 2 + tbh;
    display.setCursor(x, y);
    display.print(store_no);
  } while (display.nextPage());

  //Store Name Partial Screen
  display.setFont(&FreeMonoBold30pt7b);
  display.setPartialWindow(270, 40, 510, 70);  //based on box area
  display.firstPage();
  do {
    display.fillRect(270, 40, 510, 70, GxEPD_WHITE);  //x,y,width, height
    display.getTextBounds(store_name, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = 270 + (510 - tbw) / 2;      //x = box_x + (box_w - tbw) / 2
    y = 40 + (70 - tbh) / 2 + tbh;  //y = box_y + (box_h - tbh) / 2 + tbh
    display.setCursor(x, y);
    display.print(store_name);
  } while (display.nextPage());
}

void showContent() {
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  int16_t x, y;
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);

  //Part Name Partial Screen
  display.setFont(&FreeMonoBold42pt7b);
  display.setPartialWindow(20, 160, 760, 140);  //based on box area
  display.firstPage();
  do {
    display.fillRect(20, 160, 760, 140, GxEPD_WHITE);  //x,y,width, height
    display.getTextBounds(part_name, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = 20 + (760 - tbw) / 2;         //x = box_x + (box_w - tbw) / 2
    y = 160 + (140 - tbh) / 2 + tbh;  //y = box_y + (box_h - tbh) / 2 + tbh
    display.setCursor(x, y);
    display.print(part_name);
  } while (display.nextPage());

  //Part No Partial Screen
  display.setFont(&FreeMonoBold30pt7b);
  display.setPartialWindow(20, 355, 560, 105);  //based on box area
  display.firstPage();
  do {
    display.fillRect(20, 355, 560, 105, GxEPD_WHITE);  //x,y,width, height
    display.getTextBounds(part_no, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = 20 + (560 - tbw) / 2;         //x = box_x + (box_w - tbw) / 2
    y = 355 + (105 - tbh) / 2 + tbh;  //y = box_y + (box_h - tbh) / 2 + tbh
    display.setCursor(x, y);
    display.print(part_no);
  } while (display.nextPage());
}

void showTotal() {
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  int16_t x, y;
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold42pt7b);
  display.setPartialWindow(610, 355, 170, 105);  //based on box area
  display.firstPage();
  do {
    display.fillRect(610, 355, 170, 105, GxEPD_WHITE);  //x,y,width, height
    display.getTextBounds(total, 0, 0, &tbx, &tby, &tbw, &tbh);
    x = 610 + (170 - tbw) / 2;  //x = box_x + (box_w - tbw) / 2
    y = 355 + (105 - tbh) / 2 + tbh;
    display.setCursor(x, y);
    display.print(total);
  } while (display.nextPage());
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Detect whether there is a new card or  cannot read the card serial
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    lastUID = 0;
    return;  // No new card or can't read serial, continue to loop
  }

  // Convert UID to decimal
  for (int i = mfrc522.uid.size - 1; i >= 0; i--)  // Reverse byte order
  {
    uidDec = (uidDec << 8) | mfrc522.uid.uidByte[i];
  }

  // If UID is different, display it
  if (uidDec != lastUID) {
    lastUID = uidDec;  // Save the latest UID

    // Show UID in Hex format
    Serial.print("UID (Hex): ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      if (i < mfrc522.uid.size - 1) Serial.print(" ");
    }
    Serial.println();

    // Display UID in Decimal format
    Serial.print("UID (USB Dec): ");
    Serial.println(uidDec);
    //send to MQTT
    client.publish(topic_pub, String(uidDec).c_str());
  }

  // Wait for the card to be removed, make sure no cards are detected before continuing.
  while (mfrc522.PICC_IsNewCardPresent()) {
    delay(50);  // Wait a moment to make sure the card is completely removed.
  }
}
