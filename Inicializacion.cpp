#include "Inicializacion.h"
#include "Pines.h"
#include "PIDControl.h"
#include "Persistencia.h"
#include "Menu.h"
#include <Arduino.h>

void inicializarSistema() {
  // --- Pines / perifericos genericos (no pertenecen a un modulo puntual) ---
  pinMode(pinLCD_PWM, OUTPUT);
  digitalWrite(pinLCD_PWM, HIGH);

  analogReadResolution(12);
  analogSetPinAttenuation(pinTension, ADC_11db);
  analogSetPinAttenuation(pinCorriente, ADC_11db);

  Serial.begin(115200);

  // --- Sintonizacion guardada (si existe) ANTES de iniciar el PID ---
  cargarTuningDesdeNVS();

  // --- Inicializacion propia de cada modulo ---
  iniciarControlPID();   // PIDControl.cpp: pin PWM + PID mode/limits/sampletime
  iniciarInterfaz();     // Menu.cpp: encoder, display, splash, arbol de menu

  Serial.println("--- SOLDADOR INICIADO (menu) ---");
  Serial.print("Setpoint actual: "); Serial.println(Setpoint);
  Serial.print("Kp actual: "); Serial.println(Kp);
  Serial.print("Ki actual: "); Serial.println(Ki);
  Serial.print("Kd actual: "); Serial.println(Kd);

  // --- Tareas ---
  xTaskCreatePinnedToCore(TaskControl, "TaskControl", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(TaskUI,      "TaskUI",       6144, nullptr, 1, nullptr, 1);
}
