#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Stepper.h>
#include <WebSocketsServer.h>

const char* ssid = "erccomputer_dhrn_2";
const char* password = "CLB29A60D6"; //  Replace with your WiFi password


WiFiServer server(80);
WebSocketsServer webSocket(81);

#define PUL2 4   // Step pulse pin for altitude stepper
#define DIR2 2   // Direction pin for altitude stepper
#define ENA2 5   // Enable pin for altitude stepper

#define PUL1 18  // Step pulse pin for azimuth stepper
#define DIR1 19  // Direction pin for azimuth stepper
#define ENA1 21
const float STEPS_PER_DEGREE = 8.889;

// Position variables
int currAlt = 0;
int prevAlt = 0;
int currAz = 0;
int prevAz = 0;
bool startTracking = false;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
  
  
  pinMode(PUL1, OUTPUT);
  pinMode(DIR1, OUTPUT);
  pinMode(ENA1, OUTPUT);
  pinMode(PUL2, OUTPUT);
  pinMode(DIR2, OUTPUT);
  pinMode(ENA2, OUTPUT);

  digitalWrite(ENA1, LOW);
  digitalWrite(ENA2, LOW);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  
  webSocket.loop();
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  if (type != WStype_TEXT) return;
  // Convert payload to a String for easier handling
  String message = String((char*)payload);
  Serial.println("Received: " + message);
  
  // Check if message contains comma (indicating it has parameters)
  if (message.indexOf(',') != -1) {
    // Split the message into parts
    String parts[3]; // Maximum 3 parts: command, value1, value2
    int partIndex = 0;
    int lastIndex = 0;
    int nextIndex = message.indexOf(',');
    
    while (nextIndex != -1 && partIndex < 3) {
      parts[partIndex++] = message.substring(lastIndex, nextIndex);
      lastIndex = nextIndex + 1;
      nextIndex = message.indexOf(',', lastIndex);
    }
    
    // Get the last part
    if (lastIndex < message.length() && partIndex < 3) {
      parts[partIndex++] = message.substring(lastIndex);
    }
    
    // Now parts[0] contains the command, parts[1] contains value1, etc.
    String command = parts[0];
    float value1 = (partIndex > 1) ? parts[1].toFloat() : 0.0;
    float value2 = (partIndex > 2) ? parts[2].toFloat() : 0.0;
    if(value1 == 0 && value2 == 0){
      if(command == "startTracking"){
      startTracking = true;
    }else if(command=="stopTracking"){
      startTracking = false;
    }else if(command == "reset"){
      run_Az("HIGH", prevAz);
      prevAz=0;
      run_Alt("HIGH", prevAlt);
      prevAlt = 0;
      }
    } else {
    // Calculate steps from value1
    currAz = (int)(value1 * STEPS_PER_DEGREE);
    currAlt = (int)(value2 * STEPS_PER_DEGREE);
    Serial.print("Current Azi :");
    Serial.println(currAz);
    Serial.print("Current Alt :");
    Serial.println(currAlt);
    Serial.print("Previous Az:");
    Serial.println(prevAz);
    Serial.print("Prevous Alt:");
    Serial.println(prevAlt);
    if(prevAz >=3200){
      run_Az("HIGH", prevAz);
      prevAz=0;
    }else if(prevAlt >=800){
       run_Alt("HIGH", prevAlt);
      prevAlt = 0;
    }else if(startTracking){
      if(currAlt < 0 || currAz < 0){
        Serial.println("This planet cannot be tracked now");
      }else{
        executeCommand(command, currAz, currAlt);
      }
    }else{
      Serial.println("Currently not tracking!!!!");
    }
    }
  } 

}

// Function to execute motor commands
void executeCommand(String command, int currAz, int currAlt) {

  if(command != "Azi_forward" && command != "Azi_backward" && command != "Alt_forward" && command != "Alt_backward" ){
    Serial.println(command);
   if(currAz == prevAz){
      if(currAlt > prevAlt){
        int diff_Alt = currAlt - prevAlt;
        if(prevAlt+diff_Alt <= 800){
          run_Alt("LOW", diff_Alt);
          prevAlt +=diff_Alt;
        }else{
          int mod = (prevAlt+diff_Alt)%800;
          int move = diff_Alt - mod;
          run_Alt("LOW", move);
          prevAlt=800;
        }
        
      }else{
        int diff_Alt =prevAlt - currAlt ;
        if(prevAlt-diff_Alt >= 0){
          run_Alt("HIGH", diff_Alt);
          prevAlt -=diff_Alt;
        }else{
          int mod = diff_Alt-prevAlt;
          int move = diff_Alt-mod;
          run_Alt("HIGH", move);
          prevAlt=0;
        }
        
      }
    } else if(currAz > prevAz){
        int diff_Az = currAz-prevAz;
        if(prevAz+diff_Az <= 3200){
          if(currAlt == prevAlt){
          run_Az("LOW", diff_Az);
          prevAz += diff_Az;
        } else if(currAlt > prevAlt){
          int diff_Alt = currAlt - prevAlt;
          if(prevAlt+diff_Alt <= 800){
            run_Az("LOW", diff_Az);
            prevAz += diff_Az;
            run_Alt("LOW", diff_Alt);
            prevAlt += diff_Alt;
          }else{
            int mod = (prevAlt+diff_Alt)%800;
            int move = diff_Alt - mod;
            run_Az("LOW", diff_Az);
            prevAz +=diff_Az;
            run_Alt("LOW", move);
            prevAlt = 800;
          }
          }else{
          int diff_Alt =prevAlt - currAlt ;
          if(prevAlt-diff_Alt >= 0){
            run_Az("LOW", diff_Az);
            prevAz +=diff_Az;
            run_Alt("HIGH", diff_Alt);
            prevAlt-=diff_Alt;
          }else{
            int mod= diff_Alt-prevAlt;
            int move = diff_Alt-mod;
            run_Az("LOW", diff_Az);
            prevAz +=diff_Az;
            run_Alt("HIGH", move);
            prevAlt=0;
          }
        }
        } else{
          int mod = (prevAz+diff_Az)%3200;
          int moves = diff_Az-mod;
          if(currAlt == prevAlt){
          run_Az("LOW", moves);
          prevAz =3200;
        } else if(currAlt > prevAlt){
          int diff_Alt = currAlt - prevAlt;
          if(prevAlt+diff_Alt <= 800){
            run_Az("LOW", moves);
            prevAz =3200;
            run_Alt("LOW", diff_Alt);
            prevAlt += diff_Alt;
          }else{
            int mod = (prevAlt+diff_Alt)%800;
            int move = diff_Alt - mod;
            run_Az("LOW", moves);
            prevAz =3200;
            run_Alt("LOW", move);
            prevAlt = 800;
          }
          }else{
          int diff_Alt =prevAlt - currAlt ;
          if(prevAlt-diff_Alt >= 0){
            run_Az("LOW", moves);
            prevAz =3200;
            run_Alt("HIGH", diff_Alt);
            prevAlt=0;
          }else{
            int mod= diff_Alt-prevAlt;
            int move = diff_Alt-mod;
            run_Az("LOW", moves);
            prevAz =3200;
            run_Alt("HIGH", move);
            prevAlt=0;
          }
        }
        }
        
    }
  } else if(command == "Azi_forward"){
    if(currAz+prevAz <=3200){
      run_Az("LOW", currAz);
      prevAz+=currAz;
    }else{
      int mod =(currAz+prevAz)%3200;
      int move = currAz-mod;
      run_Az("LOW", move);
      prevAz=3200;
    }
  }else if(command == "Azi_backward"){
    if(prevAz-currAz >=0){
      run_Az("HIGH", currAz);
      prevAz-=currAz;
    }else{
      int mod = currAz-prevAz;
      int move = currAz-mod;
      run_Az("HIGH", move);
      prevAz=0;
    }
  }else if(command == "Alt_forward"){
    if(currAlt+prevAlt <= 800){
      run_Alt("LOW",currAlt );
      prevAlt+=currAlt;
    }else{
      int mod =(currAlt+prevAlt)%800;
      int move = currAlt-mod;
      run_Alt("LOW", move);
      prevAlt=800;
    }
  }else if(command == "Alt_backward"){
    if(prevAlt-currAlt >=0){
      run_Alt("HIGH", currAlt);
      prevAlt-=currAlt;
    }else{
      int mod = currAlt-prevAlt;
      int move = currAlt-mod;
      run_Alt("HIGH", move);
      prevAlt=0;
    }
  }
}

void run_Az(String dir, int az_steps){
  uint8_t direction = (dir == "HIGH") ? HIGH : LOW;
  digitalWrite(DIR1, direction);
  Serial.println(az_steps);
  for(int i=1; i<= az_steps; i++){
     digitalWrite(PUL1, HIGH);
      delayMicroseconds(500);
      digitalWrite(PUL1, LOW);
      delayMicroseconds(500);
  }
}
void run_Alt(String dir, int alt_steps){
  uint8_t direction = (dir == "HIGH") ? HIGH : LOW;
  digitalWrite(DIR2, direction);
  Serial.println(alt_steps);
  for(int i=1; i<= alt_steps; i++){
     digitalWrite(PUL2, HIGH);
      delayMicroseconds(500);
      digitalWrite(PUL2, LOW);
      delayMicroseconds(500);
  }
}
