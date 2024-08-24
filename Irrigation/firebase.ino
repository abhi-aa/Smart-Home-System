#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "arduino_secrets.h"
#include "ArduinoHttpClient.h" // Library to simplify HTTP fetching on Arduino (https://github.com/arduino-libraries/ArduinoHttpClient)
#include "ArduinoJson.h" // ArduinoJson - https://arduinojson.org 
#include "DHT.h"

#define DHT11_PIN 2
DHT dht11(DHT11_PIN, DHT11);

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
// WiFiClient client;

String serverAddress = "smart-home-new-4cf7a-default-rtdb.firebaseio.com";

// HTTPS port
int port = 443;

// Database path
String path = "/sensors/arduino1.json?auth=" + String(FIREBASE_AUTH);

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

bool irrigationActive = false;
bool irrigationAutomation = true;
const int motor = 8;             // Motor connected to digital pin 8
const int OpenAirReading = 700;  // Calibration data 1
const int WaterReading = 280;    // Calibration data 2
const int lowPercentage = 30;
const int highPercentage = 70;
const int targetChange = 3;

int MoistureLevel = 0;
float SoilMoisturePercentage = 0;
float previousPercentage = 0;
int waterDuration = 2000;
int waitDuration = 5000;
float actualChange = 0;
bool start = true;
// Define the states
enum SoilState {
  SOIL_HYDRATED,
  IRRIGATION_NEEDED,
  INITIATING_IRRIGATION
};

const int cooling_led = 5;
const int heating_led = 4;
float humidity  = 0;
float  currentTemp = 20;
float  desiredTemp = 0;
SoilState currentState = SOIL_HYDRATED; // Initial state

/* -------------------------------------------------------------------------- */
void setup() {
/* -------------------------------------------------------------------------- */  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  randomSeed(analogRead(0));
  pinMode(motor, OUTPUT);  //sets the digital pin as output
  pinMode(cooling_led, OUTPUT);  //sets the digital pin as output
  pinMode(heating_led, OUTPUT);  //sets the digital pin as output
  dht11.begin(); // initialize the sensor

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Hello Starting Program");
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
     
    // wait 10 seconds for connection:
    delay(10000);
  }
  
  printWifiStatus();
}

/* -------------------------------------------------------------------------- */
void loop() {
/* -------------------------------------------------------------------------- */  
  printWifiStatus();

  // Read Sensors
  readFirebaseData();
  readMoistureSensor();
  readDHT();

  // Write to database
  writeTempData();

  // Control
  controlTemperatureLED();
  irrigation();
  
  delay(5000);

}

void irrigation(){
  if(irrigationAutomation){
    switch (currentState) {
      case SOIL_HYDRATED:
        if (SoilMoisturePercentage < lowPercentage) {
          currentState = IRRIGATION_NEEDED;
        }
        break;

      case IRRIGATION_NEEDED:
        if(start) {
          Serial.println("Initiating Irrigation");
          waterDuration = 2000;
          start = false;
          previousPercentage = SoilMoisturePercentage;
        }else {
          actualChange = abs(previousPercentage - SoilMoisturePercentage);
          Serial.println("change" + String(actualChange));
          if(actualChange <= 0.5){
            actualChange = targetChange*0.7;
          }
          waterDuration =  static_cast<int>((targetChange/actualChange) * waterDuration);
          if (waterDuration > 8000){
            waterDuration = 8000;
          }
        }
        currentState = INITIATING_IRRIGATION;
        break;

      case INITIATING_IRRIGATION:
        Serial.println("Watering for " + String(waterDuration) + "ms");
        digitalWrite(motor, HIGH);
        delay(waterDuration);
        digitalWrite(motor, LOW);
        delay(waitDuration);

        if(SoilMoisturePercentage > highPercentage){
          currentState = SOIL_HYDRATED; // Transition back to SOIL_HYDRATED after irrigation
          start = true;
        }else{
          currentState = IRRIGATION_NEEDED;
          Serial.println("Finished Watering");
        }
        break;

      // Update previous percentage regardless of the state
      previousPercentage = SoilMoisturePercentage;
    }

  

  }else{
    currentState = SOIL_HYDRATED;
    start = true;
    waterDuration = 2000;
    if(irrigationActive){
    digitalWrite(motor, HIGH);
    }else{
      digitalWrite(motor, LOW);
    }
  }
}

void readDHT(){
  // read humidity
  humidity  = dht11.readHumidity();
  // read temperature as Celsius
  currentTemp = dht11.readTemperature();

  // check if any reads failed
  if (isnan(humidity) || isnan(currentTemp)) {
    Serial.println("Failed to read from DHT11 sensor!");
  } else {
    Serial.print("DHT11# Humidity: ");
    Serial.print(humidity);
    Serial.print("%");

    Serial.print("  |  "); 

    Serial.print("Temperature: ");
    Serial.print(currentTemp);
    Serial.print("°C ~ ");
  }
}

void controlTemperatureLED() {
  Serial.print("Desired Temperature: ");
  Serial.println(desiredTemp);
  Serial.print("Current Temperature: ");
  Serial.println(currentTemp);


  if (currentTemp < desiredTemp) {
    Serial.println("Heating");
    digitalWrite(4, HIGH); // Turn on LED connected to pin 4
    digitalWrite(5, LOW);  // Turn off LED connected to pin 5
  } else if (currentTemp > desiredTemp) {
    Serial.println("Cooling");
    digitalWrite(4, LOW);  // Turn off LED connected to pin 4
    digitalWrite(5, HIGH); // Turn on LED connected to pin 5
  } else {
    digitalWrite(4, LOW);  // Turn off LED connected to pin 4
    digitalWrite(5, LOW);  // Turn off LED connected to pin 5
  }
}


void readMoistureSensor() {
  MoistureLevel = analogRead(A0);  // Update based on the analog Pin selected
  SoilMoisturePercentage = map(MoistureLevel, OpenAirReading, WaterReading, 0, 100);
  Serial.println("Soil Percent @" + String(SoilMoisturePercentage));
}

/* -------------------------------------------------------------------------- */
void writeTempData() {
/* -------------------------------------------------------------------------- */
  Serial.println("PUT request");
  // Set HTTP header
  String contentType = "application/x-www-form-urlencoded";

  // Create JSON structure
  JsonDocument doc;

  JsonObject temperatureJsonObject = doc["temperature"].to<JsonObject>();
  temperatureJsonObject["value"] = String(currentTemp, 2);
  temperatureJsonObject["unitOfMeasure"] = "°C";

  JsonObject humidityJsonObject = doc["humidity"].to<JsonObject>();
  humidityJsonObject["value"] = String(humidity, 2);
  humidityJsonObject["unitOfMeasure"] = "%";

  JsonObject soilMoisutreJsonObject = doc["soilMoisture"].to<JsonObject>();
  soilMoisutreJsonObject["value"] = String(SoilMoisturePercentage, 2);
  soilMoisutreJsonObject["state"] = currentState;

  String output;
  doc.shrinkToFit();
  serializeJson(doc, output);

  Serial.println(output);

  // PUT request with parameters
  client.put(path, contentType, output);

  // Response status code
  int statusCode = client.responseStatusCode();
  // Response body
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  // HTTP client stop
  client.stop();
}



/* -------------------------------------------------------------------------- */
void readFirebaseData() {
/* -------------------------------------------------------------------------- */
  Serial.println("GET request");

  // Endpoint to the Firebase node you want to read from
  String endpoint = "/control.json?auth=" + String(FIREBASE_AUTH);

  // GET request
  client.get(endpoint);

  // Response status code
  int statusCode = client.responseStatusCode();

  // Response body
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  // Parse the JSON response
  if (statusCode == 200) {
    // Parse JSON response here and extract the data
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);

    // Check if parsing succeeded
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // Extract values
    irrigationActive = doc["irrigation_active"];
    irrigationAutomation = doc["irrigation_automation"];
    desiredTemp = doc["desired_temp"]["value"];

    // Print extracted values
    Serial.print("Irrigation Active: ");
    Serial.println(irrigationActive);
    Serial.print("Irrigation Automation: ");
    Serial.println(irrigationAutomation);
  }

  // HTTP client stop
  client.stop();
}

/* -------------------------------------------------------------------------- */
void printWifiStatus() {
/* -------------------------------------------------------------------------- */  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}