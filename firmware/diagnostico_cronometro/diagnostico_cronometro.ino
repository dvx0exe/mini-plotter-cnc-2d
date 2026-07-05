/*
 * ============================================================
 * MINI PLOTTER CNC — CALIBRAÇÃO DE EIXOS v4.2 (Diagnóstico)
 * Modo: DIAGNÓSTICO / CRONÔMETRO
 * ------------------------------------------------------------
 * Executa a MESMA sequência de formas do sketch de demonstração,
 * mas cada chamada de irPara() é cronometrada e o tempo real de
 * execução de cada segmento é impresso no Serial Monitor.
 *
 * Objetivo: tornar mensurável, em tempo real, o comportamento de
 * perda de passos do motor 28BYJ-48 em malha aberta (sem encoder,
 * sem feedback de posição) — o motor recebe o pulso e o firmware
 * assume que ele obedeceu, mesmo quando isso não acontece.
 * ============================================================
 */

#include <AccelStepper.h>
#include <Servo.h>
#include <math.h>

// ── Motores ─────────────────────────────────────────────────
AccelStepper motor1(AccelStepper::HALF4WIRE, 8, 10, 9, 11);
AccelStepper motor2(AccelStepper::HALF4WIRE, 4,  6, 5,  7);

// ── Servo da caneta ─────────────────────────────────────────
Servo servoCaneta;
const int PINO_SERVO  = 3;    // altere se o servo estiver em outro pino
const int SERVO_CIMA  = 40;   // caneta levantada — ajuste conforme hardware
const int SERVO_BAIXO = 80;   // caneta abaixada  — ajuste conforme hardware
bool temServo = true;         // mude para false se ainda não tiver servo

// ── Velocidade dos motores ──────────────────────────────────
const float VEL = 300.0;      // passos/segundo  (reduza se perder passos)
const float ACC = 300.0;      // passos/segundo²

// ── TAMANHO BASE INICIAL ────────────────────────────────────
long tamanhoQuadrado = 9000;

// ─────────────────────────────────────────────────────────────
//  FUNÇÕES AUXILIARES DE MOVIMENTO E CANETA
// ─────────────────────────────────────────────────────────────

void canetaCima() {
  if (!temServo) return;
  servoCaneta.write(SERVO_CIMA);
  delay(300); // Tempo físico para o servo levantar totalmente
}

void canetaBaixo() {
  if (!temServo) return;
  servoCaneta.write(SERVO_BAIXO);
  delay(300); // Tempo para a caneta estabilizar no papel
}

// Move os dois motores em paralelo para posição absoluta (pos1, pos2) com CRONÔMETRO
void irPara(long pos1, long pos2) {
  // Se os motores já estiverem na posição de destino, não faz nada e não conta tempo
  if (motor1.currentPosition() == pos1 && motor2.currentPosition() == pos2) {
    return;
  }

  // CRONÔMETRO: guarda o tempo exato em milissegundos antes de começar a andar
  unsigned long tempoInicio = millis();

  motor1.moveTo(pos1);
  motor2.moveTo(pos2);

  // Executa o movimento até os motores pararem no destino
  while (motor1.distanceToGo() != 0 || motor2.distanceToGo() != 0) {
    motor1.run();
    motor2.run();
  }

  // CRONÔMETRO: calcula a diferença de tempo ao parar
  unsigned long tempoTotalMs = millis() - tempoInicio;
  float tempoSegundos = (float)tempoTotalMs / 1000.0; // Converte para segundos

  // Imprime o resultado no terminal (mantendo histórico na tela)
  Serial.print(F("  [Cronômetro] Movimento para ("));
  Serial.print(pos1);
  Serial.print(F(", "));
  Serial.print(pos2);
  Serial.print(F(") concluído em: "));
  Serial.print(tempoSegundos, 3); // Exibe com 3 casas decimais
  Serial.println(F(" segundos."));
}

// Levanta a caneta, retorna à origem e imprime confirmação
void voltarOrigem() {
  canetaCima(); // Garante que levanta antes de voltar
  irPara(0, 0);
  Serial.println(F("  [ok] Voltou a origem (0,0)"));
  Serial.println();
}

// Retorno FINAL: sobe caneta → vai a (0,0) → desativa tudo
void retornarHome() {
  Serial.println();
  Serial.println(F("  =============================="));
  Serial.println(F("  >> RETORNANDO AO HOME FINAL..."));
  Serial.println(F("  =============================="));
  canetaCima();
  irPara(0, 0);
  motor1.disableOutputs();
  motor2.disableOutputs();
  if (temServo) servoCaneta.detach();
  Serial.println(F("  [HOME] Posicao inicial (0,0) atingida!"));
  Serial.println(F("  [HOME] Motores e servo desativados."));
  Serial.println(F("  =============================="));
  Serial.println();
}

// ─────────────────────────────────────────────────────────────
//  FUNÇÃO DE INTERATIVIDADE COM O USUÁRIO
// ─────────────────────────────────────────────────────────────
void aguardarUsuario(String nomeTeste) {
  canetaCima();
  Serial.println(F("--------------------------------------------------"));
  Serial.println(nomeTeste);
  Serial.print(F("Tamanho atual configurado: ")); Serial.println(tamanhoQuadrado);
  Serial.println(F("-> Pressione [ENTER] para liberar o próximo desenho."));
  Serial.println(F("-> OU digite um NOVO VALOR numérico e pressione [ENTER]."));
  Serial.println(F("--------------------------------------------------"));

  while (Serial.available() > 0) { Serial.read(); }

  while (true) {
    if (Serial.available() > 0) {
      String entrada = Serial.readStringUntil('\n');
      entrada.trim();

      if (entrada.length() > 0) {
        long novoValor = entrada.toInt();
        if (novoValor > 0) {
          tamanhoQuadrado = novoValor;
          Serial.print(F(">>> Tamanho alterado com sucesso para: "));
          Serial.println(tamanhoQuadrado);
        }
      }
      break;
    }
  }
}

// ─────────────────────────────────────────────────────────────
//  FUNÇÕES DE FORMAS GEOMÉTRICAS
// ─────────────────────────────────────────────────────────────

void desenharPoligono(int lados, long raio, float anguloInicialGraus = 90.0) {
  float angRad = anguloInicialGraus * (float)PI / 180.0;
  long  x0 = (long)(raio * cos(angRad));
  long  y0 = (long)(raio * sin(angRad));

  canetaCima();
  irPara(x0, y0);
  canetaBaixo();

  for (int i = 1; i <= lados; i++) {
    float ang = (anguloInicialGraus + (float)i * 360.0f / lados) * (float)PI / 180.0;
    long  x   = (long)(raio * cos(ang));
    long  y   = (long)(raio * sin(ang));
    irPara(x, y);
  }
}

void desenharCirculo(long raio, int segmentos = 36) {
  canetaCima();
  irPara(raio, 0);
  canetaBaixo();

  for (int i = 1; i <= segmentos; i++) {
    float ang = (float)i * 2.0f * (float)PI / segmentos;
    long  x   = (long)(raio * cos(ang));
    long  y   = (long)(raio * sin(ang));
    irPara(x, y);
  }
}

void desenharEstrela(int pontas, long raioExt, long raioInt) {
  float angOff = 90.0f * (float)PI / 180.0f;
  float passo  = (float)PI / pontas;

  long x0 = (long)(raioExt * cos(angOff));
  long y0 = (long)(raioExt * sin(angOff));

  canetaCima();
  irPara(x0, y0);
  canetaBaixo();

  for (int i = 0; i < pontas; i++) {
    float angIn = angOff + passo * (2 * i + 1);
    irPara((long)(raioInt * cos(angIn)),  (long)(raioInt * sin(angIn)));
    float angOut = angOff + passo * (2 * i + 2);
    irPara((long)(raioExt * cos(angOut)), (long)(raioExt * sin(angOut)));
  }
}

void desenharCruz(long braco, long largura) {
  long b = largura / 2;
  long a = braco;

  canetaCima();
  irPara(-b, -a);
  canetaBaixo();

  irPara( b, -a); irPara( b, -b); irPara( a, -b);
  irPara( a,  b);
  irPara( b,  b); irPara( b,  a); irPara(-b,  a); irPara(-b,  b);
  irPara(-a,  b); irPara(-a, -b); irPara(-b, -b); irPara(-b, -a);
}

void desenharEspiral(float raioInicial, float raioFinal, int voltas, int passosPorVolta = 36) {
  int   totalPontos = voltas * passosPorVolta;
  canetaCima();
  irPara((long)raioInicial, 0);
  canetaBaixo();

  for (int i = 1; i <= totalPontos; i++) {
    float ang  = (float)i * 2.0f * (float)PI / passosPorVolta;
    float raio = raioInicial + (raioFinal - raioInicial) * (float)i / totalPontos;
    irPara((long)(raio * cos(ang)), (long)(raio * sin(ang)));
  }
}

// ─────────────────────────────────────────────────────────────
//  SETUP — todos os testes acontecem aqui
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  motor1.setMaxSpeed(VEL);   motor1.setAcceleration(ACC);
  motor2.setMaxSpeed(VEL);   motor2.setAcceleration(ACC);
  motor1.setCurrentPosition(0);
  motor2.setCurrentPosition(0);

  if (temServo) {
    servoCaneta.write(SERVO_CIMA);
    servoCaneta.attach(PINO_SERVO);
    canetaCima();
  }

  delay(1000);

  Serial.println(F("============================================"));
  Serial.println(F("  MINI PLOTTER — CALIBRACAO DE EIXOS v4.2  "));
  Serial.println(F("  Modo Diagnostico / Cronometro Ativado    "));
  Serial.println(F("============================================"));
  Serial.println(F("  Ponto HOME = ORIGEM (0, 0)"));
  Serial.println();

  long R, L;

  aguardarUsuario("[ TESTE ] Quadrado centrado na origem");
  L = tamanhoQuadrado / 2;
  canetaCima();
  irPara(-L, -L);
  canetaBaixo();
  irPara(+L, -L);
  irPara(+L, +L);
  irPara(-L, +L);
  irPara(-L, -L - 3500);
  voltarOrigem();

  aguardarUsuario("[ TESTE ] Circulo (36 segmentos)");
  R = tamanhoQuadrado / 2;
  desenharCirculo(R, 36);
  voltarOrigem();

  aguardarUsuario("[ TESTE ] Estrela de 5 pontas");
  R = tamanhoQuadrado / 2;
  long raioExt8 = R;
  long raioInt8 = (long)(R * 0.40f);
  desenharEstrela(5, raioExt8, raioInt8);
  voltarOrigem();

  aguardarUsuario("[ TESTE ] Cruz (+)");
  R = tamanhoQuadrado / 2;
  long bracoCruz   = R;
  long larguraCruz = R / 3;
  desenharCruz(bracoCruz, larguraCruz);
  voltarOrigem();

  aguardarUsuario("[ TESTE ] Espiral de Arquimedes (2 voltas)");
  R = tamanhoQuadrado / 2;
  float raioIni10 = (float)R / 5.0f;
  float raioFin10 = (float)R;
  desenharEspiral(raioIni10, raioFin10, 2, 36);
  voltarOrigem();

  retornarHome();

  Serial.println(F("============================================"));
  Serial.println(F("  TESTES CONCLUIDOS! (5 formas)             "));
  Serial.println(F("============================================"));
}

// ─────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────
void loop() {
  motor1.moveTo(0);
  motor2.moveTo(0);
  motor1.run();
  motor2.run();
}
