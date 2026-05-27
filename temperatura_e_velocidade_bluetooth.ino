#include <Arduino.h>
#include <LittleFS.h>
#include "BluetoothSerial.h"

// Verifica se o Bluetooth está ativado corretamente no core do ESP32
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// --- Configurações do Bluetooth ---
BluetoothSerial ESP_BT; 
const char* bluetoothName = "ESP32_Carrinho"; // Nome que aparecerá no pareamento

const int ledStatus = 2;

// --- Configurações do Carrinho (Roda 68mm) ---
const int hallPin = 13;
volatile int pulsos = 0;
const float diametro = 0.068; 
const float circunferencia = PI * diametro;

// --- Variáveis de Controle ---
bool isLogging = false; // Estado do Datalogger

// --- Temperatura Interna ---
#ifdef __cplusplus
extern "C" { uint8_t temprature_sens_read(); }
#endif

// --- Interrupção do Sensor Hall ---
void IRAM_ATTR countPulse() { 
  pulsos++;
}

// --- Função de Gravação no LittleFS ---
void logToFS(float v, float t) {
  if (!isLogging) return; // Só grava se o logger estiver ativo

  File file = LittleFS.open("/data.csv", FILE_APPEND);
  if (file) {
    file.printf("%lu,%.2f,%.2f\n", millis(), v, t);
    file.close();
  }
}

// --- Função para Enviar o Arquivo CSV via Bluetooth ---
void enviarArquivoCSV() {
  if (!LittleFS.exists("/data.csv")) {
    ESP_BT.println("\n--- Erro: Arquivo data.csv nao existe ainda. ---");
    return;
  }

  // Envia marcadores claros para você saber onde começa e termina o arquivo
  ESP_BT.println("\n=== INICIO DO ARQUIVO CSV ===");
  ESP_BT.println("Tempo(ms),Velocidade(km/h),Temperatura(C)");
  
  File file = LittleFS.open("/data.csv", FILE_READ);
  if (file) {
    while (file.available()) {
      ESP_BT.write(file.read()); 
    }
    file.close();
    ESP_BT.println("=== FIM DO ARQUIVO CSV ===\n");
  } else {
    ESP_BT.println("\n--- Erro ao abrir o arquivo para leitura. ---");
  }
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);

  pinMode(hallPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(hallPin), countPulse, FALLING);

  pinMode(ledStatus, OUTPUT);

  // Inicializa o Bluetooth
  ESP_BT.begin(bluetoothName); 
  Serial.printf("Bluetooth inicializado! Procure por: %s\n", bluetoothName);
  
  // Pisca o LED para indicar que o sistema está pronto
  digitalWrite(ledStatus, HIGH);
  delay(300);
  digitalWrite(ledStatus, LOW);
}

void loop() {
  // --- Processamento de Comandos via Bluetooth ---
  if (ESP_BT.available()) {
    char comando = ESP_BT.read();
    
    // Comando 's' ou 'S' -> Liga/Desliga o Datalogger
    if (comando == 's' || comando == 'S') {
      isLogging = !isLogging;
      if (isLogging) {
        digitalWrite(ledStatus, HIGH); // LED aceso fixo = Gravando dados
      } else {
        digitalWrite(ledStatus, LOW);  // LED apagado = Pausado
      }
    }
    
    // Comando 'd' ou 'D' -> Faz o "Download" dos dados salvos
    if (comando == 'd' || comando == 'D') {
      bool estadoAnterior = isLogging;
      isLogging = false; // Pausa a gravação temporariamente para evitar conflitos no LittleFS
      
      enviarArquivoCSV();
      
      isLogging = estadoAnterior; // Retorna ao estado anterior
    }
  }

  // --- Cálculo e Envio de Dados (A cada 1 segundo) ---
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    
    // Cálculo da Velocidade
    detachInterrupt(digitalPinToInterrupt(hallPin));
    float rps = (float)pulsos; 
    float velocidade = (rps * circunferencia) * 3.6;
    pulsos = 0;
    attachInterrupt(digitalPinToInterrupt(hallPin), countPulse, FALLING);

    // Leitura da Temperatura Interna do ESP32
    float tempC = (temprature_sens_read() - 32) / 1.8;

    // --- TRANSMISSÃO DOS DADOS EM FORMATO DE GRÁFICO (CSV SIMPLIFICADO) ---
    // Envia "Velocidade,Temperatura" -> Exemplo: 12.50,38.2
    // Desse modo, o Serial Plotter do Arduino ou Apps geram as linhas em tempo real.
    ESP_BT.print(velocidade);
    ESP_BT.print(",");
    ESP_BT.println(tempC);

    // Envia também para o cabo USB (Monitor Serial da bancada)
    Serial.print(velocidade);
    Serial.print(",");
    Serial.println(tempC);

    // Grava no arquivo interno do LittleFS (Se o logger estiver ativo)
    logToFS(velocidade, tempC);

    lastUpdate = millis();
  }
}