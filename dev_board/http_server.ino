#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* wifiSSID = "Wifi";
const char* wifiPassword = "Password";

// Set up the web server on port 80
WebServer server(80);

void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(wifiSSID, wifiPassword);
  Serial.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Attempting to connect...");
  }

  // Once connected, print the IP address
  Serial.println("Connected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Define the behavior for the "/user" route
  server.on("/user", handleUserRoute);

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}

// Function to handle requests to "/user"
void handleUserRoute() {
  String message = "Hello, this is the /user endpoint!";
  // Send HTTP 200 response with the message
  server.send(200, "text/plain", message);
}