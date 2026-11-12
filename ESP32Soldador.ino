#include "Inicializacion.h"

// Todo el trabajo real vive en tareas de FreeRTOS (ver Inicializacion.cpp,
// PIDControl.cpp y Menu.cpp). setup()/loop() quedan minimos a proposito.

void setup() {
  inicializarSistema();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
