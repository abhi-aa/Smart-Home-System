#include <Stepper.h>
#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "credentials.h"
#include "ArduinoHttpClient.h"
#include "ArduinoJson.h"

// Stepper motor setup
const int STEPS_PER_REV = 200;
Stepper stepper(STEPS_PER_REV, 8, 9, 10, 11); // Adjust pins as necessary

// WiFi and Firebase setup
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String serverAddress = "smart-home-31620-default-rtdb.firebaseio.com";
int port = 443;
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

void setup() {
  Serial.begin(9600);
  stepper.setSpeed(60); // Default motor speed
  WiFi.begin(ssid, pass); // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

void loop() {
  Serial.println("Checking blinds status...");
  // Check the blinds status from Firebase
  String blindsPath = "/control/blinds.json?auth=" + String(FIREBASE_AUTH);
  client.get(blindsPath);
  int statusCode = client.responseStatusCode();
  if (statusCode == 200) {
    String payload = client.responseBody(); // Get the response body
    Serial.print("Blinds status: ");
    Serial.println(payload);

    // Parse the JSON payload
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return;
    }

    // Extract the "blinds" value from the JSON object
    String blindsStatus = doc["blinds"].as<String>();
    Serial.print("Parsed blinds status: ");
    Serial.println(blindsStatus);

    // Move the stepper motor based on the blinds status
    if (blindsStatus == "open") {
      Serial.println("Command to open blinds received.");
      stepper.step(STEPS_PER_REV); // Rotate clockwise
      Serial.println("Blinds should be opened now.");
      updateBlindsStatus("opened");
    } else if (blindsStatus == "close") {
      Serial.println("Command to close blinds received.");
      stepper.step(-STEPS_PER_REV); // Rotate counterclockwise
      Serial.println("Blinds should be closed now.");
      updateBlindsStatus("closed");
    }
  } else {
    Serial.print("Failed to get blinds status from Firebase, status code: ");
    Serial.println(statusCode);
  }

  client.stop(); // End the connection
  delay(500); // Wait for 0.5 seconds before checking again
}

void updateBlindsStatus(String status) {
  StaticJsonDocument<100> doc;
  doc["blinds"] = status;
  
  String output;
  serializeJson(doc, output);
  
  String blindsPath = "/control/blinds.json?auth=" + String(FIREBASE_AUTH);
  client.put(blindsPath, "application/json", output);
  
  // Optionally, check the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}


