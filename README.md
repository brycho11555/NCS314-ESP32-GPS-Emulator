# NCS314 Nixie Clock – ESP32-C3 GPS Emulator (NTP Sync)

**Pełna emulacja GPS NMEA dla oryginalnego firmware GRA & AFCH NCS314 v1.98**  
**Bez żadnej modyfikacji kodu na Arduino Mega!**  


### Dlaczego to rozwiązanie jest najlepsze? 🚀
- Mega myśli, że ma podłączony **prawdziwy GPS** (pełne pakiety `$GPRMC`, `$GPGGA`, `$GPGSA` z checksum).
- Czas lokalny **Polski** (CET/CEST – automatyczne lato/zima).
- NTP z polskich i europejskich serwerów (bardzo dokładne).
- Automatyczny **fallback** na drugą sieć WiFi.
- Sekundy **idealnie płynne** (pakiety co 1s).
- **Zero zmian w oryginalnym firmware** – działa z czystym 1.98!

### Podłączenie 🛠️
**Poprzez kabelek mini jack stereo**

**UWAGA WAŻNA!**  
W schemacie zegara gniazdo GPS jest **błędnie oznaczone** – zamienione są wyprowadzenia: zasilanie +5V ↔ RX  
(Nic się nie stanie jak źle przylutujesz – po prostu ESP nie będzie działał).

ESP32-C3          →          Arduino Mega

TX (GPIO4)        →          RX1 (pin 19) ŚRODKOWY PIN JACK KANAŁ AUDIO PRAWY

GND               →          GND

+5V               →          +5V



### Ustawienia w menu zegara ⏰
1. Wejdź w menu (krótki klik przycisku **SET**).
2. Przejdź do pozycji **Time Zone**.
3. Ustaw wartość **+00** (czas w pakietach NMEA jest już lokalny – Polska).

### Funkcje emulatora 🔧
- Automatyczne WiFi reconnect
- Resync NTP co godzinę
- Diagnostyka w Serial Monitor (co 10 pakietów)
- Obsługa dwóch sieci WiFi (fallback)

### Dlaczego co 1s?

Oryginalny firmware Mega oczekuje pakietów GPS co ~1s (standard NMEA).
Dzięki temu sekundy na tubach są idealnie płynne – zegar traktuje to jak prawdziwy GPS.
Nie ma skoków ani zatrzymań sekund.

NTP resync (odświeżenie czasu z serwera) dzieje się co 1 godzinę:
C++if (millis() - lastNTPSync > 3600000) {  // 3600000 ms = 1 godzina
  // resync NTP
}
To wystarcza – NTP jest bardzo dokładny, a drift RTC minimalny.

### Pliki w repo
- Emulator GPS dla ESP32-C3

### Oryginalny projekt
- https://github.com/afch/NixeTubesShieldNCS314
- https://gra-afch.com

### Autor
brycho11555 ⏰✨  
(Inspiracja: społeczność Nixie – dzięki za wszystkie pomysły!)

---

**Licencja** [![MIT License](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

**Jeśli Ci się przyda – daj ⭐ na repo!**

Enjoy your perfect Nixie clock with internet time! 🕰️🇵🇱
