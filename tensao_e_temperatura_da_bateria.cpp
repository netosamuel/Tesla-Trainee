#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <LittleFS.h>
#include <math.h>

// --- Configurações de Rede ---
const char* ssid = "Formula_Tesla_Data";
const char* password = "senhadoprojeto";

AsyncWebServer server(80);
ESPDash dashboard(&server); 

// --- Cards do Dashboard ---
Card batGauge(&dashboard, GAUGE_CARD, "Tensão Bateria", "V", 0, 9);
Card tempGauge(&dashboard, GAUGE_CARD, "Temperatura", "°C", 0, 100);
Card statusCard(&dashboard, STATUS_CARD, "Segurança", "success");
Card logAction(&dashboard, BUTTON_CARD, "Limpar Histórico (CSV)");

// --- Parâmetros de Hardware (Baseados no Relatório) ---
#define ADC_BAT_PIN 34  
#define ADC_TEMP_PIN 35 

// Divisor de Tensão: R1=10k, R2=4.7k [cite: 82, 97]
const float R1 = 10000.0; 
const float R2 = 4700.0;  
const float DIVIDER_RATIO = (R1 + R2) / R2; 

// Termistor NTC 10k [cite: 110, 129]
const float BETA = 3950.0; 
const float R_FIXO = 10000.0; // Rfixo 10k [cite: 126]

unsigned long lastLogTime = 0;
const int LOG_INTERVAL = 5000; // Log a cada 5 segundos

// --- Funções de Arquivo ---
void setupLogFile() {
  if (!LittleFS.exists("/log.csv")) {
    File file = LittleFS.open("/log.csv", FILE_WRITE);
    file.println("Tempo(ms),Tensao(V),Temperatura(C)");
    file.close();
  }
}

void setup() {
  Serial.begin(115200);

  // Inicializa Sistema de Arquivos
  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao montar LittleFS");
    return;
  }
  setupLogFile();

  WiFi.softAP(ssid, password);
  analogSetAttenuation(ADC_11db); // Leitura até ~3.1V [cite: 98]

  // Rota para Baixar o Log: 192.168.4.1/download
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/log.csv", "text/csv", true);
  });

  // Callback do Botão Limpar Log
  logAction.attachCallback([](bool value){
    LittleFS.remove("/log.csv");
    setupLogFile();
    Serial.println("Arquivo de log resetado pelo usuário.");
  });

  server.begin();
}

void loop() {
  // 1. Leitura de Tensão [cite: 101, 102]
  int rawBat = analogRead(ADC_BAT_PIN);
  float vBat = ((rawBat / 4095.0) * 3.3) * DIVIDER_RATIO;

  // 2. Leitura de Temperatura (NTC) [cite: 123, 133]
  int rawTemp = analogRead(ADC_TEMP_PIN);
  float vOutTemp = (rawTemp / 4095.0) * 3.3;
  float rNtc = R_FIXO * (vOutTemp / (3.3 - vOutTemp));
  float tempC = 1.0 / (log(rNtc / 10000.0) / BETA + 1.0 / (25.0 + 273.15)) - 273.15;

  // 3. Lógica de Segurança conforme o regulamento
  if (vBat < 6.0) { // Limite Subtensão [cite: 80, 107]
    statusCard.update("SUBTENSÃO!", "danger");
  } else if (tempC > 60.0) { // Limite Temperatura [cite: 109]
    statusCard.update("SUPERAQUECIMENTO!", "danger");
  } else {
    statusCard.update("SISTEMA NOMINAL", "success");
  }

  // 4. Atualiza Dashboard
  batGauge.update(vBat);
  tempGauge.update(tempC);
  dashboard.sendUpdates();

  // 5. Gravação de Dados Periódica
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    File file = LittleFS.open("/log.csv", FILE_APPEND);
    if (file) {
      file.printf("%lu, %.2f, %.1f\n", millis(), vBat, tempC);
      file.close();
    }
    lastLogTime = millis();
  }
}