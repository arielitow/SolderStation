#include "PIDControl.h"
#include "Pines.h"
#include "max6675.h"

// =================================================================
//  Variables de control
// =================================================================
double Setpoint = 20.0;
double Input    = 0.0;
double Output   = 0.0;

// Sintonizacion (default de fabrica; Persistencia.cpp la sobreescribe
// con lo guardado en NVS si existe, antes de iniciarControlPID()).
double Kp = 49.7348;
double Ki = 8.8417;
double Kd = 69.9396;

PID miPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

int dutyPWM = 0;

double tempFiltrada  = 0.0;
bool   sensorEnError = false;

MAX6675 thermocouple(pinCLK, pinCS_MAX, pinMISO);

// --- Paso de ajuste en pantalla SOLDAR ---
// Fijo por ahora; encapsulado aca para que el dia que se haga dinamico
// sea un cambio en un solo lugar.
static const double PASO_SETPOINT = 10.0;

void ajustarSetpoint(int direccion) {
  Setpoint += direccion * PASO_SETPOINT;
  if (Setpoint < 0)   Setpoint = 0;
  if (Setpoint > 600) Setpoint = 600;
}

// =================================================================
//  Autotune por oscilacion forzada (relay / Astrom-Hagglund)
// =================================================================
bool   autotuneActivo   = false;
double autotuneObjetivo = 0.0;

const double AT_HISTERESIS   = 1.0;    // grados de banda muerta al conmutar
const double AT_TECHO        = 550.0;  // limite absoluto de seguridad
static const int    AT_SALIDA_ALTA  = 255;
static const int    AT_SALIDA_BAJA  = 0;
static const int    AT_CICLOS_DESCARTAR = 1;
const int    AT_CICLOS_PROMEDIO  = 4;
static const unsigned long AT_TIMEOUT_MS = 3UL * 60UL * 1000UL;

static bool   atRelayAlto      = true;
static double atExtremoActual  = 0.0;
static double atUltimoPico     = 0.0;
static double atUltimoValle    = 0.0;
static bool   atHuboPico       = false;
static bool   atHuboValle      = false;
static unsigned long atTiempoInicioCiclo    = 0;
static unsigned long atTiempoInicioAutotune = 0;
static int    atCiclosContados  = 0;
int           atMuestrasValidas = 0;
static double atSumaPeriodos    = 0.0;
static double atSumaAmplitudes  = 0.0;

double calKpResultado = 0.0;
double calKiResultado = 0.0;
double calKdResultado = 0.0;
bool   calResultadoListo = false;

void iniciarAutotune(double objetivo) {
  autotuneObjetivo = objetivo;
  Setpoint = objetivo;

  if (miPID.GetMode() == AUTOMATIC) miPID.SetMode(MANUAL);

  atRelayAlto = (tempFiltrada < autotuneObjetivo);
  atExtremoActual = tempFiltrada;
  atHuboPico = false;
  atHuboValle = false;
  atCiclosContados = 0;
  atMuestrasValidas = 0;
  atSumaPeriodos = 0.0;
  atSumaAmplitudes = 0.0;
  atTiempoInicioCiclo = millis();
  atTiempoInicioAutotune = millis();
  calResultadoListo = false;

  autotuneActivo = true;

  Serial.println("=== AUTOTUNE INICIADO ===");
  Serial.print("Objetivo: "); Serial.println(autotuneObjetivo);
}

void abortarAutotune(const char* motivo) {
  autotuneActivo = false;
  dutyPWM = 0;
  analogWrite(pinPWM_OUT, 0);
  if (miPID.GetMode() == MANUAL) miPID.SetMode(AUTOMATIC);
  Serial.println("=== AUTOTUNE ABORTADO ===");
  Serial.println(motivo);
}

// Solo se usa dentro de este archivo (TaskControl la llama al cerrar
// el ultimo ciclo estable), por eso queda con enlace interno (static).
static void finalizarAutotune() {
  autotuneActivo = false;
  dutyPWM = 0;
  analogWrite(pinPWM_OUT, 0);
  if (miPID.GetMode() == MANUAL) miPID.SetMode(AUTOMATIC);

  double PuMs = atSumaPeriodos / atMuestrasValidas;
  double Pu   = PuMs / 1000.0;
  double amplitudPP = atSumaAmplitudes / atMuestrasValidas;
  double a = amplitudPP / 2.0;
  double d = (AT_SALIDA_ALTA - AT_SALIDA_BAJA) / 2.0;

  double Ku = (4.0 * d) / (PI * a);
  calKpResultado = 0.6 * Ku;
  calKiResultado = 1.2 * Ku / Pu;
  calKdResultado = 0.075 * Ku * Pu;
  calResultadoListo = true;

  Serial.println("=========================================");
  Serial.println("=== AUTOTUNE FINALIZADO ===");
  Serial.print("Pu (periodo, s): "); Serial.println(Pu, 3);
  Serial.print("Amplitud pico-a-pico (C): "); Serial.println(amplitudPP, 2);
  Serial.print("Ku: "); Serial.println(Ku, 4);
  Serial.println("--- Confirmar en el menu CALIBRAR (OK) para aplicar y guardar ---");
  Serial.print("Kp = "); Serial.println(calKpResultado, 4);
  Serial.print("Ki = "); Serial.println(calKiResultado, 4);
  Serial.print("Kd = "); Serial.println(calKdResultado, 4);
  Serial.println("=========================================");
}

// =================================================================
//  Inicializacion propia del modulo (pin PWM + parametros del PID)
// =================================================================
void iniciarControlPID() {
  pinMode(pinPWM_OUT, OUTPUT);
  analogWrite(pinPWM_OUT, 0);

  miPID.SetMode(AUTOMATIC);
  miPID.SetOutputLimits(0, 255);
  miPID.SetSampleTime(200);   // igual al periodo real de TaskControl
}

// =================================================================
//  Tarea de control — corre siempre, para siempre, cada 200ms exactos
// =================================================================
void TaskControl(void* pvParameters) {
  const TickType_t periodo = pdMS_TO_TICKS(200);
  TickType_t ultimoDespertar = xTaskGetTickCount();

  for (;;) {
    // --- Lectura de temperatura + filtro IIR pasabajos ---
    float tempC = thermocouple.readCelsius();
    if (isnan(tempC)) {
      sensorEnError = true;
    } else {
      tempFiltrada = (19.0 * tempFiltrada + (double)tempC) / 20.0;
      Input = tempFiltrada;
      sensorEnError = false;
    }

    if (sensorEnError) {
      // --- Modo seguro: sensor caido ---
      if (autotuneActivo) {
        abortarAutotune("Sensor de temperatura en error durante el autotune.");
      }
      if (miPID.GetMode() == AUTOMATIC) miPID.SetMode(MANUAL);
      Output = 0;
      dutyPWM = 0;
      analogWrite(pinPWM_OUT, 0);

    } else if (autotuneActivo) {
      // --- Modo autotune: oscilacion forzada (relay) ---
      if (tempFiltrada > AT_TECHO) {
        abortarAutotune("Se supero el techo de seguridad de temperatura.");
      } else if (millis() - atTiempoInicioAutotune > AT_TIMEOUT_MS) {
        abortarAutotune("Tiempo maximo de autotune excedido sin oscilacion suficiente.");
      } else {
        if (atRelayAlto) {
          if (tempFiltrada > atExtremoActual) atExtremoActual = tempFiltrada;
        } else {
          if (tempFiltrada < atExtremoActual) atExtremoActual = tempFiltrada;
        }

        bool atCambiar = false;
        if (atRelayAlto && tempFiltrada >= autotuneObjetivo + AT_HISTERESIS) {
          atCambiar = true;
        } else if (!atRelayAlto && tempFiltrada <= autotuneObjetivo - AT_HISTERESIS) {
          atCambiar = true;
        }

        if (atCambiar) {
          unsigned long ahoraAT = millis();
          if (atRelayAlto) {
            atUltimoPico = atExtremoActual;
            atHuboPico = true;
          } else {
            atUltimoValle = atExtremoActual;
            atHuboValle = true;
          }

          if (!atRelayAlto && atHuboPico && atHuboValle) {
            unsigned long periodoCiclo = ahoraAT - atTiempoInicioCiclo;
            atTiempoInicioCiclo = ahoraAT;
            atCiclosContados++;

            if (atCiclosContados > AT_CICLOS_DESCARTAR) {
              atSumaPeriodos += periodoCiclo;
              atSumaAmplitudes += (atUltimoPico - atUltimoValle);
              atMuestrasValidas++;
            }

            if (atMuestrasValidas >= AT_CICLOS_PROMEDIO) {
              finalizarAutotune();
            }
          }

          atRelayAlto = !atRelayAlto;
          atExtremoActual = tempFiltrada;
        }

        if (autotuneActivo) {
          dutyPWM = atRelayAlto ? AT_SALIDA_ALTA : AT_SALIDA_BAJA;
          analogWrite(pinPWM_OUT, dutyPWM);
        }
      }

    } else {
      // --- Modo normal: PID ---
      if (miPID.GetMode() == MANUAL) miPID.SetMode(AUTOMATIC);
      miPID.Compute();
      dutyPWM = (int)Output;
      analogWrite(pinPWM_OUT, dutyPWM);
    }

    vTaskDelayUntil(&ultimoDespertar, periodo);
  }
}
