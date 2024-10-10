#include <WiFi.h>

const char* wifiSSID = "Wifi";
const char* wifiPassword = "Password";

void setup() {
    Serial.begin(115200);
    
    // Conectar a la red WiFi
    WiFi.begin(wifiSSID, wifiPassword);
    
    Serial.print("Conectando a la red WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("Conectado a la red WiFi");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

void loop() {
}