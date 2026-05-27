#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>

// --- Configurações de Rede ---
const char* ssid = "Lucas";
const char* password = "lovecraft";
const int ledStatus = 2;

// --- Configurações do Carrinho (Roda 68mm) ---
const int hallPin = 13;
volatile int pulsos = 0;
const float diametro = 0.068; 
const float circunferencia = PI * diametro;

// --- Variáveis de Controle ---
bool isLogging = false; // Estado do Datalogger
unsigned long chartCounter = 0;

// --- Instâncias ---
AsyncWebServer server(80);
ESPDash dashboard(&server);

// --- Cards do Dashboard ---
Card speedCard(&dashboard, TEMPERATURE_CARD, "Velocidade", "km/h");
Card tempCard(&dashboard, TEMPERATURE_CARD, "Temp. Chip", "°C");
Card logStatus(&dashboard, STATUS_CARD, "Status Logger", "Pausado"); // Mostra se está gravando
Card logBtn(&dashboard, BUTTON_CARD, "Datalogger (Start/Stop)");     // Botão de controle

// --- Gráficos ---
Chart speedChart(&dashboard, BAR_CHART, "Velocidade Real-time (km/h)");

// Temperatura Interna
#ifdef __cplusplus
extern "C" { uint8_t temprature_sens_read(); }
#endif

void IRAM_ATTR countPulse() { pulsos++; }

// --- Função de Gravação Condicional ---
void logToFS(float v, float t) {
  if (!isLogging) return; // Só grava se o botão estiver em modo START

  File file = LittleFS.open("/data.csv", FILE_APPEND);
  if (file) {
    file.printf("%lu,%.2f,%.2f\n", millis(), v, t);
    file.close();
  }
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);

  pinMode(hallPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallPin), countPulse, FALLING);

  pinMode(ledStatus, OUTPUT);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    digitalWrite(ledStatus, HIGH);
    delay(250);
    digitalWrite(ledStatus, LOW);
    delay(250);
    Serial.print(".");
  }

  digitalWrite(ledStatus, HIGH);

  Serial.print("\nConectado com sucesso!");

  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print(".");}
  
  Serial.print("\nConectado! Endereço IP do ESP32: ");
  Serial.println(WiFi.localIP());

  // --- Callback do Botão ---
  logBtn.attachCallback([&](int value){
    isLogging = !isLogging; // Inverte o estado
    
    // Atualiza os cards para dar feedback visual ao usuário
    if(isLogging){
      logStatus.update("GRAVANDO", "success");
    } else {
      logStatus.update("PAUSADO", "danger");
    }
    
    dashboard.sendUpdates();
    Serial.println(isLogging ? "Datalogger Iniciado" : "Datalogger Parado");
  });

  // Rota para Download
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/data.csv", "text/csv", true);
  });

  server.begin();
}

void loop() {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 1000) {
    // Cálculo Velocidade
    detachInterrupt(digitalPinToInterrupt(hallPin));
    float rps = (float)pulsos; 
    float velocidade = (rps * circunferencia) * 3.6;
    pulsos = 0;
    attachInterrupt(digitalPinToInterrupt(hallPin), countPulse, FALLING);

    float tempC = (temprature_sens_read() - 32) / 1.8;

    // Atualiza Interface
    speedCard.update(velocidade);
    tempCard.update(tempC);
    
    String timeLabel = String(chartCounter++) + "s";
    float dadosVelocidade[1] = { velocidade };

    speedChart.updateY(dadosVelocidade, 1);
    
    dashboard.sendUpdates();

    dashboard.sendUpdates();

    // Datalogger (Só executa se isLogging for true)
    logToFS(velocidade, tempC);

    lastUpdate = millis();
  }
}