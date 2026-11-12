#include "Persistencia.h"
#include "PIDControl.h"
#include <Preferences.h>

static Preferences prefs;

void cargarTuningDesdeNVS() {
  prefs.begin("soldador", true);   // solo lectura
  if (prefs.isKey("kp")) Kp = prefs.getDouble("kp", Kp);
  if (prefs.isKey("ki")) Ki = prefs.getDouble("ki", Ki);
  if (prefs.isKey("kd")) Kd = prefs.getDouble("kd", Kd);
  prefs.end();
}

void guardarTuningEnNVS() {
  prefs.begin("soldador", false);  // lectura/escritura
  prefs.putDouble("kp", Kp);
  prefs.putDouble("ki", Ki);
  prefs.putDouble("kd", Kd);
  prefs.end();
  Serial.println("Tuning guardado en NVS.");
}
