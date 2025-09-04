#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <DHT20.h>
#include <Adafruit_CCS811.h>
#include <HardwareSerial.h>
#include <MHZ19.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// WiFi
#define WIFI_SSID "WIFI"
#define WIFI_PASSWORD "Password"

// Azure IoT Hub
#define SAS_TOKEN "SharedAccessSignature....."
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEtjCCA56gAwIBAgIQCv1eRG9c89YADp5Gwibf9jANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0yMjA0MjgwMDAwMDBaFw0zMjA0MjcyMzU5NTlaMEcxCzAJBgNVBAYTAlVT
MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xGDAWBgNVBAMTD01TRlQg
UlMyNTYgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMiJV34o
eVNHI0mZGh1Rj9mdde3zSY7IhQNqAmRaTzOeRye8QsfhYFXSiMW25JddlcqaqGJ9
GEMcJPWBIBIEdNVYl1bB5KQOl+3m68p59Pu7npC74lJRY8F+p8PLKZAJjSkDD9Ex
mjHBlPcRrasgflPom3D0XB++nB1y+WLn+cB7DWLoj6qZSUDyWwnEDkkjfKee6ybx
SAXq7oORPe9o2BKfgi7dTKlOd7eKhotw96yIgMx7yigE3Q3ARS8m+BOFZ/mx150g
dKFfMcDNvSkCpxjVWnk//icrrmmEsn2xJbEuDCvtoSNvGIuCXxqhTM352HGfO2JK
AF/Kjf5OrPn2QpECAwEAAaOCAYIwggF+MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYD
VR0OBBYEFAyBfpQ5X8d3on8XFnk46DWWjn+UMB8GA1UdIwQYMBaAFE4iVCAYlebj
buYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcD
AQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8v
b2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jYWNlcnRzLmRp
Z2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5jcnQwQgYDVR0fBDswOTA3
oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v
dEcyLmNybDA9BgNVHSAENjA0MAsGCWCGSAGG/WwCATAHBgVngQwBATAIBgZngQwB
AgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG9w0BAQsFAAOCAQEAdYWmf+AB
klEQShTbhGPQmH1c9BfnEgUFMJsNpzo9dvRj1Uek+L9WfI3kBQn97oUtf25BQsfc
kIIvTlE3WhA2Cg2yWLTVjH0Ny03dGsqoFYIypnuAwhOWUPHAu++vaUMcPUTUpQCb
eC1h4YW4CCSTYN37D2Q555wxnni0elPj9O0pymWS8gZnsfoKjvoYi/qDPZw1/TSR
penOgI6XjmlmPLBrk4LIw7P7PPg4uXUpCzzeybvARG/NIIkFv1eRYIbDF+bIkZbJ
QFdB9BjjlA4ukAg2YkOyCiB8eXTBi2APaceh3+uBLIgLk8ysy52g2U3gP7Q26Jlg
q/xKzj3O9hFh/g==
-----END CERTIFICATE-----
)EOF";

String iothubName = "CS147Davidhub";
String deviceName = "147esp32";
String url = "https://" + iothubName + ".azure-devices.net/devices/" + deviceName + "/messages/events?api-version=2021-04-12";

#define TELEMETRY_INTERVAL 10000 
#define SDA_PIN  21
#define SCL_PIN  22
#define LDR_PIN  36
#define CO2_RX   33
#define CO2_TX   32
#define TFT_BL   4


DHT20 dht;
Adafruit_CCS811 ccs;
HardwareSerial co2Serial(2);
MHZ19 mhz19;
TFT_eSPI tft;

uint32_t lastTelemetryTime = 0;
float temp = NAN, humi = NAN;
int   adcRaw = 0;
float lightPct = 0;

float eco2_f = NAN, tvoc_f = NAN;
const int CO2_WARN =1000;
const int CO2_ALERT=1500;
const int TVOC_WARN =300;

static inline float topct(int v) {
  float p = (v /4095.0f) *100.0f;
  if (p<0) p=0; 
  if (p>100) p = 100;
  return p;
}


void drawFrame() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextWrap(false);
}

void showonscreen(int co2R, float eco2, float tvoc) {
  const int leftX= 10;
  const int rightX=130;
  const int topY= 6;
  const int rowStep =28;

  tft.fillRect(0, 0, 240, 110, TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextWrap(false);

  //T/H
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String sTemp = "T:" +(isnan(temp) ? String("--") : String(temp, 1)+ "C");
  String sHum  = "H:" +(isnan(humi) ? String("--") : String((int)humi)+ "%");
  String sLight  = "L:" + String((int)lightPct)+ "%";
  tft.drawString(sTemp, leftX, topY, 2);
  tft.drawString(sHum,rightX, topY, 2);
  tft.drawString(sLight,leftX, topY + rowStep, 2);

  //CO2
  uint16_t co2Color=TFT_WHITE;
  if (co2R >= CO2_ALERT)  co2Color = TFT_RED;
  else if (co2R >= CO2_WARN)  co2Color = TFT_ORANGE;
  tft.setTextColor(co2Color, TFT_BLACK);
  String sCO2 = "C:" + ((co2R > 0 && co2R < 5000) ? String(co2R) : String("--"));
  tft.drawString(sCO2,rightX, topY + rowStep, 2);

  //eCO2
  uint16_t eco2Color = TFT_WHITE;
  if (!isnan(eco2)) {
    if (eco2 >= CO2_ALERT)  eco2Color = TFT_RED;
    else if (eco2 >= CO2_WARN)  eco2Color = TFT_ORANGE;
  }
  tft.setTextColor(eco2Color, TFT_BLACK);
  String sECO2 = "e:" + (isnan(eco2) ? String("--") : String((int)eco2));
  tft.drawString(sECO2, leftX, topY + rowStep * 2, 2);

  //TVOC
  uint16_t tvocColor = TFT_WHITE;
  if (!isnan(tvoc)) {
    if (tvoc >= TVOC_WARN) tvocColor = TFT_RED;
  }
  tft.setTextColor(tvocColor, TFT_BLACK);
  String sTVOC = "v:" + (isnan(tvoc) ? String("--") : String((int)tvoc));
  tft.drawString(sTVOC, rightX, topY+ rowStep * 2, 2);
}



void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  dht.begin();

  if (ccs.begin(0x5A)) {
    ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);
  } else {
    Serial.println("CCS811 not found!");
  }

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  co2Serial.begin(9600, SERIAL_8N1, CO2_RX, CO2_TX);
  mhz19.begin(co2Serial);
  mhz19.autoCalibration(false);

  drawFrame();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected!");
}

void loop() {

  if (dht.read() == 0) {
    temp = dht.getTemperature();
    humi = dht.getHumidity();
  }

  adcRaw   = analogRead(LDR_PIN);
  lightPct = topct(adcRaw);
  float eco2_now = NAN, tvoc_now = NAN;
  if (ccs.available()) {
    if (!isnan(humi) && !isnan(temp)) ccs.setEnvironmentalData((uint8_t)humi, temp);
    if (!ccs.readData()) {
    float eco2_now = ccs.geteCO2();
    float tvoc_now = ccs.getTVOC();
    if (!isnan(eco2_now)) eco2_f = eco2_now;
    if (!isnan(tvoc_now)) tvoc_f = tvoc_now;
  }
}
  int co2R = mhz19.getCO2();
  Serial.print("{\"temp\":");      
  Serial.print(isnan(temp)?NAN:temp,1);
  Serial.print(",\"hum\":");       
  Serial.print(isnan(humi)?NAN:humi,0);
  Serial.print(",\"light_raw\":"); 
  Serial.print(adcRaw);
  Serial.print(",\"light_pct\":"); 
  Serial.print(lightPct,0);
  Serial.print(",\"co2\":");       
  Serial.print(co2R);
  Serial.print(",\"eco2\":");      
  Serial.print(isnan(eco2_f)?-1:eco2_f,0);
  Serial.print(",\"tvoc\":");      
  Serial.print(isnan(tvoc_f)?-1:tvoc_f,0);
  Serial.println("}");

  showonscreen(co2R, eco2_f, tvoc_f);

if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
  StaticJsonDocument<256> doc;

  doc["temperature"] = temp;
  doc["humidity"]  = humi;
  doc["light_raw"]  = adcRaw;
  doc["light_pct"]  = (int)lightPct;
  doc["co2"]      = co2R;

  if (isnan(eco2_f)) doc["eco2"] = nullptr; else doc["eco2"] = (int)eco2_f;
  if (isnan(tvoc_f)) doc["tvoc"] = nullptr; else doc["tvoc"] = (int)tvoc_f;

  char buffer[320];
  serializeJson(doc, buffer);

  WiFiClientSecure client;
  client.setCACert(root_ca);

  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", SAS_TOKEN);

  int httpCode = http.POST((uint8_t*)buffer, strlen(buffer));
  if (httpCode == 204) {
    Serial.println(String("Telemetry sent: ") + buffer);
  } else {
    Serial.println(String("Send failed, HTTP: ") + httpCode);
  }
  http.end();

  lastTelemetryTime = millis();
}
delay(1000); 
}
