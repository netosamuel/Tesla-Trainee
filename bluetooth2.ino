#include "BluetoothSerial.h"


#include "LittleFS.h"

// Declaração da função nativa do ESP32 para temperatura interna
#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

// --- Configurações de Pinos ---
const int PIN_HALL = 13;       // Pino do sensor de Hall KY-003
const int PIN_LED_BT = 2;      // LED azul nativo do ESP32

// --- Constantes do Carro ---
// Diâmetro = 68mm -> Raio = 34mm -> 0.034 metros
const float RAIO_RODA = 0.034;  
const float PI_VAL = 3.141592;

// --- Variáveis Globais ---
BluetoothSerial SerialBT;
volatile unsigned long impulsos_hall = 0;
unsigned long tempo_anterior = 0;
const unsigned long INTERVALO_LEITURA = 1000; // 1 segundo

// Controle de Estado do Programa
bool coletando = false; 

// Nome do arquivo de log
const char* LOG_FILE = "/datalogger.txt";

// --- Função de Interrupção (ISR) ---
void IRAM_ATTR contarImpulso() {
  if (coletando) {
    impulsos_hall++;
  }
}

// --- Função para ler a temperatura interna em Celsius ---
float lerTemperaturaInterna() {
  return (temprature_sens_read() - 32) / 1.8;
}

// --- Inicializa ou limpa o Datalogger ---
void resetarDatalogger() {
  // Abre o arquivo de forma segura
  File file = LittleFS.open(LOG_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("Erro ao abrir arquivo de log.");
    return;
  }
  file.println("Tempo(ms),Temp_Celsius,Velocidade_KmH"); // Cabeçalho CSV
  file.close();
  
  Serial.println("Datalogger resetado!");
  if (SerialBT.connected()) {
    SerialBT.println(">>> Datalogger resetado com sucesso! <<<");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500); // Pequena pausa para estabilizar o Serial Monitor USB
  Serial.println("\n--- Inicializando Sistema ---");
  
  // 1. Configuração dos Pinos e Interrupção
  pinMode(PIN_HALL, INPUT_PULLUP);
  pinMode(PIN_LED_BT, OUTPUT);
  digitalWrite(PIN_LED_BT, LOW);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), contarImpulso, FALLING);

  // 2. Inicializa o Sistema de Arquivos LittleFS primeiro
  if (!LittleFS.begin(true)) {
    Serial.println("Erro crítico ao iniciar o LittleFS.");
    while (1); // Trava aqui se houver defeito físico na memória flash
  }
  Serial.println("LittleFS iniciado com sucesso.");

  // Se o arquivo não existir, cria ele sem mandar dados pro BT ainda
  if (!LittleFS.exists(LOG_FILE)) {
    File file = LittleFS.open(LOG_FILE, FILE_WRITE);
    if (file) {
      file.println("Tempo(ms),Temp_Celsius,Velocidade_KmH");
      file.close();
    }
  }

  // 3. Pequena pausa de segurança antes de ligar o rádio Bluetooth
  delay(300); 

  // 4. Inicializa o Bluetooth de forma isolada
  if (!SerialBT.begin("Telemetria_ESP32")) {
    Serial.println("Erro ao iniciar o Bluetooth!");
    while (1);
  }
  
  Serial.println("Bluetooth ativado! Pronto para emparelhar.");
}

void loop() {
  // 1. Controle do LED de Status do Bluetooth
  if (SerialBT.connected()) {
    digitalWrite(PIN_LED_BT, HIGH); 
  } else {
    digitalWrite(PIN_LED_BT, LOW);  
  }

  // 2. Verificação de Comandos via Bluetooth
  if (SerialBT.available()) {
    char comando = SerialBT.read();
    
    if (comando == 's' || comando == 'S') {
      coletando = true;
      tempo_anterior = millis(); 
      Serial.println("Coleta INICIADA.");
      if (SerialBT.connected()) SerialBT.println(">>> Monitoramento Iniciado <<<");
    } 
    else if (comando == 'p' || comando == 'P') {
      coletando = false;
      Serial.println("Coleta PAUSADA.");
      if (SerialBT.connected()) SerialBT.println(">>> Monitoramento Pausado <<<");
    } 
    else if (comando == 'r' || comando == 'R') {
      resetarDatalogger();
    }
  }

  // 3. Execução da Coleta (Apenas se 'coletando' for verdadeiro)
  if (coletando) {
    unsigned long tempo_atual = millis();
    
    if (tempo_atual - tempo_anterior >= INTERVALO_LEITURA) {
      
      noInterrupts();
      unsigned long contagem = impulsos_hall;
      impulsos_hall = 0; 
      interrupts();

      // Cálculo da Velocidade (Diâmetro = 68mm -> Raio = 0.034m)
      float distancia_por_volta = 2.0 * PI_VAL * RAIO_RODA;
      float velocidade_ms = (contagem * distancia_por_volta) / ((tempo_atual - tempo_anterior) / 1000.0);
      float velocidade_kmh = velocidade_ms * 3.6;

      float temp_celsius = lerTemperaturaInterna();
      tempo_anterior = tempo_atual;

      // Formatação dos Dados
      String dadosCSV = String(tempo_atual) + "," + String(temp_celsius, 1) + "," + String(velocidade_kmh, 1);

      // 4. Salva na Memória Flash
      File file = LittleFS.open(LOG_FILE, FILE_APPEND);
      if (file) {
        file.println(dadosCSV);
        file.close();
      }

      // 5. Envia para o Dashboard via Bluetooth
      if (SerialBT.connected()) {
        SerialBT.print("Temperatura_C:");
        SerialBT.print(temp_celsius, 1);
        SerialBT.print(",");
        SerialBT.print("Velocidade_KmH:");
        SerialBT.println(velocidade_kmh, 1);
      }

      // Cópia no USB para Debug
      Serial.println(dadosCSV);
    }
  }
}