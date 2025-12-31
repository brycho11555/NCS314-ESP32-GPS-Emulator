#include <WiFi.h>
#include <time.h>
#include <HardwareSerial.h>

// ════════════════════════════════════════════════════════════════
//  KONFIGURACJA WiFi
// ════════════════════════════════════════════════════════════════
const char* ssid1     = "Twoja_sieć_1";
const char* password1 = "Twoje_hasło_do_sieci_1";
const char* ssid2     = "Twoja_sieć_2";
const char* password2 = "Twoje_hasło_do sieci_2";

// ════════════════════════════════════════════════════════════════
//  STREFA CZASOWA - POLSKA (automatyczne lato/zima)
// ════════════════════════════════════════════════════════════════
// CET-1CEST,M3.5.0,M10.5.0/3 oznacza:
// - Zimą: CET (UTC+1)
// - Latem: CEST (UTC+2) 
// - Zmiana na letni: ostatnia niedziela marca o 2:00
// - Zmiana na zimowy: ostatnia niedziela października o 3:00
const char* timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

// ════════════════════════════════════════════════════════════════
//  UART do Arduino Mega (symulacja GPS)
// ════════════════════════════════════════════════════════════════
HardwareSerial GPSSerial(1);
#define GPS_TX_PIN 4        // ESP TX → Arduino RX1 (pin 19)
#define GPS_BAUD   9600

// ════════════════════════════════════════════════════════════════
//  ZMIENNE GLOBALNE
// ════════════════════════════════════════════════════════════════
unsigned long lastGPSSent = 0;
const unsigned long GPS_INTERVAL = 1000;  // Co 1 sekundę
bool ntpSynced = false;

// ════════════════════════════════════════════════════════════════
//  FUNKCJE
// ════════════════════════════════════════════════════════════════

bool connectToWiFi(const char* ssid, const char* password) {
  Serial.print("📡 Łączenie z: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi OK: " + String(ssid));
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("\n✗ WiFi FAIL: " + String(ssid));
  return false;
}

// Oblicza checksum XOR dla pakietu NMEA
uint8_t calculateNMEAChecksum(const char* sentence) {
  uint8_t checksum = 0;
  for (int i = 1; sentence[i] != '*' && sentence[i] != '\0'; i++) {
    checksum ^= sentence[i];
  }
  return checksum;
}

// Generuje pakiet $GPRMC z polskim czasem lokalnym
String generateGPRMC(struct tm &timeinfo) {
  char packet[120];
  
  // Format NMEA: $GPRMC,HHMMSS.000,A,lat,N,lon,E,speed,course,DDMMYY,mag,*CS
  // Pozycja: Warszawa (52.23N, 21.01E)
  sprintf(packet, "$GPRMC,%02d%02d%02d.000,A,5213.8000,N,02100.6000,E,0.0,0.0,%02d%02d%02d,0.0,E",
          timeinfo.tm_hour,    // Czas LOKALNY dla Polski (CET/CEST)
          timeinfo.tm_min,
          timeinfo.tm_sec,
          timeinfo.tm_mday,
          timeinfo.tm_mon + 1,
          (timeinfo.tm_year + 1900) % 100);
  
  uint8_t checksum = calculateNMEAChecksum(packet);
  
  char fullPacket[130];
  sprintf(fullPacket, "%s*%02X\r\n", packet, checksum);
  
  return String(fullPacket);
}

// Dodatkowy pakiet GPGGA (opcjonalny - dla kompletności)
String generateGPGGA(struct tm &timeinfo) {
  char packet[120];
  sprintf(packet, "$GPGGA,%02d%02d%02d.000,5213.8000,N,02100.6000,E,1,08,1.0,100.0,M,0.0,M,,",
          timeinfo.tm_hour,
          timeinfo.tm_min,
          timeinfo.tm_sec);
  
  uint8_t checksum = calculateNMEAChecksum(packet);
  char fullPacket[130];
  sprintf(fullPacket, "%s*%02X\r\n", packet, checksum);
  return String(fullPacket);
}

String generateGPGSA() {
  return "$GPGSA,A,3,01,02,03,04,05,06,07,08,,,,,2.0,1.0,1.7*30\r\n";
}

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, -1, GPS_TX_PIN);
  
  delay(500);
  
  Serial.println("ESP32-C3 GPS Time Server dla NCS314");
  Serial.println("Strefa: Polska (CET/CEST) - auto lato/zima");
  

  // ────────────────────────────────────────────────────────────
  // POŁĄCZENIE WiFi
  // ────────────────────────────────────────────────────────────
  if (!connectToWiFi(ssid1, password1)) {
    if (!connectToWiFi(ssid2, password2)) {
      Serial.println("✗ Brak WiFi - restart za 10s");
      delay(10000);
      ESP.restart();
    }
  }

  // ────────────────────────────────────────────────────────────
  // SYNCHRONIZACJA NTP
  // ────────────────────────────────────────────────────────────
  Serial.println("\n⏳ Synchronizacja z NTP...");
  
  // Używamy wielu serwerów NTP dla niezawodności
  configTime(0, 0, 
             "pl.pool.ntp.org",      // Polski serwer NTP
             "europe.pool.ntp.org",  // Europejski
             "pool.ntp.org");        // Globalny
  
  // Ustawienie strefy czasowej Polski
  setenv("TZ", timezone, 1);
  tzset();
  
  Serial.println("  Serwery NTP:");
  Serial.println("  - pl.pool.ntp.org");
  Serial.println("  - europe.pool.ntp.org");
  Serial.println("  - pool.ntp.org");

  // Czekaj na synchronizację
  int ntpAttempts = 0;
  while (time(nullptr) < 1000000000 && ntpAttempts < 40) {
    delay(500);
    Serial.print(".");
    ntpAttempts++;
  }
  
  if (time(nullptr) < 1000000000) {
    Serial.println("\n✗ NTP timeout - restart za 5s");
    delay(5000);
    ESP.restart();
  }
  
  ntpSynced = true;
  Serial.println("\n✓ NTP zsynchronizowany!");
  
  // ────────────────────────────────────────────────────────────
  // WYŚWIETL AKTUALNY CZAS
  // ────────────────────────────────────────────────────────────
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[100];
    sprintf(timeStr, "✓ Czas lokalny: %04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
    Serial.println(timeStr);
    
    // Informacja o strefie czasowej
    if (timeinfo.tm_isdst > 0) {
      Serial.println("  Strefa: CEST (czas letni, UTC+2)");
    } else {
      Serial.println("  Strefa: CET (czas zimowy, UTC+1)");
    }
  }
  
  Serial.println("\n🛰️  Emulacja GPS START - pakiety NMEA co 1s");
  Serial.println("   Arduino powinno mieć TimeZone ustawioną na +00");
  Serial.println("════════════════════════════════════════════════════════\n");
  
  delay(1000);
}

void loop() {
  static int packetCounter = 0;
  static unsigned long lastReconnect = 0;
  static unsigned long lastNTPSync = 0;
  
  // ────────────────────────────────────────────────────────────
  // AUTO-RECONNECT WiFi (co 30s jeśli rozłączony)
  // ────────────────────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED && (millis() - lastReconnect > 30000)) {
    lastReconnect = millis();
    Serial.println("\n⚠ WiFi lost - reconnecting...");
    if (!connectToWiFi(ssid1, password1)) {
      connectToWiFi(ssid2, password2);
    }
  }

  // ────────────────────────────────────────────────────────────
  // RESYNC NTP (co 1 godzinę)
  // ────────────────────────────────────────────────────────────
  if (millis() - lastNTPSync > 3600000) {
    lastNTPSync = millis();
    Serial.println("🔄 NTP resync...");
    configTime(0, 0, "pl.pool.ntp.org");
  }

  // ────────────────────────────────────────────────────────────
  // WYSYŁANIE PAKIETÓW GPS (co 1 sekundę)
  // ────────────────────────────────────────────────────────────
  if (millis() - lastGPSSent >= GPS_INTERVAL) {
    lastGPSSent = millis();
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("✗ Błąd odczytu czasu");
      return;
    }
    
    // Walidacja czasu (nie wysyłaj jeśli czas nieprawidłowy)
    if (timeinfo.tm_year < 124) {  // 2024
      Serial.println("⚠ Czas < 2024 - czekam na NTP");
      return;
    }
    
    // Generuj i wyślij pakiety NMEA
    String rmc = generateGPRMC(timeinfo);
    String gga = generateGPGGA(timeinfo);
    String gsa = generateGPGSA();
    
    // Wyślij do Arduino przez UART
    GPSSerial.print(rmc);
    GPSSerial.print(gga);
    GPSSerial.print(gsa);
    
    // ────────────────────────────────────────────────────────
    // DIAGNOSTYKA (co 10 pakietów)
    // ────────────────────────────────────────────────────────
    packetCounter++;
    if (packetCounter % 10 == 0) {
      char logLine[150];
      sprintf(logLine, "📡 [%4d] %04d-%02d-%02d %02d:%02d:%02d | %s | WiFi:%s | %lus",
              packetCounter,
              timeinfo.tm_year + 1900,
              timeinfo.tm_mon + 1,
              timeinfo.tm_mday,
              timeinfo.tm_hour,
              timeinfo.tm_min,
              timeinfo.tm_sec,
              timeinfo.tm_isdst > 0 ? "CEST(+2)" : "CET(+1) ",
              WiFi.status() == WL_CONNECTED ? "OK" : "ERR",
              millis() / 1000);
      Serial.println(logLine);
      
      // Pokaż przykładowy pakiet RMC
      Serial.print("   RMC: ");
      Serial.print(rmc);
    }
  }
  
  delay(10);
}