#include <WiFi.h>
#include <WiFiManager.h>
#include <ThingSpeak.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Pines ---
#define PIN_DATOS_TEMP 4
#define PIN_TRIG 5
#define PIN_ECHO 18
#define PH_PIN 34          // Pin ADC1 donde llega la señal del módulo PH-4502C
#define NUM_MUESTRAS 30    // Muestras para promediar la lectura de pH

// --- ThingSpeak ---
unsigned long CHANNEL_ID = 3480616;
const char* WRITE_API_KEY = "S937KVE0LGV8EE4S";

// --- Calibración de pH (Actualizada con los 3 puntos) ---
float PH_PENDIENTE = -10.3326;
float PH_ORDENADA = 32.7215;

// Ajuste fino: se deja en 0.0 y se modifica DESPUÉS de verificar en ThingSpeak.
float AJUSTE_PH = 0.0;

// --- Conversión de microsegundos a segundos ---
#define uS_TO_S_FACTOR 1000000ULL
// --- Tiempo de sueño en segundos (60 seg para pruebas / 3600 para 1 hora) ---
#define TIEMPO_SUEÑO 60

OneWire oneWire(PIN_DATOS_TEMP);
DallasTemperature sensorTemp(&oneWire);
WiFiClient cliente;

float medirTemperatura() {
  sensorTemp.requestTemperatures();
  return sensorTemp.getTempCByIndex(0);
}

float medirNivel() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);
  return duracion * 0.0343 / 2;
}

// Devuelve el voltaje (en V) que llega al pin usando la calibración ADC del ESP32
float leerVoltajePH() {
  long suma = 0;
  for (int i = 0; i < NUM_MUESTRAS; i++) {
    suma += analogReadMilliVolts(PH_PIN);
    delay(10);
  }
  return (suma / (float)NUM_MUESTRAS) / 1000.0;
}

float medirPH() {
  float voltaje = leerVoltajePH();
  return PH_PENDIENTE * voltaje + PH_ORDENADA + AJUSTE_PH;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  sensorTemp.begin();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // --- 1) MEDIR PRIMERO, con el WiFi todavía apagado ---
  // Se mide antes de conectar para evitar que los picos de consumo del Wi-Fi afecten la referencia de voltaje del pH
  float temperatura = medirTemperatura();
  float distancia = medirNivel();
  float ph = medirPH();

  Serial.println("----------------------");
  Serial.print("Temp: "); Serial.print(temperatura); Serial.print(" C    ");
  Serial.print("Nivel: "); Serial.print(distancia); Serial.print(" cm    ");
  Serial.print("pH: "); Serial.println(ph, 2);

  // --- 2) RECIÉN AHORA conectar el WiFi ---
  WiFiManager wm;
  wm.setDebugOutput(false);

  bool conectado = wm.autoConnect("ESP32-Pileta");
  if (!conectado) {
    Serial.println("[ERROR] No se pudo conectar al WiFi");
    delay(3000);
    ESP.restart();
  }

  Serial.println("[OK] WiFi conectado");
  ThingSpeak.begin(cliente);

  // --- 3) Enviar a ThingSpeak ---
  ThingSpeak.setField(1, distancia);
  ThingSpeak.setField(2, temperatura);
  ThingSpeak.setField(3, ph);
  int resultado = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);

  if (resultado == 200) {
    Serial.println("[OK] Enviado a ThingSpeak");
  } else {
    Serial.print("[ERROR] Código HTTP: ");
    Serial.println(resultado);
  }

  // --- 4) Configurar y activar Deep Sleep ---
  Serial.println("[INFO] Entrando en Deep Sleep...");
  Serial.flush();

  esp_sleep_enable_timer_wakeup(TIEMPO_SUEÑO * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // El loop queda vacío porque todo el flujo se ejecuta una sola vez en setup()
}