#include <Wire.h> // For I2C communications with the accelerometer(s)
#include <SPI.h> // Not sure why we need to keep this as we're using I2C, but this sketch fails compilation if we comment it out
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define wifiSSID "YOUR_SSID"
#define wifiPassword "YOUR_WIFI_PASSWORD"
#define mqttServer "YOUR_MQTT_SERVER_ADDRESS"
#define mqttUser "YOUR_MQTT_USERNAME"
#define mqttPassword "YOUR_MQTT_PASS"
#define topic "laundry/test"
#define wLightPin 2
#define dLightPin 2
// Could vary by washer and dryer, these worked for me
#define washerClickThreshhold 14
#define dryerClickThreshhold 16

// Initialize sensors.
Adafruit_LIS3DH washer = Adafruit_LIS3DH(); // washer
Adafruit_LIS3DH dryer = Adafruit_LIS3DH(); // dryer

// Global variables. 
int wClicks = 0;
int dClicks = 0;
int lowActivity = 0;
unsigned long startWLight = 0;
unsigned long startDLight = 0;
boolean wLightOn = false;
boolean dLightOn = false;
unsigned long startTime = 0;

// Wi-Fi stuff
WiFiClient espClient;
PubSubClient psClient(espClient);

void setup() {
  // Blink lights to show device is operational.
  blinkBoth(1);
  Serial.begin(115200);
  Serial.println("Washer/Dryer Monitor Starting");
  
  sensorSetup();
  // Blink lights to show sensors setup is complete. 
  blinkBoth(2);
  
  pinMode(wLightPin, OUTPUT); // declare the wLightPin as as OUTPUT
  pinMode(dLightPin, OUTPUT); // declare the dLightPin as as OUTPUT
  
  setupWifi();
  // Blink lights to show Wi-Fi connected.
  blinkBoth(3);
  
  // Connect to MQTT broker and send startup message.
  psClient.setServer(mqttServer, 1883);
  if (!psClient.connected()) {
    reconnect();
  }
  psClient.loop();
  psClient.publish(topic, "Completed setup. 0 0 0");
  blinkBoth(4);
  startWLight = startDLight = startTime = millis();
} //end 'Setup' section

// Blink LEDs
void blinkBoth(int num) {
  for (int z = 1; z <= num; z++) {
    digitalWrite(wLightPin, LOW);
    digitalWrite(dLightPin, LOW);
    delay(250);
    digitalWrite(wLightPin, HIGH);
    digitalWrite(dLightPin, HIGH);
    delay(250);
  }
}

// Set up Wi-Fi
void setupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifiSSID);

  WiFi.hostname("LaundryMonitor");
  WiFi.begin(wifiSSID, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
} // end setupWifi()

void reconnect() {
  // Loop until we're reconnected
  while (!psClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (psClient.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(psClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} // end reconnect()

// Set up sensors.
void sensorSetup() {
  if (! washer.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Could not start washer sensor");
    while (1);
  }
  Serial.println("LIS3DH washer sensor found!");

  if (! dryer.begin(0x19)) {   // changed to 0x19 for alternative i2c address
    Serial.println("Could not start dryer sensor");
    while (1);
  }
  Serial.println("LIS3DH dryer sensor found!");
  washer.setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G! // washer
  dryer.setRange(LIS3DH_RANGE_2_G);    // 2, 4, 8 or 16 G! // dryer
  
  Serial.print("Range1 = ");
  Serial.print(2 << washer.getRange());  
  Serial.println("G");
  Serial.print("Range2 = ");
  Serial.print(2 << dryer.getRange());  
  Serial.println("G");
  Serial.println("");
  
  // 0 = turn off click detection & interrupt
  // 1 = single click only interrupt output
  // 2 = double click only interrupt output, detect single click
  // Adjust threshhold, LOWer numbers are less sensitive
  washer.setClick(2, washerClickThreshhold);
  dryer.setClick(2, dryerClickThreshhold);
  delay(100);
} // end sensorSetup()

// Send MQTT message.
void sendMQTT() {
  String tempString = String(wClicks) + " Washer clicks and " + String(dClicks) + " Dryer clicks in last 10 seconds.";
  int messageLength = tempString.length() + 1;
  char message[messageLength];
  tempString.toCharArray(message, messageLength);
  if (wClicks + dClicks > 0) {
    Serial.println();
  }
  wClicks = dClicks = 0;
  
  if (!psClient.connected()) {
    reconnect();
  }
  psClient.loop();
  psClient.publish(topic, message, messageLength);
  Serial.println(message);
} // sendMQTT()

// Loop function. This is where the action is!
void loop() {
  // Turn washer light off if it has been on for 50 ms. 
  if (wLightOn) {
    if (millis() >= startWLight + 50) {
      digitalWrite(wLightPin, HIGH);
      wLightOn = false;
    }
  }

  // Turn dryer light off if it has been on for 50 ms.
  if (dLightOn) {
    if (millis() >= startDLight + 50) {
      digitalWrite(dLightPin, HIGH);
      dLightOn = false;
    }
  }

  // Tally up clicks every 10 seconds. Send MQTT message if applicable. 
  if (millis() >= startTime + 10000) {
    Reset all numbers if the millis() result overflows. 
	if (millis() + 10000 < startTime) {
      startTime = startDLight = startWLight = millis();
    }
    lowActivity = lowActivity + 1;
    if (wClicks + dClicks > 20) {
      lowActivity = 0;
    }
	// Send MQTT messages unless 100 ten-second periods with no activity have elapsed. 
    if (lowActivity <= 100) {
      sendMQTT();
    }
    startTime = millis();
  }

  uint8_t wClick = washer.getClick();
  uint8_t dClick = dryer.getClick();

  // Count washer clicks. Turn the washer LED on if at least one click detected. 
  if (wClick & 0x10 || wClick & 0x20) {
    wClicks = wClicks + 1;
    Serial.print("w");
    if (!wLightOn) {
      digitalWrite(wLightPin, LOW);
      wLightOn = true;
      startWLight = millis();
    }
  }
  
  // Count dryer clicks. Turn the dryer LED on if at least one click detected. 
  if (dClick & 0x10 || dClick & 0x20) {
    dClicks = dClicks + 1;    
    Serial.print("d");
    if (!dLightOn) {
      digitalWrite(dLightPin, LOW);
      dLightOn = true;
      startDLight = millis();
    }
  }
} // end loop()
