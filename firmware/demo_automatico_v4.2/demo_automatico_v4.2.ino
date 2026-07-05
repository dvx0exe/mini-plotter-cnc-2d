/*
 * ============================================================
 * MINI PLOTTER CNC — CALIBRAÇÃO DE EIXOS v4.2 (Interativo)
 * Modo: DEMONSTRAÇÃO AUTOMÁTICA
 * ------------------------------------------------------------
 * Desenha uma sequência de formas geométricas (quadrado,
 * círculo, estrela, cruz, espiral) uma após a outra.
 * Pressione [ENTER] no Serial Monitor para liberar cada forma,
 * ou digite um novo valor numérico para alterar o tamanho base.
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

// Move os dois motores em paralelo para posição absoluta (pos1, pos2)
void irPara(long pos1, long pos2) {
  motor1.moveTo(pos1);
  motor2.moveTo(pos2);
  while (motor1.distanceToGo() != 0 || motor2.distanceToGo() != 0) {
    motor1.run();
    motor2.run();
  }
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
  // Garante que a caneta está levantada enquanto aguarda o usuário
  canetaCima();

  Serial.println(F("--------------------------------------------------"));
  Serial.println(nomeTeste);
  Serial.print(F("Tamanho atual configurado: ")); Serial.println(tamanhoQuadrado);
  Serial.println(F("-> Pressione [ENTER] para liberar o próximo desenho."));
  Serial.println(F("-> OU digite um NOVO VALOR numérico e pressione [ENTER]."));
  Serial.println(F("--------------------------------------------------"));

  // Limpa qualquer lixo que tenha ficado no buffer serial
  while (Serial.available() > 0) { Serial.read(); }

  // Fica travado aqui até receber alguma resposta do usuário (Enter ou Valor+Enter)
  while (true) {
    if (Serial.available() > 0) {
      String entrada = Serial.readStringUntil('\n');
      entrada.trim(); // Remove espaços e caracteres ocultos de nova linha

      if (entrada.length() > 0) {
        long novoValor = entrada.toInt();
        if (novoValor > 0) {
          tamanhoQuadrado = novoValor;
          Serial.print(F(">>> Tamanho alterado com sucesso para: "));
          Serial.println(tamanhoQuadrado);
        }
      }
      break; // Quebra o loop após o comando do usuário
    }
  }
  Serial.println(F(">>> Movendo para a posição inicial..."));
}

// ─────────────────────────────────────────────────────────────
//  FUNÇÕES DE FORMAS GEOMÉTRICAS
// ─────────────────────────────────────────────────────────────

void desenharPoligono(int lados, long raio, float anguloInicialGraus = 90.0) {
  float angRad = anguloInicialGraus * (float)PI / 180.0;
  long  x0 = (long)(raio * cos(angRad));
  long  y0 = (long)(raio * sin(angRad));

  canetaCima();
  irPara(x0, y0);   // posiciona no primeiro vértice sem arrastar
  canetaBaixo();    // só abaixa após o comando de Enter e após chegar na posição inicial

  for (int i = 1; i <= lados; i++) {
    float ang = (anguloInicialGraus + (float)i * 360.0f / lados) * (float)PI / 180.0;
    long  x   = (long)(raio * cos(ang));
    long  y   = (long)(raio * sin(ang));
    irPara(x, y);
  }
}

void desenharCirculo(long raio, int segmentos = 36) {
  canetaCima();
  irPara(raio, 0);   // posiciona no inicio sem arrastar
  canetaBaixo();     // abaixa para começar o desenho

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

  irPara( b, -a); irPara( b, -b); irPara( a, -b); irPara( a,  b);
  irPara( b,  b); irPara( b,  a); irPara(-b,  a); irPara(-b,  b);
  irPara(-a,  b); irPara(-a, -b); irPara(-b, -b); irPara(-b, -a);
}

void desenharEspiral(float raioInicial, float raioFinal, int voltas, int passosPorVolta = 36) {
  int   totalPontos = voltas * passosPorVolta;

  canetaCima();
  irPara((long)raioInicial, 0);  // move sem arrastar
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
    // 1. Avisa a biblioteca que o alvo inicial é a posição levantada (40)
    servoCaneta.write(SERVO_CIMA);
    // 2. Só então liga o sinal PWM ao pino (ele já liga em 40, sem pular para 90)
    servoCaneta.attach(PINO_SERVO);
    // 3. Garante o delay de estabilização
    canetaCima();
  }

  delay(1000);

  Serial.println(F("============================================"));
  Serial.println(F("  MINI PLOTTER — CALIBRACAO DE EIXOS v4.2  "));
  Serial.println(F("  Modo Interativo Anti-Arrasto Ativado     "));
  Serial.println(F("============================================"));
  Serial.println(F("  Ponto HOME = ORIGEM (0, 0)"));
  Serial.println();

  // Variáveis que serão recalculadas a cada etapa
  long R, L;

  // ─────────────────────────────────────────────────────────
  //  Quadrado centrado na origem
  // ─────────────────────────────────────────────────────────
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

  // ─────────────────────────────────────────────────────────
  //  Círculo (36 segmentos)
  // ─────────────────────────────────────────────────────────
  aguardarUsuario("[ TESTE ] Circulo (36 segmentos)");
  R = tamanhoQuadrado / 2;
  desenharCirculo(R, 36);
  voltarOrigem();

  // ─────────────────────────────────────────────────────────
  //  Estrela de 5 pontas
  // ─────────────────────────────────────────────────────────
  aguardarUsuario("[ TESTE ] Estrela de 5 pontas");
  R = tamanhoQuadrado / 2;
  long raioExt8 = R;
  long raioInt8 = (long)(R * 0.40f);
  desenharEstrela(5, raioExt8, raioInt8);
  voltarOrigem();

  // ─────────────────────────────────────────────────────────
  //  Cruz (+)
  // ─────────────────────────────────────────────────────────
  aguardarUsuario("[ TESTE ] Cruz (+)");
  R = tamanhoQuadrado / 2;
  long bracoCruz   = R;
  long larguraCruz = R / 3;
  desenharCruz(bracoCruz, larguraCruz);
  voltarOrigem();

  // ─────────────────────────────────────────────────────────
  //  Espiral de Arquimedes (2 voltas)
  // ─────────────────────────────────────────────────────────
  aguardarUsuario("[ TESTE ] Espiral de Arquimedes (2 voltas)");
  R = tamanhoQuadrado / 2;
  float raioIni10 = (float)R / 5.0f;
  float raioFin10 = (float)R;
  desenharEspiral(raioIni10, raioFin10, 2, 36);
  voltarOrigem();

  // ── RETORNO FINAL ──────────────────────────────────────────
  retornarHome();

  // ── RESUMO ─────────────────────────────────────────
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
