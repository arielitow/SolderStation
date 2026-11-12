#pragma once
// =================================================================
//  Menu — encoder, arbol de pantallas (SOLDAR / PROGRAMAS / CALIBRAR /
//  INFORMACION / AJUSTES) y la tarea de interfaz. No conoce el detalle
//  interno del PID, solo lee/edita las variables publicas de
//  PIDControl.h y llama a Persistencia.h para guardar.
// =================================================================
void iniciarInterfaz();            // encoder.begin(), u8g2.begin(), splash, arma el menu
void TaskUI(void* pvParameters);   // tarea FreeRTOS: encoder + menu + comandos serie
