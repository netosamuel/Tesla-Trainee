#define RED_PIN 27
#define GREEN_PIN 26
#define BLUE_PIN 25

#define BUZZER_PIN 23
#define BUTTON_PIN 21

enum Estado {
  DESLIGADO,
  ERRO,
  INICIALIZANDO,
  READY,
  DRIVING
};

Estado estadoAtual = INICIALIZANDO;
bool Erro;

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

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Vermelho
  setColor(HIGH, LOW, LOW);

  delay(2000);

  // Amarelo
  estadoAtual = INICIALIZANDO;
  setColor(HIGH, HIGH, LOW);
  Erro = true; //condições a serem recebidas

  if (Erro) {
    while (true) {
      setColor(HIGH, LOW, LOW);
      delay(500);

      setColor(LOW, LOW, LOW);
      delay(500);
    }
  }
  else {

    delay(2000);

    // Verde
    estadoAtual = READY;
    setColor(LOW, HIGH, LOW);

    Serial.println("READY TO DRIVE");
  }
}

void loop() {

  if (estadoAtual == READY) {

    if (digitalRead(BUTTON_PIN) == LOW) {

      Serial.println("RTD BUTTON PRESSED");

      tone(BUZZER_PIN, 1000);

      delay(3000);

      noTone(BUZZER_PIN);

      estadoAtual = DRIVING;

      Serial.println("CARRO LIBERADO");

      // Azul para indicar movimento
      setColor(LOW, LOW, HIGH);

      delay(500);
    }
  }
}