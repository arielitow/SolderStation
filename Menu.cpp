#include "Menu.h"
#include "Pines.h"
#include "PIDControl.h"
#include "Persistencia.h"
#include <U8g2lib.h>
#include "bitmapsSoldador.h"   // contiene el bitmap de splash OBAKE
                                //     img_sold, img_prog, img_PID, img_info,
                                //     img_ajustes como "static const uint8_t
                                //     xxx[] PROGMEM" de 64x64 (formato XBM),
                                //     igual que OBAKE en bitmapsSoldador.h.

#define MENU_USE_ENCODER
#include <Encoder.h>
#include <MenuFramework.h>

// =================================================================
//  Objetos de hardware / framework
// =================================================================
static U8G2_ST7565_64128N_F_4W_SW_SPI u8g2(U8G2_R0, pinCLK, pinLCD_MOSI, pinLCD_CS, pinLCD_DC, pinLCD_RST);
static Encoder     encoder(pinEncoderCLK, pinEncoderDT, pinEncoderSW);
static MenuContext menu_ctx(u8g2);
static RootMenu    root;

// =================================================================
//  PANTALLA: SOLDAR
// =================================================================
static void drawSoldar(U8G2& scr) {
  scr.clearBuffer();

  scr.setFont(u8g2_font_6x12_tr);
  scr.drawStr(5, 16, "ESTACION DE SOLDAR");
  scr.drawLine(0, 16, 128, 16);

  scr.setFont(u8g2_font_logisoso16_tf);
  char buffer[12];
  dtostrf(tempFiltrada, 5, 2, buffer);
  scr.drawStr(2, 42, buffer);

  scr.setFont(u8g2_font_6x12_tr);
  scr.drawStr(98, 32, "\xB0 C");

  scr.drawLine(0, 48, 128, 48);

  char infoBuf[24];
  if (sensorEnError) {
    scr.setFont(u8g2_font_5x7_tr);
    sprintf(infoBuf, "Set:%dC PWM:%d%%", (int)Setpoint, (int)((dutyPWM / 255.0) * 100));
    scr.drawStr(2, 56, infoBuf);
    scr.drawStr(2, 63, "!! ERROR TERMOCUPLA !!");
  } else {
    scr.setFont(u8g2_font_6x12_tr);
    sprintf(infoBuf, "Set:%dC PWM:%d%%", (int)Setpoint, (int)((dutyPWM / 255.0) * 100));
    scr.drawStr(5, 60, infoBuf);
  }

  scr.sendBuffer();
}

static bool eventSoldar(MenuEvent ev, MenuContext& ctx) {
  if (ev == MENU_VAL_INC) { ajustarSetpoint(+1); return true; }
  if (ev == MENU_VAL_DEC) { ajustarSetpoint(-1); return true; }
  return false; // BACK -> comportamiento default del framework (pop)
}

static FullscreenItem scr_soldador("Soldar", drawSoldar, eventSoldar);

// =================================================================
//  SUBMENU: PROGRAMAS
// =================================================================
// Temperaturas de referencia (AJUSTAR A GUSTO, son valores de partida).
static const double TEMP_FREE_LEAD = 350.0; // soldadura sin plomo
static const double TEMP_CUPPER    = 380.0; // cobre / trenza desoldadora
static const double TEMP_NORMAL    = 320.0; // soldadura con plomo

static CustomSubMenu scr_programas("Programas");

static ActionItem prog_freelead("Free Lead", [](MenuContext& ctx) {
  Setpoint = TEMP_FREE_LEAD;
  ctx.popToRoot();
});
static ActionItem prog_cupper("Cupper", [](MenuContext& ctx) {
  Setpoint = TEMP_CUPPER;
  ctx.popToRoot();
});
static ActionItem prog_normal("Normal", [](MenuContext& ctx) {
  Setpoint = TEMP_NORMAL;
  ctx.popToRoot();
});

// =================================================================
//  PANTALLA: CALIBRAR
// =================================================================
static uint8_t calBtnSel = 0; // 0 = OK, 1 = Cancelar (solo aplica con resultado listo)

static void enterCalibrar() {
  // Si ya hay un resultado pendiente de confirmar (por ejemplo, un
  // autotune disparado antes por comando serie), no lo pisamos: se
  // muestra tal cual. Si no hay nada corriendo ni pendiente, arrancamos
  // uno nuevo a 500C.
  if (!autotuneActivo && !calResultadoListo) {
    calBtnSel = 0;
    iniciarAutotune(500.0);
  }
}

static void drawCalibrar(U8G2& scr) {
  scr.clearBuffer();
  _menuDrawTitleBar(scr, "Calibrar");

  scr.setFont(MENU_FONT_NORMAL);
  char buf[24];

  if (autotuneActivo) {
    scr.setCursor(4, _menuContentY() + 2);
    scr.print("Calibrando a 500C");

    snprintf(buf, sizeof(buf), "Temp: %.1f C", tempFiltrada);
    scr.setCursor(4, _menuContentY() + 16);
    scr.print(buf);

    snprintf(buf, sizeof(buf), "Ciclos: %d/%d", atMuestrasValidas, AT_CICLOS_PROMEDIO);
    scr.setCursor(4, _menuContentY() + 30);
    scr.print(buf);
  } else if (calResultadoListo) {
    snprintf(buf, sizeof(buf), "Kp=%.3f Ki=%.4f", calKpResultado, calKiResultado);
    scr.setCursor(2, _menuContentY() + 2);
    scr.print(buf);
    snprintf(buf, sizeof(buf), "Kd=%.3f", calKdResultado);
    scr.setCursor(2, _menuContentY() + 16);
    scr.print(buf);

    const uint8_t btnY  = MENU_SCREEN_H - MENU_BTNROW_H;
    const uint8_t halfW = MENU_SCREEN_W / 2;
    _menuDrawBtn(scr, MENU_STR_OK,     2,         btnY, halfW - 4, MENU_BTNROW_H, calBtnSel == 0);
    _menuDrawBtn(scr, MENU_STR_CANCEL, halfW + 2, btnY, halfW - 4, MENU_BTNROW_H, calBtnSel == 1);
  } else {
    scr.setCursor(4, _menuContentY() + 2);
    scr.print("Abortado.");
  }

  scr.sendBuffer();
}

static bool eventCalibrar(MenuEvent ev, MenuContext& ctx) {
  if (autotuneActivo) {
    if (ev == MenuEvent::BACK) {
      abortarAutotune("Cancelado desde el menu.");
      ctx.pop();
    }
    return true; // mientras calibra, se ignora cualquier otro evento
  }

  if (calResultadoListo) {
    if (ev == MENU_VAL_INC || ev == MENU_VAL_DEC) {
      calBtnSel = (calBtnSel == 0) ? 1 : 0;
    } else if (ev == MenuEvent::BACK) {
      if (calBtnSel == 0) {  // OK: aplicar y guardar
        Kp = calKpResultado;
        Ki = calKiResultado;
        Kd = calKdResultado;
        miPID.SetTunings(Kp, Ki, Kd);
        guardarTuningEnNVS();
      }
      calResultadoListo = false;
      ctx.pop();
    }
    return true;
  }

  // Se aborto por seguridad mientras la pantalla estaba abierta:
  // cualquier pulsacion de boton vuelve al root.
  if (ev == MenuEvent::BACK) ctx.pop();
  return true;
}

static FullscreenItem scr_calibrar("Calibrar", drawCalibrar, eventCalibrar, enterCalibrar, nullptr);

// =================================================================
//  PANTALLA: INFORMACION
// =================================================================
static DisplayScreen scr_info("Informacion", [](U8G2& scr) {
  char buf[24];
  scr.setFont(MENU_FONT_NORMAL);

  snprintf(buf, sizeof(buf), "Kp: %.3f", Kp);
  scr.setCursor(4, _menuContentY() + 2);
  scr.print(buf);

  snprintf(buf, sizeof(buf), "Ki: %.4f", Ki);
  scr.setCursor(4, _menuContentY() + 16);
  scr.print(buf);

  snprintf(buf, sizeof(buf), "Kd: %.3f", Kd);
  scr.setCursor(4, _menuContentY() + 30);
  scr.print(buf);
});

// =================================================================
//  PANTALLA: AJUSTES (placeholder)
// =================================================================
static DisplayScreen scr_ajustes("Ajustes", [](U8G2& scr) {
  scr.setFont(MENU_FONT_NORMAL);
  scr.setCursor(4, _menuContentY() + 2);
  scr.print("Proximamente...");
});

// =================================================================
//  Armado del arbol de menu
// =================================================================
static void construirMenu() {
    scr_programas.add(&prog_freelead);
    scr_programas.add(&prog_cupper);
    scr_programas.add(&prog_normal);

    root.add(&scr_soldador, img_sold);
    root.add(&scr_programas, img_prog);
    root.add(&scr_calibrar, img_PID);
    root.add(&scr_info, img_info);
    root.add(&scr_ajustes, img_ajustes);
    menu_ctx.push(&root);
}

// =================================================================
//  Inicializacion propia del modulo
// =================================================================
void iniciarInterfaz() {
  encoder.begin();
  u8g2.begin();

  // --- Splash (bitmap directo, sin texto previo) ---
  u8g2.clearBuffer();
  u8g2.drawXBM(0, 0, 128, 64, WHALE_LOGO);
  u8g2.sendBuffer();
  delay(2000);

  construirMenu();
}

// =================================================================
//  Tarea de interfaz — encoder + menu + comandos serie (debug)
// =================================================================
void TaskUI(void* pvParameters) {
  for (;;) {
    // --- Comandos seriales (debug / compatibilidad, en paralelo al menu) ---
    if (Serial.available() > 0) {
      String comando = Serial.readStringUntil('\n');
      comando.trim();

      if (comando.length() > 0) {
        char letra = comando.charAt(0);
        double valor = comando.substring(1).toDouble();

        if (letra == 'T' || letra == 't') {
          if (comando.length() > 1) {
            if (valor >= 0 && valor <= 600) {
              Setpoint = valor;
              Serial.print("Nuevo Setpoint (T): ");
              Serial.println(Setpoint);
            }
          } else {
            Serial.println("Comando T sin valor, se ignora.");
          }
        }
        else if (letra == 'P' || letra == 'p') {
          if (comando.length() > 1) {
            Kp = valor;
            miPID.SetTunings(Kp, Ki, Kd);
            Serial.print("Nuevo Kp: ");
            Serial.println(Kp);
          } else {
            Serial.println("Comando P sin valor, se ignora.");
          }
        }
        else if (letra == 'I' || letra == 'i') {
          if (comando.length() > 1) {
            Ki = valor;
            miPID.SetTunings(Kp, Ki, Kd);
            Serial.print("Nuevo Ki: ");
            Serial.println(Ki);
          } else {
            Serial.println("Comando I sin valor, se ignora.");
          }
        }
        else if (letra == 'D' || letra == 'd') {
          if (comando.length() > 1) {
            Kd = valor;
            miPID.SetTunings(Kp, Ki, Kd);
            Serial.print("Nuevo Kd: ");
            Serial.println(Kd);
          } else {
            Serial.println("Comando D sin valor, se ignora.");
          }
        }
        else if (letra == 'S' || letra == 's') {
          Serial.println("=== ESTADO ACTUAL PID ===");
          Serial.print("Setpoint: "); Serial.println(Setpoint);
          Serial.print("Kp: "); Serial.println(Kp);
          Serial.print("Ki: "); Serial.println(Ki);
          Serial.print("Kd: "); Serial.println(Kd);
          Serial.println("=========================");
        }
        else if (letra == 'A' || letra == 'a') {
          if (comando.length() > 1) {
            double objetivo = valor;
            if (autotuneActivo) {
              Serial.println("Ya hay un autotune en curso, se ignora.");
            } else if (objetivo <= 0 || objetivo + AT_HISTERESIS + 20 > AT_TECHO) {
              Serial.print("Objetivo invalido o demasiado cercano al techo de seguridad (");
              Serial.print(AT_TECHO);
              Serial.println(" C). Elegi un valor menor.");
            } else {
              iniciarAutotune(objetivo);
            }
          } else {
            if (autotuneActivo) {
              abortarAutotune("Cancelado manualmente por comando serie.");
            } else {
              Serial.println("Comando A sin valor, se ignora.");
            }
          }
        }
      }
    }

    // --- Encoder + menu ---
    menu_ctx.update(encoder);
    menu_ctx.draw();

    vTaskDelay(pdMS_TO_TICKS(30));
  }
}
