#include <Stepper.h>
#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "credentials.h"
#include "ArduinoHttpClient.h"
#include "ArduinoJson.h"

// Stepper motor setup
const int STEPS_PER_REV = 200;
Stepper stepper(STEPS_PER_REV, 8, 9, 10, 11);  // Adjust pins as necessary

// Light sensor and thresholds (for automatic mode)
const int PHOTO_RESISTOR_PIN = A0;
const int MORNING_LIGHT_THRESHOLD = 500;  // Opening threshold
const int EVENING_LIGHT_THRESHOLD = 300;  // Closing threshold
bool blindsOpen = false;

// WiFi and Firebase setup
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String serverAddress = "smart-home-31620-default-rtdb.firebaseio.com";
int port = 443;
String path = "/control/mode.json?auth=" + String(FIREBASE_AUTH);
WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

void setup() {
  // Begin serial communication and set motor speed.
  Serial.begin(9600);
  stepper.setSpeed(60);

  // Initialize WiFi connection.
  WiFi.begin(ssid, pass);  // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  // Set the photoresistor pin as input.
  pinMode(PHOTO_RESISTOR_PIN, INPUT);
}

void loop() {
  // Checks the operation mode (manual/automatic).
  String mode = getMode();

  if (mode == "manual") {
    // Perform manual operations based on blinds status from Firebase
    performManualOperation();
  } else if (mode == "automatic") {
    // Perform automatic operations based on light sensor
    performAutomaticOperation();
  }
  delay(2000);  // Main loop delay
}

String getMode() {
  // Get the current operation mode from Firebase.
  String modePath = "/control/mode.json?auth=" + String(FIREBASE_AUTH);
  client.get(modePath);
  // After the GET request, check the HTTP response to see if successful.
  int statusCode = client.responseStatusCode();  

  // Process the Firebase response and return the mode.
  //200 means GET req is successful
  if (statusCode == 200) {                       
    String modePayload = client.responseBody();  //reads the mode value
    modePayload.trim();
    // Firebase sends JSON data with "" around strings, so remove them for clean text.
    modePayload.replace("\"", "");  
    //debugging purposes
    Serial.print("Mode status: ");  
    Serial.println(modePayload);
    return modePayload;  //returns mode; manual or automatic

  } else {
    // Handle failure to get mode status from Firebase.
    Serial.print("Failed to get mode status from Firebase, status code: ");
    Serial.println(statusCode);
  }
  client.stop();  // End the connection
  return "";
}

void performManualOperation() {
  // Retrieve the blinds open status from Firebase.
  String blindsOpenPath = "/control/blinds_open.json?auth=" + String(FIREBASE_AUTH);
  client.get(blindsOpenPath);

  //check status code to see if req is successful (req = 200)
  int statusCode = client.responseStatusCode();
  if (statusCode == 200) {
    // If the request was successful, read the response body.
    String blindsOpenPayload = client.responseBody();
    blindsOpenPayload.trim();
    blindsOpenPayload.replace("\"", "");  // Remove double quotes

    // Print the blinds open status to the Serial Monitor for debugging.
    Serial.print("Blinds open status: ");
    Serial.println(blindsOpenPayload);

    // Convert the string to a boolean value. blinds should be open if payload = "true"
    bool shouldOpenBlinds = blindsOpenPayload == "true";

    // If the blinds should be open and are currently closed, open them.
    if (shouldOpenBlinds && !blindsOpen) {
      stepper.step(STEPS_PER_REV * 5);
      // Update the state variable to reflect the blinds are now open.
      blindsOpen = true;
      Serial.println("Blinds opened manually.");
      // Update the blinds status in Firebase
      updateBlindsStatus("Open");
    } else if (!shouldOpenBlinds && blindsOpen) {
      stepper.step(-STEPS_PER_REV * 5);
      blindsOpen = false;
      Serial.println("Blinds closed manually.");
      // Update the blinds status in Firebase
      updateBlindsStatus("Closed");
    }
  } else {
    Serial.print("Failed to get blinds open status from Firebase, status code: ");
    Serial.println(statusCode);
  }
  client.stop();  // End the connection
}

void updateBlindsStatus(const String& status) {
  // Create a JSON document that will hold the new status.
  StaticJsonDocument<100> doc;
  // Assign the status to the "blinds" key in the JSON document.
  doc["blinds"] = status;

  // Serialize the JSON document into a string that can be sent in the HTTP request.
  String output;
  serializeJson(doc, output);

  //Firebase path where blinds status is updated
  String blindsStatusPath = "/sensors/arduino2/blinds_status.json?auth=" + String(FIREBASE_AUTH);
  client.put(blindsStatusPath, "application/json", output);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  client.stop();  // End the connection
}

void performAutomaticOperation() {
  // Perform automatic operation based on the light level.
  int lightLevel = analogRead(PHOTO_RESISTOR_PIN);
  Serial.print("Light level: ");
  Serial.println(lightLevel);

  // Open or close blinds based on light thresholds.
  if (lightLevel > MORNING_LIGHT_THRESHOLD && !blindsOpen) {
    stepper.step(STEPS_PER_REV * 5);
    blindsOpen = true;
    Serial.println("Blinds opened automatically based on light level.");
    uploadData(lightLevel, "Open");
  } else if (lightLevel < EVENING_LIGHT_THRESHOLD && blindsOpen) {
    stepper.step(-STEPS_PER_REV * 5);
    blindsOpen = false;
    Serial.println("Blinds closed automatically based on light level.");
    uploadData(lightLevel, "Closed");
  }
}

void uploadData(int lightLevel, String blindsStatus) {
  // Upload the light level and blinds status to Firebase.
  StaticJsonDocument<200> doc;
  doc["lightLevel"] = lightLevel;
  doc["blinds"] = blindsStatus;

  String output;
  serializeJson(doc, output);

  String blindsStatusPath = "/sensors/arduino2/blinds_status.json?auth=" + String(FIREBASE_AUTH);
  client.put(blindsStatusPath, "application/json", output);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  client.stop();  // End the connection
}
