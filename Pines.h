#pragma once
// =================================================================
//  PINES — todo el mapeo de hardware en un solo lugar
// =================================================================

// --- MAX6675 ---
const int pinCS_MAX   = 2;
const int pinCLK      = 18;
const int pinMISO     = 19;

// --- LCD (U8g2 SPI) ---
const int pinLCD_CS   = 22;
const int pinLCD_DC   = 25;
const int pinLCD_RST  = 21;
const int pinLCD_MOSI = 23;
const int pinLCD_PWM  = 4;

// --- Salida PWM del calefactor ---
const int pinPWM_OUT  = 15;

// --- ADC (tension / corriente) ---
const int pinCorriente = 34;
const int pinTension   = 39;

// --- Encoder rotativo (pads del modulo ESP-WROOM-32, no dev board) ---
//   pad 29 -> GPIO5  -> CLK (A)
//   pad 28 -> GPIO17 -> DT  (B)
//   pad 27 -> GPIO16 -> SW  (boton)
const int pinEncoderCLK = 5;
const int pinEncoderDT  = 17;
const int pinEncoderSW  = 16;
