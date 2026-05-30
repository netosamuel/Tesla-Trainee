#define RED_PIN 19
#define GREEN_PIN 18
#define BLUE_PIN 5
#define ENB 25
#define IN3 27
#define IN4 26
#define ENA 13
#define IN1 12
#define IN2 14
#define BUZZER_PIN 21
#define BUTTON_PIN 4

enum Estado {
  DESLIGADO,
  ERRO,
  INICIALIZANDO,
  VERIFICANDO,
  READY,
  DRIVING
};

Estado estadoAtual = INICIALIZANDO;

void acionaMotores() {
  Serial.println("ACIONAMENTO DOS MOTORES");

  //Acionamento:
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  // liga motor
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);
}

void setColor(bool r, bool g, bool b) {
  digitalWrite(RED_PIN, r);
  digitalWrite(GREEN_PIN, g);
  digitalWrite(BLUE_PIN, b);
}

void setup() {

  Serial.begin(115200);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Vermelho
  estadoAtual = INICIALIZANDO;
  setColor(HIGH, LOW, LOW);
  Serial.println("INICIALIZANDO");
  delay(2000);

  // Amarelo
  estadoAtual = VERIFICANDO;
  setColor(HIGH, HIGH, LOW);
  Serial.println("VERIFICANDO");
  delay(2000);

  while (estadoAtual == ERRO) {
      Serial.println("ERRO");
      setColor(HIGH, LOW, LOW);
      delay(500);

      setColor(LOW, LOW, LOW);
      delay(500);
  }
  delay(2000);

  // Verde
  estadoAtual = READY;
  setColor(LOW, HIGH, LOW);

  Serial.println("READY TO DRIVE");
}

void loop() {

  if (estadoAtual == READY) {

    if (digitalRead(BUTTON_PIN) == LOW) {

      Serial.println("BOTAO RTD APERTADO");

      tone(BUZZER_PIN, 1000);

      delay(3000);

      noTone(BUZZER_PIN);

      estadoAtual = DRIVING;

      Serial.println("CARRO LIBERADO");

      // Azul para indicar movimento
      setColor(LOW, LOW, HIGH);

      delay(500);

      acionaMotores();
    }
  }
}
