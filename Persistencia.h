#pragma once
// =================================================================
//  Persistencia — guarda/carga la sintonizacion del PID en NVS
//  (memoria no volatil del ESP32), asi sobrevive a un reinicio sin
//  tener que recompilar el codigo.
// =================================================================
void cargarTuningDesdeNVS();   // llamar antes de iniciarControlPID()
void guardarTuningEnNVS();     // llamar cuando el usuario confirma un autotune
