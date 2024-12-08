#include <Arduino.h>
#include <painlessMesh.h>
#include <SoftwareSerial.h>

// Defines
// Mesh network configuration
#define MESH_PREFIX     "ParkingMesh"
#define MESH_PASSWORD   "password123"
#define MESH_PORT       5555

// Trigger pin (input from Arduino)
#define TRIGGER_PIN 5

// Virtual serial RX and TX pins
#define RX_PIN 12  // Virtual RX
#define TX_PIN 14  // Virtual TX

// Variables
Scheduler userScheduler;
painlessMesh mesh;
// Struct to store received data
struct SectionData {
  int freeSpots = -1;
  int entranceScore = 0;
  int elevatorScore = 0;
};
std::map<String, SectionData> sectionSpots;
SoftwareSerial softSerial(RX_PIN, TX_PIN);

// Function declarations
void sendDataToDisplay();
void receivedCallback(uint32_t from, String &msg);
void checkTrigger();
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// Task for monitoring the trigger pin and sending data
Task taskCheckTrigger(TASK_IMMEDIATE, TASK_FOREVER, &checkTrigger);

void setup() {
  Serial.begin(115200);       // Main serial port for debugging
  softSerial.begin(9600);     // Virtual serial port for Arduino communication

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // Mesh network setup
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // Display errors and startup messages
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Add and enable the trigger checking task
  userScheduler.addTask(taskCheckTrigger);
  taskCheckTrigger.enable();
}

void loop() {
  mesh.update();
  userScheduler.execute();
}

// Function definitions:
// Send parking spot data to Arduino
void sendDataToDisplay() {
  if (!sectionSpots.empty()) {
    for (auto const& section : sectionSpots) {
      String message = section.first + " " + String(section.second.freeSpots) + " " + String(section.second.entranceScore) + " " + String(section.second.elevatorScore); 
      softSerial.println(message);                                  
      Serial.println("Sent to Arduino: " + message);               
    }
    softSerial.println("END");  // Indicate the end of data
    sectionSpots.clear();
  } else {
    softSerial.println("NO");  // Send message if no data is available
  }
}
// Callback for receiving messages from the mesh
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());

  unsigned int index = 0;
  while (index < msg.length()) {
    int spaceIdx = msg.indexOf(' ', index);
    if (spaceIdx == -1) break;

    String section = msg.substring(index, spaceIdx);
    index = spaceIdx + 1;

    spaceIdx = msg.indexOf(' ', index);
    int spots = msg.substring(index, spaceIdx).toInt();
    index = spaceIdx + 1;

    spaceIdx = msg.indexOf(' ', index);
    int entranceScore = msg.substring(index, spaceIdx).toInt();
    index = spaceIdx + 1;
    
    // read entrance Score
    // Note: The End condition is for last score -> (spaceIdx == -1)
    spaceIdx = msg.indexOf(' ', index); 
    int elevatorScore = (spaceIdx == -1) ? msg.substring(index).toInt() : msg.substring(index, spaceIdx).toInt();
    index = (spaceIdx == -1) ? msg.length() : spaceIdx + 1;

    sectionSpots[section] = {spots, entranceScore, elevatorScore};
    Serial.printf("Updated section %s: spots=%d, entrance=%d, elevator=%d\n", section.c_str(), spots, entranceScore, elevatorScore);
  }
}
// Task function to monitor the trigger pin
void checkTrigger() {
  static bool lastState = HIGH;           
  bool currentState = digitalRead(TRIGGER_PIN);

  if(currentState == LOW && lastState == HIGH){ // Trigger activated
    sendDataToDisplay();
  }

  lastState = currentState;
}
// Callback for new connections
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection established, nodeId = %u\n", nodeId);
}
// Callback for changed connections
void changedConnectionCallback() {
  Serial.println("Connections updated");
}
// Callback for adjusted node time
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Node time adjusted. Offset = %d\n", offset);
}