/*
 * Projekt Robot VR-Telemetria, RC - Marcin Słowik 2026
 * Projekt_AUTONIMUS - NodeMCU v3 (STA) - podsłuch RC i telemetria JSON
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// --- Dane sieci Twojej kamery ESP32-CAM ---
	const char* ssid = "Projekt_AUTONIMUS";
const char* password = "12345678"; // Sprawdź czy masz takie hasło w kamerze
ESP8266WebServer server(80);

// --- Piny i zmienne dla 2 kanałów RC ---
const int pinM = 4; // Fizyczny pin D2 (Napęd)
const int pinS = 5; // Fizyczny pin D1 (Skręt)

volatile uint32_t riseM = 0, riseS = 0;
volatile uint16_t rawM = 1500, rawS = 1500;

// Przerwania do precyzyjnego pomiaru sygnału z odbiornika RC
void ICACHE_RAM_ATTR handleM() {
  if (digitalRead(pinM) == HIGH) riseM = micros();
  else { uint32_t d = micros() - riseM; if (d > 800 && d < 2200) rawM = d; }
}

void ICACHE_RAM_ATTR handleS() {
  if (digitalRead(pinS) == HIGH) riseS = micros();
  else { uint32_t d = micros() - riseS; if (d > 800 && d < 2200) rawS = d; }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(pinM, INPUT); 
  pinMode(pinS, INPUT);
  attachInterrupt(digitalPinToInterrupt(pinM), handleM, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinS), handleS, CHANGE);

  // Łączenie z siecią kamery
  Serial.println("\nLaczenie z: " + String(ssid));
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n[OK] Polaczono z ESP32-CAM!");
  Serial.print("Adres IP NodeMCU: ");
  Serial.println(WiFi.localIP());

  // Serwer danych JSON
  server.on("/data", []() {
    // Funkcja mapująca z jawnym określeniem typu 'int' (naprawia błąd kompilacji)
    auto mapRC = [](int val, int outMin, int outMax) -> int {
      if (val > 1550) return (int)map(val, 1550, 2000, 35, outMax);
      if (val < 1450) return (int)-map(val, 1450, 1000, 35, outMax);
      return 0;
    };

    int pwmM = mapRC(rawM, 35, 150);
    int pwmS = mapRC(rawS, 35, 55);

    // Budowanie odpowiedzi JSON
    String json = "{";
    json += "\"pwmM\":" + String(pwmM) + ",";
    json += "\"pwmS\":" + String(pwmS) + ",";
    json += "\"rawM\":" + String(rawM) + ",";
    json += "\"rawS\":" + String(rawS);
    json += "}";
    
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}