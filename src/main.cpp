#include <Arduino.h>
#include <painlessMesh.h>
#include <SoftwareSerial.h>

// Mesh network configuration
#define MESH_PREFIX     "ParkingMesh"
#define MESH_PASSWORD   "password123"
#define MESH_PORT       5555

// Variables
Scheduler userScheduler;
painlessMesh mesh;
std::map<String, int> sectionSpots;

// Function declarations
void sendMessage();
void receivedCallback(uint32_t from, String &msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void checkTrigger(); // Check trigger pin

void setup() {
  Serial.begin(115200);

  // Mesh network setup
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // Display errors and startup messages
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

}

void loop() {
  mesh.update();
}

// Function definitions:

// Callback for receiving messages from the mesh
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: msg=%s\n", from, msg.c_str());
  
  int index = msg.indexOf(' ');
  if (index != -1) {
    String section = msg.substring(0, index);         // Extract section name
    int spots = msg.substring(index + 1).toInt();    // Extract number of spots
    sectionSpots[section] = spots;                   
  }
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