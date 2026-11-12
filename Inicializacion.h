#pragma once
// =================================================================
//  Inicializacion — junta el arranque de todos los modulos en un
//  solo lugar, en el orden correcto, y crea las dos tareas de
//  FreeRTOS. setup() en el .ino solo llama a inicializarSistema().
// =================================================================
void inicializarSistema();
