#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <Stepper.h>

// Network credentials
const char* ssid = "taranath_dhrn";
const char* password = "CLB275C5DB";

// Python server address
const char* serverAddress = "192.168.1.83";
const int serverPort = 5000;

// Create Web Server
WebServer server(80);

// Pin definitions for stepper motors
#define PUL2 4   // Step pulse pin for altitude stepper
#define DIR2 2   // Direction pin for altitude stepper
#define ENA2 5   // Enable pin for altitude stepper

#define PUL1 18  // Step pulse pin for azimuth stepper
#define DIR1 19  // Direction pin for azimuth stepper
#define ENA1 21  // Enable pin for azimuth stepper

// Position tracking variables
float currentAzimuth = 0.0;    // Current azimuth position in degrees (0-360)
float currentAltitude = 0.0;   // Current altitude position in degrees (0-90)

// Steps per degree - motor calibration factor
const float STEPS_PER_DEGREE = 8.889;  // Adjust based on your motor and gear ratio

// Function declarations
void handleRoot();
void handleManualControl();
void handlePlanetPage();
void handleGetAngles();
void handleUpdateLocation();
void handleGetCurrentPosition();
void handleResetPosition();
void moveMotorByDifference(float newAngle, float &currentAngle, int pulPin, int dirPin, bool isAltitude);
void moveStepperMotor(int steps, int pulPin, int dirPin, bool directionCW);
void setupServer();

// void setup() {
//   Serial.begin(115200);
  
//   // Initialize motor pins
//   pinMode(PUL1, OUTPUT);
//   pinMode(DIR1, OUTPUT);
//   pinMode(ENA1, OUTPUT);
//   pinMode(PUL2, OUTPUT);
//   pinMode(DIR2, OUTPUT);
//   pinMode(ENA2, OUTPUT);
  
//   digitalWrite(ENA1, LOW); // Enable motors
//   digitalWrite(ENA2, LOW);
  
//   // Connect to WiFi
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     Serial.println("Connecting to WiFi...");
//   }
//   Serial.println("Connected to WiFi!");
//   Serial.print("IP Address: ");
//   Serial.println(WiFi.localIP());
  
//   // Set up mDNS responder
//   if(!MDNS.begin("telescope")) {
//     Serial.println("Error setting up mDNS responder!");
//   } else {
//     Serial.println("mDNS responder started - access at http://telescope.local");
//   }
  
//   // Set up web server
//   setupServer();
  
//   // Start server
//   server.begin();
//   Serial.println("HTTP server started");
// }

// void loop() {
//   server.handleClient();
// }

void setupServer() {
  // Main page
  server.on("/", HTTP_GET, handleRoot);
  
  // Manual control page
  server.on("/manual", HTTP_GET, handleManualControl);
  server.on("/manual-control", HTTP_POST, handleManualControl);
  
  // Planet specific pages
  server.on("/planet", HTTP_GET, handlePlanetPage);
  
  // API endpoints
  server.on("/get-angles", HTTP_GET, handleGetAngles);
  server.on("/update-location", HTTP_POST, handleUpdateLocation);
  server.on("/current-position", HTTP_GET, handleGetCurrentPosition);
  server.on("/reset-position", HTTP_GET, handleResetPosition);
  
  // Not found handler
  server.onNotFound([]() {
    server.send(404, "text/plain", "Page Not Found");
  });
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Planetary Tracking System</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: #1a1a2e;
      color: #e6e6e6;
      padding: 20px;
      max-width: 800px;
      margin: 0 auto;
    }
    .container {
      background-color: #16213e;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      margin-bottom: 20px;
    }
    h1, h2 {
      color: #00b4d8;
      text-align: center;
    }
    input, select, button {
      padding: 10px;
      margin: 8px 0;
      border-radius: 5px;
      border: 1px solid #0f3460;
      background-color: #0f3460;
      color: white;
      width: 100%;
      box-sizing: border-box;
    }
    button {
      background-color: #e94560;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.3s;
    }
    button:hover {
      background-color: #ff6b6b;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      color: #90e0ef;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 15px;
      margin-top: 20px;
    }
    .planet-card {
      background-color: #0f3460;
      border-radius: 8px;
      padding: 15px;
      text-align: center;
      transition: transform 0.3s;
    }
    .planet-card:hover {
      transform: translateY(-5px);
      box-shadow: 0 10px 20px rgba(0, 0, 0, 0.3);
    }
    .planet-card h3 {
      color: #00b4d8;
      margin-top: 0;
    }
    .planet-card img {
      width: 80px;
      height: 80px;
      border-radius: 50%;
      margin: 10px 0;
      object-fit: cover;
    }
    .result-box {
      padding: 15px;
      background-color: #0f3460;
      border-radius: 5px;
      margin-top: 10px;
      min-height: 60px;
    }
    .position-display {
      background-color: #0f3460;
      padding: 10px;
      border-radius: 5px;
      margin-top: 15px;
      text-align: center;
    }
    .position-value {
      font-weight: bold;
      color: #00b4d8;
    }
    a {
      text-decoration: none;
      color: inherit;
    }
    .navigation {
      display: flex;
      justify-content: center;
      margin: 20px 0;
    }
    .nav-button {
      margin: 0 10px;
      padding: 10px 15px;
      background-color: #0f3460;
      border-radius: 5px;
      text-decoration: none;
      color: white;
      font-weight: bold;
    }
    .nav-button:hover {
      background-color: #e94560;
    }
  </style>
</head>
<body>
  <h1>Planetary Tracking System</h1>
  
  <div class="navigation">
    <a href="/" class="nav-button">Home</a>
    <a href="/manual" class="nav-button">Manual Control</a>
  </div>
  
  <div class="container">
    <h2>Observer Location</h2>
    <div class="form-group">
      <label for="latitude">Latitude:</label>
      <input type="number" id="latitude" step="0.0000001" required>
    </div>
    <div class="form-group">
      <label for="longitude">Longitude:</label>
      <input type="number" id="longitude" step="0.0000001" required>
    </div>
    <div class="form-group">
      <label for="elevation">Elevation (meters):</label>
      <input type="number" id="elevation" step="0.01" required>
    </div>
    <button onclick="updateLocation()">Update Location</button>
    <button onclick="getDeviceLocation()">Use Device Location</button>
    <div id="locationStatus" class="result-box"></div>
    
    <div class="position-display">
      Current Position: 
      Azimuth <span id="currentAzimuth" class="position-value">0.00</span>° | 
      Altitude <span id="currentAltitude" class="position-value">0.00</span>°
      <button onclick="resetPosition()" style="margin-top:10px;">Reset Position to Zero</button>
    </div>
  </div>
  
  <div class="container">
    <h2>Select a Planet to Track</h2>
    <div class="grid">
      <a href="/planet?body=sun">
        <div class="planet-card">
          <h3>Sun</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/b/b4/The_Sun_by_the_Atmospheric_Imaging_Assembly_of_NASA%27s_Solar_Dynamics_Observatory_-_20100819.jpg/240px-The_Sun_by_the_Atmospheric_Imaging_Assembly_of_NASA%27s_Solar_Dynamics_Observatory_-_20100819.jpg" alt="Sun">
          <p>Track the Sun</p>
        </div>
      </a>
      <a href="/planet?body=moon">
        <div class="planet-card">
          <h3>Moon</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/e/e1/FullMoon2010.jpg/240px-FullMoon2010.jpg" alt="Moon">
          <p>Track the Moon</p>
        </div>
      </a>
      <a href="/planet?body=mercury">
        <div class="planet-card">
          <h3>Mercury</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/d/d9/Mercury_in_color_-_Prockter07-edit1.jpg/240px-Mercury_in_color_-_Prockter07-edit1.jpg" alt="Mercury">
          <p>Track Mercury</p>
        </div>
      </a>
      <a href="/planet?body=venus">
        <div class="planet-card">
          <h3>Venus</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/e/e5/Venus-real_color.jpg/240px-Venus-real_color.jpg" alt="Venus">
          <p>Track Venus</p>
        </div>
      </a>
      <a href="/planet?body=mars">
        <div class="planet-card">
          <h3>Mars</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/0/02/OSIRIS_Mars_true_color.jpg/240px-OSIRIS_Mars_true_color.jpg" alt="Mars">
          <p>Track Mars</p>
        </div>
      </a>
      <a href="/planet?body=jupiter">
        <div class="planet-card">
          <h3>Jupiter</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/2/2b/Jupiter_and_its_shrunken_Great_Red_Spot.jpg/240px-Jupiter_and_its_shrunken_Great_Red_Spot.jpg" alt="Jupiter">
          <p>Track Jupiter</p>
        </div>
      </a>
      <a href="/planet?body=saturn">
        <div class="planet-card">
          <h3>Saturn</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/Saturn_during_Equinox.jpg/240px-Saturn_during_Equinox.jpg" alt="Saturn">
          <p>Track Saturn</p>
        </div>
      </a>
      <a href="/planet?body=uranus">
        <div class="planet-card">
          <h3>Uranus</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/3/3d/Uranus2.jpg/240px-Uranus2.jpg" alt="Uranus">
          <p>Track Uranus</p>
        </div>
      </a>
      <a href="/planet?body=neptune">
        <div class="planet-card">
          <h3>Neptune</h3>
          <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/6/63/Neptune_-_Voyager_2_%2829347980845%29_flatten_crop.jpg/240px-Neptune_-_Voyager_2_%2829347980845%29_flatten_crop.jpg" alt="Neptune">
          <p>Track Neptune</p>
        </div>
      </a>
    </div>
  </div>

  <script>
    // Fetch current position when page loads
    window.onload = function() {
      fetchCurrentPosition();
      fetchServerLocation();
      
      // Refresh position every 5 seconds
      setInterval(fetchCurrentPosition, 5000);
    };
    
    function fetchCurrentPosition() {
      fetch('/current-position')
        .then(response => response.json())
        .then(data => {
          document.getElementById('currentAzimuth').innerText = data.azimuth.toFixed(2);
          document.getElementById('currentAltitude').innerText = data.altitude.toFixed(2);
        })
        .catch(error => {
          console.error('Error fetching position:', error);
        });
    }
    
      function resetPosition() {
      if (confirm('Are you sure you want to reset the current position to zero?')) {
        fetch('/reset-position')
          .then(response => response.json())
          .then(data => {
            if (data.success) {
              alert('Position reset successfully');
              fetchCurrentPosition();
            } else {
              alert('Failed to reset position: ' + data.message);
            }
          })
          .catch(error => {
            alert('Error: ' + error);
          });
      }
    }
    
    function fetchServerLocation() {
      fetch('/get-server-location')
        .then(response => response.json())
        .then(data => {
          document.getElementById('latitude').value = data.latitude;
          document.getElementById('longitude').value = data.longitude;
          document.getElementById('elevation').value = data.elevation;
        })
        .catch(error => {
          console.error('Error fetching location:', error);
          document.getElementById('locationStatus').innerHTML = "Error loading location data";
        });
    }
    
    function updateLocation() {
      const data = {
        latitude: parseFloat(document.getElementById('latitude').value),
        longitude: parseFloat(document.getElementById('longitude').value),
        elevation: parseFloat(document.getElementById('elevation').value)
      };
      
      document.getElementById('locationStatus').innerHTML = "Updating location...";
      
      fetch('/update-location', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
      })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          document.getElementById('locationStatus').innerHTML = "Location updated successfully!";
        } else {
          document.getElementById('locationStatus').innerHTML = "Error: " + data.error;
        }
      })
      .catch(error => {
        document.getElementById('locationStatus').innerHTML = "Failed to update location on server";
        console.error('Error:', error);
      });
    }
    
    function getDeviceLocation() {
      document.getElementById('locationStatus').innerHTML = "Getting device location...";
      
      if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(
          function(position) {
            document.getElementById('latitude').value = position.coords.latitude;
            document.getElementById('longitude').value = position.coords.longitude;
            
            // Use current elevation value
            const elev = document.getElementById('elevation').value || 0;
            
            const data = {
              latitude: position.coords.latitude,
              longitude: position.coords.longitude,
              elevation: parseFloat(elev)
            };
            
            fetch('/update-location', {
              method: 'POST',
              headers: {
                'Content-Type': 'application/json'
              },
              body: JSON.stringify(data)
            })
            .then(response => response.json())
            .then(data => {
              if (data.success) {
                document.getElementById('locationStatus').innerHTML = "Location updated successfully using device GPS!";
              } else {
                document.getElementById('locationStatus').innerHTML = "Error: " + data.error;
              }
            })
            .catch(error => {
              document.getElementById('locationStatus').innerHTML = "Failed to update location on server";
              console.error('Error:', error);
            });
          },
          function(error) {
            let errorMessage = "Unable to retrieve your location: ";
            
            switch(error.code) {
              case error.PERMISSION_DENIED:
                errorMessage += "Permission denied.";
                break;
              case error.POSITION_UNAVAILABLE:
                errorMessage += "Location information unavailable.";
                break;
              case error.TIMEOUT:
                errorMessage += "Request timed out.";
                break;
              default:
                errorMessage += "Unknown error.";
                break;
            }
            
            document.getElementById('locationStatus').innerHTML = errorMessage;
          }
        );
      } else {
        document.getElementById('locationStatus').innerHTML = "Geolocation is not supported by this browser.";
      }
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleManualControl() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Manual Control - Planetary Tracking System</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: #1a1a2e;
      color: #e6e6e6;
      padding: 20px;
      max-width: 800px;
      margin: 0 auto;
    }
    .container {
      background-color: #16213e;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      margin-bottom: 20px;
    }
    h1, h2 {
      color: #00b4d8;
      text-align: center;
    }
    input, select, button {
      padding: 10px;
      margin: 8px 0;
      border-radius: 5px;
      border: 1px solid #0f3460;
      background-color: #0f3460;
      color: white;
      width: 100%;
      box-sizing: border-box;
    }
    button {
      background-color: #e94560;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.3s;
    }
    button:hover {
      background-color: #ff6b6b;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      color: #90e0ef;
    }
    .result-box {
      padding: 15px;
      background-color: #0f3460;
      border-radius: 5px;
      margin-top: 10px;
      min-height: 60px;
    }
    .position-display {
      background-color: #0f3460;
      padding: 10px;
      border-radius: 5px;
      margin-top: 15px;
      text-align: center;
    }
    .position-value {
      font-weight: bold;
      color: #00b4d8;
    }
    .navigation {
      display: flex;
      justify-content: center;
      margin: 20px 0;
    }
    .nav-button {
      margin: 0 10px;
      padding: 10px 15px;
      background-color: #0f3460;
      border-radius: 5px;
      text-decoration: none;
      color: white;
      font-weight: bold;
    }
    .nav-button:hover {
      background-color: #e94560;
    }
  </style>
</head>
<body>
  <h1>Manual Motor Control</h1>
  
  <div class="navigation">
    <a href="/" class="nav-button">Home</a>
    <a href="/manual" class="nav-button">Manual Control</a>
  </div>
  
  <div class="container">
    <h2>Current Position</h2>
    <div class="position-display">
      Azimuth <span id="currentAzimuth" class="position-value">0.00</span>° | 
      Altitude <span id="currentAltitude" class="position-value">0.00</span>°
      <button onclick="resetPosition()" style="margin-top:10px;">Reset Position to Zero</button>
    </div>
  </div>
  
  <div class="container">
    <h2>Manual Control</h2>
    <form id="manualForm" onsubmit="submitManualControl(event)">
      <div class="form-group">
        <label for="azimuth">Azimuth (0° to 360°):</label>
        <input type="number" id="azimuth" name="azimuth" min="0" max="360" step="0.01" required>
      </div>
      <div class="form-group">
        <label for="altitude">Altitude (0° to 90°):</label>
        <input type="number" id="altitude" name="altitude" min="0" max="90" step="0.01" required>
      </div>
      <button type="submit">Move to Position</button>
    </form>
    <div id="manualResult" class="result-box"></div>
  </div>

  <script>
    // Fetch current position when page loads
    window.onload = function() {
      fetchCurrentPosition();
      
      // Refresh position every 5 seconds
      setInterval(fetchCurrentPosition, 5000);
    };
    
    function fetchCurrentPosition() {
      fetch('/current-position')
        .then(response => response.json())
        .then(data => {
          document.getElementById('currentAzimuth').innerText = data.azimuth.toFixed(2);
          document.getElementById('currentAltitude').innerText = data.altitude.toFixed(2);
        })
        .catch(error => {
          console.error('Error fetching position:', error);
        });
    }
    
    function resetPosition() {
      if (confirm('Are you sure you want to reset the current position to zero?')) {
        fetch('/reset-position')
          .then(response => response.json())
          .then(data => {
            if (data.success) {
              alert('Position reset successfully');
              fetchCurrentPosition();
            } else {
              alert('Failed to reset position: ' + data.message);
            }
          })
          .catch(error => {
            alert('Error: ' + error);
          });
      }
    }
    
    function submitManualControl(event) {
      event.preventDefault();
      
      const azimuth = parseFloat(document.getElementById('azimuth').value);
      const altitude = parseFloat(document.getElementById('altitude').value);
      
      document.getElementById('manualResult').innerHTML = "Moving motors...";
      
      fetch('/manual-control', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ azimuth, altitude })
      })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          document.getElementById('manualResult').innerHTML = data.message;
          fetchCurrentPosition();
        } else {
          document.getElementById('manualResult').innerHTML = "Error: " + data.message;
        }
      })
      .catch(error => {
        document.getElementById('manualResult').innerHTML = "Failed to send command to motors";
        console.error('Error:', error);
      });
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handlePlanetPage() {
  String body = server.arg("body");
  if (body.length() == 0) {
    body = "sun"; // Default to sun if no body specified
  }
  
  // Capitalize first letter for display
  String bodyDisplay = body;
  bodyDisplay.setCharAt(0, toupper(bodyDisplay.charAt(0)));
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>)rawliteral" + bodyDisplay + R"rawliteral( Tracking - Planetary System</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background-color: #1a1a2e;
      color: #e6e6e6;
      padding: 20px;
      max-width: 800px;
      margin: 0 auto;
    }
    .container {
      background-color: #16213e;
      border-radius: 10px;
      padding: 20px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      margin-bottom: 20px;
    }
    h1, h2 {
      color: #00b4d8;
      text-align: center;
    }
    input, select, button {
      padding: 10px;
      margin: 8px 0;
      border-radius: 5px;
      border: 1px solid #0f3460;
      background-color: #0f3460;
      color: white;
      width: 100%;
      box-sizing: border-box;
    }
    button {
      background-color: #e94560;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.3s;
    }
    button:hover {
      background-color: #ff6b6b;
    }
    .result-box {
      padding: 15px;
      background-color: #0f3460;
      border-radius: 5px;
      margin-top: 10px;
      min-height: 80px;
    }
    .position-display {
      background-color: #0f3460;
      padding: 10px;
      border-radius: 5px;
      margin-top: 15px;
      text-align: center;
    }
    .position-value {
      font-weight: bold;
      color: #00b4d8;
    }
    .planet-info {
      display: flex;
      align-items: center;
      margin-bottom: 20px;
    }
    .planet-image {
      width: 100px;
      height: 100px;
      border-radius: 50%;
      margin-right: 20px;
      object-fit: cover;
    }
    .planet-details {
      flex: 1;
    }
    .navigation {
      display: flex;
      justify-content: center;
      margin: 20px 0;
    }
    .nav-button {
      margin: 0 10px;
      padding: 10px 15px;
      background-color: #0f3460;
      border-radius: 5px;
      text-decoration: none;
      color: white;
      font-weight: bold;
    }
    .nav-button:hover {
      background-color: #e94560;
    }
    .tracking-active {
      background-color: #4CAF50;
    }
    .tracking-inactive {
      background-color: #e94560;
    }
    .error-message {
      color: #ff6b6b;
      font-weight: bold;
    }
    .success-message {
      color: #4CAF50;
      font-weight: bold;
    }
  </style>
</head>
<body>
   <h1>)rawliteral" + bodyDisplay + R"rawliteral( Tracking</h1>
  
  <div class="navigation">
    <a href="/" class="nav-button">Home</a>
    <a href="/manual" class="nav-button">Manual Control</a>
  </div>
  
  <div class="container">
    <div class="planet-info">
      <img src="" alt=")rawliteral" + bodyDisplay + R"rawliteral(" class="planet-image" id="planetImage">
      <div class="planet-details">
        <h2>)rawliteral" + bodyDisplay + R"rawliteral(</h2>
        <p id="planetDescription">Loading information...</p>
      </div>
    </div>
    
    <div class="position-display">
      Current Position: 
      Azimuth <span id="currentAzimuth" class="position-value">0.00</span>° | 
      Altitude <span id="currentAltitude" class="position-value">0.00</span>°
    </div>
    
    <button id="trackButton" onclick="toggleTracking()" class="tracking-inactive">Start Tracking</button>
    <button onclick="getPosition()">Get Current Position</button>
    
    <div id="trackingResult" class="result-box">
      Press "Get Current Position" to see the current )rawliteral" + bodyDisplay + R"rawliteral( position.
    </div>
  </div>

  <script>
    const body = ")rawliteral" + body + R"rawliteral(";
    let trackingActive = false;
    let trackingInterval;
    
    // Planet images and descriptions
    const planetInfo = {
      sun: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/b/b4/The_Sun_by_the_Atmospheric_Imaging_Assembly_of_NASA%27s_Solar_Dynamics_Observatory_-_20100819.jpg/240px-The_Sun_by_the_Atmospheric_Imaging_Assembly_of_NASA%27s_Solar_Dynamics_Observatory_-_20100819.jpg",
        description: "The Sun is the star at the center of our Solar System and is responsible for the Earth's climate and weather."
      },
      moon: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/e/e1/FullMoon2010.jpg/240px-FullMoon2010.jpg",
        description: "The Moon is Earth's only natural satellite and the fifth largest moon in the Solar System."
      },
      mercury: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/d/d9/Mercury_in_color_-_Prockter07-edit1.jpg/240px-Mercury_in_color_-_Prockter07-edit1.jpg",
        description: "Mercury is the smallest and innermost planet in the Solar System, orbiting the Sun once every 88 Earth days."
      },
      venus: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/e/e5/Venus-real_color.jpg/240px-Venus-real_color.jpg",
        description: "Venus is the second planet from the Sun and is often called Earth's sister planet due to their similar size and mass."
      },
      mars: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/0/02/OSIRIS_Mars_true_color.jpg/240px-OSIRIS_Mars_true_color.jpg",
        description: "Mars is the fourth planet from the Sun and is often referred to as the 'Red Planet' due to its reddish appearance."
      },
      jupiter: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/2/2b/Jupiter_and_its_shrunken_Great_Red_Spot.jpg/240px-Jupiter_and_its_shrunken_Great_Red_Spot.jpg",
        description: "Jupiter is the largest planet in the Solar System and is primarily composed of hydrogen and helium."
      },
      saturn: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/Saturn_during_Equinox.jpg/240px-Saturn_during_Equinox.jpg",
        description: "Saturn is the sixth planet from the Sun and is known for its prominent ring system."
      },
      uranus: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/3/3d/Uranus2.jpg/240px-Uranus2.jpg",
        description: "Uranus is the seventh planet from the Sun and has the third-largest diameter in our solar system."
      },
      neptune: {
        image: "https://upload.wikimedia.org/wikipedia/commons/thumb/6/63/Neptune_-_Voyager_2_%2829347980845%29_flatten_crop.jpg/240px-Neptune_-_Voyager_2_%2829347980845%29_flatten_crop.jpg",
        description: "Neptune is the eighth and farthest known planet from the Sun in the Solar System."
      }
    };
    
    // Fetch current position when page loads
    window.onload = function() {
      fetchCurrentPosition();
      
      // Set planet image and description
      const info = planetInfo[body] || { 
        image: "", 
        description: "No information available." 
      };
      
      document.getElementById('planetImage').src = info.image;
      document.getElementById('planetDescription').innerText = info.description;
      
      // Refresh position every 5 seconds
      setInterval(fetchCurrentPosition, 5000);
    };
    
    function fetchCurrentPosition() {
      fetch('/current-position')
        .then(response => response.json())
        .then(data => {
          document.getElementById('currentAzimuth').innerText = data.azimuth.toFixed(2);
          document.getElementById('currentAltitude').innerText = data.altitude.toFixed(2);
        })
        .catch(error => {
          console.error('Error fetching position:', error);
        });
    }
    
    function getPosition() {
      document.getElementById('trackingResult').innerHTML = "Fetching current position...";
      
      fetch(`/get-angles?body=${body}`)
        .then(response => response.json())
        .then(data => {
          if (data.error) {
            document.getElementById('trackingResult').innerHTML = `<span class="error-message">${data.error}</span>`;
          } else {
            if (data.altitude < 0) {
              document.getElementById('trackingResult').innerHTML = 
                `<span class="error-message">This planet is not visible from your location.</span><br>` + 
                `Altitude: ${data.altitude.toFixed(2)}° (below horizon)<br>` +
                `Azimuth: ${data.azimuth.toFixed(2)}°`;
            } else {
              document.getElementById('trackingResult').innerHTML = 
                `<span class="success-message">Position obtained!</span><br>` + 
                `Altitude: ${data.altitude.toFixed(2)}°<br>` +
                `Azimuth: ${data.azimuth.toFixed(2)}°<br>` +
                `Time: ${new Date().toLocaleTimeString()}`;
                
              // Move motors to this position
              moveToPosition(data.azimuth, data.altitude);
            }
          }
        })
        .catch(error => {
          document.getElementById('trackingResult').innerHTML = `<span class="error-message">Error: ${error}</span>`;
          console.error('Error:', error);
        });
    }
    
    function moveToPosition(azimuth, altitude) {
      fetch('/manual-control', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ azimuth, altitude })
      })
      .then(response => response.json())
      .then(data => {
        if (!data.success) {
          document.getElementById('trackingResult').innerHTML += `<br><span class="error-message">Motor movement error: ${data.message}</span>`;
        }
      })
      .catch(error => {
        document.getElementById('trackingResult').innerHTML += `<br><span class="error-message">Motor control error: ${error}</span>`;
      });
    }
    
    function toggleTracking() {
      const trackButton = document.getElementById('trackButton');
      
      if (trackingActive) {
        // Stop tracking
        clearInterval(trackingInterval);
        trackingActive = false;
        trackButton.innerText = "Start Tracking";
        trackButton.classList.remove('tracking-active');
        trackButton.classList.add('tracking-inactive');
        document.getElementById('trackingResult').innerHTML += "<br><span class='success-message'>Tracking stopped</span>";
      } else {
        // Start tracking
        getPosition(); // Get position immediately
        
        // Then set up interval for continuous tracking
        trackingInterval = setInterval(() => {
          getPosition();
        }, 30000); // Update every 30 seconds
        
        trackingActive = true;
        trackButton.innerText = "Stop Tracking";
        trackButton.classList.remove('tracking-inactive');
        trackButton.classList.add('tracking-active');
      }
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleGetAngles() {
  String body = server.arg("body");
  if (body.length() == 0) {
    body = "sun"; // Default to sun
  }
  
  // Make HTTP request to the Python server
  WiFiClient client;
  if (!client.connect(serverAddress, serverPort)) {
    server.send(500, "application/json", "{\"error\":\"Failed to connect to Python server\"}");
    return;
  }
  
  // Send the request
  client.print(String("GET ") + "/angles?body=" + body + " HTTP/1.1\r\n" +
               "Host: " + serverAddress + "\r\n" +
               "Connection: close\r\n\r\n");
  
  // Wait for the response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      server.send(500, "application/json", "{\"error\":\"Request to Python server timed out\"}");
      return;
    }
  }
  
  // Skip HTTP headers
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line == "\n") break;
  }
  
  // Read the body of the response
  String response = client.readString();
  
  // Parse JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    server.send(500, "application/json", "{\"error\":\"Failed to parse JSON response\"}");
    return;
  }
  
  float altitude = doc["altitude"];
  float azimuth = doc["azimuth"];
  
  // If we got the response, move motors only if altitude is positive
  if (altitude >= 0) {
    moveMotorByDifference(azimuth, currentAzimuth, PUL1, DIR1, false);
    moveMotorByDifference(altitude, currentAltitude, PUL2, DIR2, true);
  }
  
  // Return the response to the client
  server.send(200, "application/json", response);
}

void handleUpdateLocation() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Body not received\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Parse JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to parse JSON\"}");
    return;
  }
  
  // Get location from JSON
  float latitude = doc["latitude"];
  float longitude = doc["longitude"];
  float elevation = doc["elevation"];
  
  // Validate latitude and longitude
  if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid latitude or longitude\"}");
    return;
  }
  
  // Make HTTP request to the Python server
  WiFiClient client;
  if (!client.connect(serverAddress, serverPort)) {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"Failed to connect to Python server\"}");
    return;
  }
  
  // Build JSON to send
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["latitude"] = latitude;
  jsonDoc["longitude"] = longitude;
  jsonDoc["elevation"] = elevation;
  
  String jsonStr;
  serializeJson(jsonDoc, jsonStr);
  
  // Send the request
  client.print(String("POST ") + "/update-location HTTP/1.1\r\n" +
               "Host: " + serverAddress + "\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + jsonStr.length() + "\r\n" +
               "Connection: close\r\n\r\n" +
               jsonStr);
  
  // Wait for the response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      server.send(500, "application/json", "{\"success\":false,\"error\":\"Request to Python server timed out\"}");
      return;
    }
  }
  
  // Skip HTTP headers
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line == "\n") break;
  }
  
  // Read the body of the response
  String response = client.readString();
  
  // Return the response to the client
  server.send(200, "application/json", response);
}

void handleGetCurrentPosition() {
  DynamicJsonDocument doc(256);
  doc["azimuth"] = currentAzimuth;
  doc["altitude"] = currentAltitude;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

void handleResetPosition() {
  // Reset position to zero
  currentAzimuth = 0.0;
  currentAltitude = 0.0;
  
  DynamicJsonDocument doc(256);
  doc["success"] = true;
  doc["message"] = "Position reset to zero";
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}


// Server endpoint for manual motor control
// void handleManualControl() {
//   if (server.hasArg("plain") == false) {
//     server.send(400, "application/json", "{\"success\":false,\"message\":\"Body not received\"}");
//     return;
//   }
  
//   String body = server.arg("plain");
  
//   // Parse JSON
//   DynamicJsonDocument doc(1024);
//   DeserializationError error = deserializeJson(doc, body);
  
//   if (error) {
//     server.send(400, "application/json", "{\"success\":false,\"message\":\"Failed to parse JSON\"}");
//     return;
//   }
  
//   // Get azimuth and altitude from JSON
//   float azimuth = doc["azimuth"];
//   float altitude = doc["altitude"];
  
//   // Validate angles
//   if (azimuth < 0 || azimuth > 360 || altitude < 0 || altitude > 90) {
//     server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid angles. Azimuth must be 0-360, Altitude must be 0-90\"}");
//     return;
//   }
  
//   // Move motors to specified position
//   moveMotorByDifference(azimuth, currentAzimuth, PUL1, DIR1, false);
//   moveMotorByDifference(altitude, currentAltitude, PUL2, DIR2, true);
  
//   // Send response
//   DynamicJsonDocument responseDoc(256);
//   responseDoc["success"] = true;
//   responseDoc["message"] = "Motors moved to position: AZ=" + String(azimuth, 2) + "°, ALT=" + String(altitude, 2) + "°";
  
//   String response;
//   serializeJson(responseDoc, response);
  
//   server.send(200, "application/json", response);
// }

// Handle server location requests
void handleGetServerLocation() {
  // Make HTTP request to the Python server
  WiFiClient client;
  if (!client.connect(serverAddress, serverPort)) {
    server.send(500, "application/json", "{\"error\":\"Failed to connect to Python server\"}");
    return;
  }
  
  // Send the request
  client.print(String("GET ") + "/get-location HTTP/1.1\r\n" +
               "Host: " + serverAddress + "\r\n" +
               "Connection: close\r\n\r\n");
  
  // Wait for the response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      client.stop();
      server.send(500, "application/json", "{\"error\":\"Request to Python server timed out\"}");
      return;
    }
  }
  
  // Skip HTTP headers
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (line == "\n") break;
  }
  
  // Read the body of the response
  String response = client.readString();
  
  // Return the response to the client
  server.send(200, "application/json", response);
}

// Move motor by the difference between current and new angle
void moveMotorByDifference(float newAngle, float &currentAngle, int pulPin, int dirPin, bool isAltitude) {
  float maxAngle = isAltitude ? 90.0 : 360.0;
  
  // Normalize angles to 0-360 or 0-90 range
  newAngle = fmod(newAngle, maxAngle);
  currentAngle = fmod(currentAngle, maxAngle);
  
  // Calculate the shortest path
  float difference = newAngle - currentAngle;
  
  // For azimuth (360° range), find the shortest direction
  if (!isAltitude) {
    if (difference > 180) {
      difference -= 360;
    } else if (difference < -180) {
      difference += 360;
    }
  }
  
  // Calculate required steps
  int steps = abs(difference) * STEPS_PER_DEGREE;
  
  // Determine direction (CW or CCW)
  bool directionCW = difference > 0;
  
  // Move the motor
  moveStepperMotor(steps, pulPin, dirPin, directionCW);
  
  // Update current position
  currentAngle = newAngle;
  
  Serial.println("Moved " + String(isAltitude ? "ALT" : "AZ") + " motor to " + String(newAngle, 2) + "°");
}

// Function to move stepper motor by a specified number of steps
void moveStepperMotor(int steps, int pulPin, int dirPin, bool directionCW) {
  // Set direction
  digitalWrite(dirPin, directionCW ? HIGH : LOW);
  
  // Send pulses
  for (int i = 0; i < steps; i++) {
    digitalWrite(pulPin, HIGH);
    delayMicroseconds(500);  // Adjust pulse width as needed
    digitalWrite(pulPin, LOW);
    delayMicroseconds(500);  // Adjust pulse width as needed
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize motor pins
  pinMode(PUL1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(ENA1, OUTPUT);
  pinMode(PUL2, OUTPUT);
  pinMode(DIR2, OUTPUT);
  pinMode(ENA2, OUTPUT);
  
  digitalWrite(ENA1, LOW); // Enable motors
  digitalWrite(ENA2, LOW);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Set up mDNS responder
  if(!MDNS.begin("telescope")) {
    Serial.println("Error setting up mDNS responder!");
  } else {
    Serial.println("mDNS responder started - access at http://telescope.local");
  }
  
  // Set up web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/manual", HTTP_GET, handleManualControl);
  server.on("/planet", HTTP_GET, handlePlanetPage);
  server.on("/get-angles", HTTP_GET, handleGetAngles);
  server.on("/update-location", HTTP_POST, handleUpdateLocation);
  server.on("/current-position", HTTP_GET, handleGetCurrentPosition);
  server.on("/reset-position", HTTP_GET, handleResetPosition);
  server.on("/manual-control", HTTP_POST, handleManualControl);
  server.on("/get-server-location", HTTP_GET, handleGetServerLocation);
  
  server.onNotFound([]() {
    server.send(404, "text/plain", "Page Not Found");
  });
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

