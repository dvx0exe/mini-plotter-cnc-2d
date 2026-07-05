/*
 * ============================================================
 * MINI PLOTTER CNC — CONTROLE MANUAL POR TECLADO
 * ============================================================
 * Q = Motor 1 FRENTE      A = Motor 1 TRÁS
 * W = Motor 2 FRENTE      S = Motor 2 TRÁS
 * X ou ESPAÇO = PARA TUDO
 * ============================================================
 */

#include <AccelStepper.h>

// ── Motores ─────────────────────────────────────────────────
AccelStepper motor1(AccelStepper::HALF4WIRE, 8, 10, 9, 11);
AccelStepper motor2(AccelStepper::HALF4WIRE, 4,  6, 5,  7);

// ── Velocidade (passos/segundo) ────────────────────────────
const float VEL_MOTOR1 = 800.0;
const float VEL_MOTOR2 = 800.0;

// ── Direção atual: -1 = trás, 0 = parado, 1 = frente ───────
int dirMotor1 = 0;
int dirMotor2 = 0;

void mostrarMenu() {
  Serial.println(F("=================================================="));
  Serial.println(F("  CONTROLE MANUAL DOS MOTORES - TECLADO"));
  Serial.println(F("=================================================="));
  Serial.println(F("  Q = Motor 1 FRENTE      A = Motor 1 TRAS"));
  Serial.println(F("  W = Motor 2 FRENTE      S = Motor 2 TRAS"));
  Serial.println(F("  X ou ESPACO = PARAR TUDO"));
  Serial.println(F("=================================================="));
}

void pararTudo() {
  dirMotor1 = 0;
  dirMotor2 = 0;
  motor1.setSpeed(0);
  motor2.setSpeed(0);
  motor1.stop();
  motor2.stop();
  Serial.println(F("[STOP] Motores parados."));
}

void processarTecla(char tecla) {
  switch (tecla) {
    case 'q': case 'Q':
      dirMotor1 = 1;
      Serial.println(F("[Motor 1] FRENTE"));
      break;
    case 'a': case 'A':
      dirMotor1 = -1;
      Serial.println(F("[Motor 1] TRAS"));
      break;
    case 'w': case 'W':
      dirMotor2 = 1;
      Serial.println(F("[Motor 2] FRENTE"));
      break;
    case 's': case 'S':
      dirMotor2 = -1;
      Serial.println(F("[Motor 2] TRAS"));
      break;
    case 'x': case 'X': case ' ':
      pararTudo();
      break;
    default:
      // ignora Enter, quebras de linha e teclas desconhecidas
      break;
  }
}

void setup() {
  Serial.begin(9600);

  motor1.setMaxSpeed(VEL_MOTOR1);
  motor2.setMaxSpeed(VEL_MOTOR2);
  motor1.setCurrentPosition(0);
  motor2.setCurrentPosition(0);

  delay(500);
  mostrarMenu();
}

void loop() {
  // Lê tecla a cada caractere recebido (não precisa esperar Enter)
  if (Serial.available() > 0) {
    char tecla = Serial.read();
    processarTecla(tecla);
  }

  // Aplica a direção atual continuamente
  motor1.setSpeed(dirMotor1 * VEL_MOTOR1);
  motor2.setSpeed(dirMotor2 * VEL_MOTOR2);

  motor1.runSpeed();
  motor2.runSpeed();
}
