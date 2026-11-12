#pragma once
#include <Arduino.h>
#include <PID_v1.h>

// =================================================================
//  PIDControl — todo lo relativo a temperatura, PID y autotune.
//  No sabe nada de pantallas ni del encoder: solo controla el
//  calefactor. Corre en su propia tarea (TaskControl), siempre,
//  independiente de que pantalla del menu este activa.
// =================================================================

// --- Variables de control (leidas/editadas tambien desde Menu.cpp) ---
extern double Setpoint;
extern double Input;
extern double Output;
extern double Kp;
extern double Ki;
extern double Kd;
extern PID    miPID;
extern int    dutyPWM;

extern double tempFiltrada;
extern bool   sensorEnError;

// --- Estado del autotune (leido por la pantalla CALIBRAR) ---
extern bool         autotuneActivo;
extern double       autotuneObjetivo;
extern const double AT_HISTERESIS;
extern const double AT_TECHO;
extern const int    AT_CICLOS_PROMEDIO;
extern int          atMuestrasValidas;

// Resultado del autotune a la espera de confirmacion (no se aplica a
// Kp/Ki/Kd hasta que algo llame a aplicar+guardar).
extern double calKpResultado;
extern double calKiResultado;
extern double calKdResultado;
extern bool   calResultadoListo;

// --- API publica ---
void ajustarSetpoint(int direccion);      // +-1 click = +-PASO_SETPOINT grados
void iniciarAutotune(double objetivo);
void abortarAutotune(const char* motivo);

void iniciarControlPID();                 // pin PWM + PID mode/limits/sampletime
void TaskControl(void* pvParameters);     // tarea FreeRTOS del lazo de control
