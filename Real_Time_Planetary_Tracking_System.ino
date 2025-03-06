#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <credentials.h>  // You need to install this library

// Network credentials
// const char* ssid = "Your wifi name";
// const char* password = "Your wifi Password";

// Create Web Server
WebServer server(80);

// Pin definitions for stepper motors
#define PUL2 4   // Step pulse pin for first stepper
#define DIR2 2   // Direction pin for first stepper
#define ENA2 5   // Enable pin for first stepper

#define PUL1 18  // Step pulse pin for second stepper
#define DIR1 19  // Direction pin for second stepper
#define ENA1 21  // Enable pin for second stepper

// HTML form with fetch functionality
const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>ESP32 Stepper Control</title>
    <script>
      let interval;

      function submitForm(event) {
        event.preventDefault();
        const formData = new FormData(event.target);
        fetch('/submit?' + new URLSearchParams(formData))
          .then(response => response.text())
          .then(data => {
            document.getElementById('result').innerHTML = data;
          });
      }

      function startSending() {
        interval = setInterval(() => {
          fetch("/get-angles")
            .then(response => response.text())
            .then(data => {
              document.getElementById('apiValues').innerHTML = data;
            });
        }, 10000);
      }

      function stopSending() {
        clearInterval(interval);
      }
    </script>
  </head>
  <body>
    <h1>Stepper Motors Control</h1>
    <form onsubmit="submitForm(event)">
      Stepper 1 Angle (Azimuth): <input type="number" name="var1"><br>
      Stepper 2 Angle (Altitude): <input type="number" name="var2"><br>
      <input type="submit" value="Submit">
    </form>
    <div id="result"></div>
    <br>
    <button onclick="startSending()">Start Sending</button>
    <button onclick="stopSending()">Stop Sending</button>
    <div id="apiValues"></div>
  </body>
</html>)rawliteral";

void moveStepperMotor(int steps, int pulPin, int dirPin, bool directionCW = true, int pulseWidth = 500) {
  // Set direction - allow for reversal if needed
  digitalWrite(dirPin, directionCW ? HIGH : LOW);
  
  // Check if we need to move at all
  if (steps <= 0) {
    return;
  }
  
  // Move the motor
  for (int i = 0; i < steps; i++) {
    digitalWrite(pulPin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(pulPin, LOW);
    delayMicroseconds(pulseWidth);
  }
  
  // Short delay after movement completes
  delay(100); // Reduced from 1000ms to 100ms for faster response
}

void handleFormSubmit() {
  String var1 = server.arg("var1");
  String var2 = server.arg("var2");

  int stepper1Angle = var1.toInt();
  int stepper2Angle = var2.toInt();

  int steps1 = static_cast<int>(8.889 * stepper1Angle);
  int steps2 = static_cast<int>(8.889 * stepper2Angle);

  moveStepperMotor(steps1, PUL1, DIR1, true);  // Try with directionCW = true
moveStepperMotor(steps2, PUL2, DIR2, true);  // Keep this the same


  String response = "Stepper 1 (Azimuth) set to: " + var1 + "°<br>";
  response += "Stepper 2 (Altitude) set to: " + var2 + "°";
  server.send(200, "text/html", response);
}

void handleFetchAngles() {
  WiFiClient client;
  if (client.connect("192.168.1.83", 5000)) {
    // Send HTTP request
    client.println("GET /angles HTTP/1.1");
    client.println("Host: 192.168.1.83");
    client.println("Connection: close");
    client.println();
    
    // Wait for response
    delay(500);
    
    // Skip HTTP headers
    while (client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;  // Headers end with an empty line
      }
    }
    
    // Parse JSON response
    String jsonResponse = "";
    while (client.available()) {
      jsonResponse += client.readString();
    }
    client.stop();
    
    Serial.println("Raw API response: " + jsonResponse);
    
    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonResponse);
    
    if (!error) {
      float altitude = doc["altitude"];
      float azimuth = doc["azimuth"];
      
      Serial.println("Parsed Altitude: " + String(altitude));
      Serial.println("Parsed Azimuth: " + String(azimuth));
      
      int steps1 = static_cast<int>(8.889 * azimuth);
      int steps2 = static_cast<int>(8.889 * altitude);
      
     moveStepperMotor(steps1, PUL1, DIR1, true);  // Try with directionCW = true
moveStepperMotor(steps2, PUL2, DIR2, true);  // Keep this the same
      
      String displayResponse = "Current API Values:<br>";
      displayResponse += "Azimuth: " + String(azimuth) + "°<br>";
      displayResponse += "Altitude: " + String(altitude) + "°";
      server.send(200, "text/html", displayResponse);
    } else {
      Serial.println("JSON parsing failed: " + String(error.c_str()));
      server.send(500, "text/html", "Failed to parse JSON response");
    }
  } else {
    server.send(500, "text/html", "Failed to connect to Python server");
  }
}

void handleStop() {
  digitalWrite(ENA1, HIGH);
  digitalWrite(ENA2, HIGH);
  server.send(200, "text/plain", "Motors Stopped");
}

void setup() {
  Serial.begin(115200);  // Higher baud rate for better debugging

  pinMode(PUL1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(ENA1, OUTPUT);
  pinMode(PUL2, OUTPUT);
  pinMode(DIR2, OUTPUT);
  pinMode(ENA2, OUTPUT);
  
  digitalWrite(ENA1, LOW);
  digitalWrite(ENA2, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlForm);
  });
  server.on("/submit", HTTP_GET, handleFormSubmit);
  server.on("/get-angles", HTTP_GET, handleFetchAngles);
  server.on("/stop", HTTP_GET, handleStop);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
