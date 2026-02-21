// Copyright (C) 2026 C-E

const char* FirmwareVersion = "3.0";
#include "Configuration.h"
#include <string>
#include "Datastore.h"
#include "Display_Graphic.h"
#include <Hoymiles.h>
#include "InverterSettings.h"
#include "Led_Single.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include "NtpSettings.h"
#include "PinMapping.h"
#include "Scheduler.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <TaskScheduler.h>
#include <LoRa.h>
#include "Cloud_logger.h"

SPIClass spiV(VSPI); // VSPI für LoRa verwenden
#define LORA_SCLK_PIN              5
#define LORA_MISO_PIN              19
#define LORA_MOSI_PIN              27
#define LORA_CS_PIN                18
#define LORA_DIO0_PIN              26
#define LORA_RST_PIN               23
std::string ausgabe = "Nr.";
std::string recv = ""; // string receiving lora data 
volatile int wattprozent = 0; // watt in % von 800 watt
volatile bool man_aend_wattprozent = false; // watt in % manuell via webseite geändert
bool nureinmal = false;
float dcVoltage_float;
float abschaltwert;
bool hoy_aktiv = false;
int akPower = 0; // aktuell gesetzter wert in % von 800 watt
int NewPower = 0;
bool LoraOn = true; // zum ein-/ausschalten der verarbeitung der lora-messages (default: ein)
std::shared_ptr<InverterAbstract> inv;
bool cmdwaitperiod = false;
uint32_t cmdwaitperiodstart;
uint32_t taktstart;
uint32_t _lastWattaenderung;
uint32_t lastLoRaMillis;
int zaehl2 = 0; // für abschaltwert-verzögerung
bool abschaltwert_belassen = false;
float watts;
float inverterTemp = 0.0f;

const char identifier[6] = { 'V', '#', '@', '~', '&', '_' }; // identifier für lora
bool aufmachen = true;

char payload[9];
int32_t powerSaldo;
int32_t altpowerSaldo;
int i = 0;
bool jetzteinschalten = false;
bool jetztabschalten = false;

const float HYSTERESIS_VOLTAGE = 0.4;
const float VOLTAGE_LEVELS[] = {26.7, 26.8, 26.9, 27.0, 27.1, 27.2};
const int POWER_LEVELS[] = {10, 20, 30, 40, 50, 60};
const int NUM_LEVELS = 6;
static float upperThresholds[NUM_LEVELS] = {
    27.1, 27.2, 27.3, 27.4, 27.5, 27.6  // mit 0,4 V Hysterese
};

const uint32_t POWER_ON_OFF_WAIT = 3000;      // Wartezeit vor Ein-/Ausschalten 3000 ms = 3 s
const uint32_t POWER_ADJUST_WAIT = 4500;      // Wartezeit nach Wattänderung 4500 ms = 4,5 s
const uint32_t CMD_WAIT_PERIOD = 6000;        // Wartezeit nach Ein-/Aus-Schaltbefehl 6000 ms = 6 s
const uint32_t LORA_TIMEOUT = 30000;          // LoRa-Daten-Timeout, z.b. 30 s
const uint32_t INV_DATA_TIMEOUT = 30000;      // Inverter-Daten-Timeout, z.b. 30 s
const uint32_t TAKT_INTERVAL = 2000;          // Takt, z.b. 2 s


void setup()
{
   delay(1500);     // When the power is turned on, a delay is required.
   Serial.begin(SERIAL_BAUDRATE); // Initialize serial output
   #if ARDUINO_USB_CDC_ON_BOOT
      Serial.setTxTimeoutMs(0);
      delay(100);
   #else
      while (!Serial)
      yield();
   #endif
   MessageOutput.init(scheduler);
   MessageOutput.println();
   MessageOutput.println("Starting LoraDTU");
   delay(1500);     // When the power is turned on, a delay is required.

   // Initialize file system
   MessageOutput.print("Initialize FS... ");
   if (!LittleFS.begin(false)) { // Do not format if mount failed
       MessageOutput.print("failed... trying to format...");
       if (!LittleFS.begin(true)) {
           MessageOutput.print("success");
       } else {
           MessageOutput.print("failed");
       }
   } else {
       MessageOutput.println("done");
   }

   // Read configuration values
   MessageOutput.print("Reading configuration... ");
   if (!Configuration.read()) {
       MessageOutput.print("initializing... ");
       Configuration.init();
       if (Configuration.write()) {
           MessageOutput.print("written... ");
       } else {
           MessageOutput.print("failed... ");
       }
   }
   auto& config = Configuration.get();
   MessageOutput.println("done");

   // Load PinMapping
   MessageOutput.print("Reading PinMapping... ");
   if (PinMapping.init(String(Configuration.get().Dev_PinMapping))) {
       MessageOutput.print("found valid mapping ");
   } else {
       MessageOutput.print("using default config ");
   }
   const auto& pin = PinMapping.get();
   MessageOutput.println("done");

   // Initialize WiFi
   MessageOutput.print("Initialize Network... ");
   NetworkSettings.init(scheduler);
   MessageOutput.println("done");
   NetworkSettings.applyConfig();

   // Initialize NTP
   MessageOutput.print("Initialize NTP... ");
   NtpSettings.init();
   MessageOutput.println("done");

   // Initialize WebApi
   MessageOutput.print("Initialize WebApi... ");
   WebApi.init(scheduler);
   MessageOutput.println("done");

   // Initialize Display
   MessageOutput.print("Initialize Display... ");
   Display.init(
      scheduler,
      static_cast<DisplayType_t>(pin.display_type),
      pin.display_data,
      pin.display_clk,
      pin.display_cs,
      pin.display_reset);
   Display.setOrientation(config.Display.Rotation);
   Display.enablePowerSafe = config.Display.PowerSafe;
   Display.enableScreensaver = config.Display.ScreenSaver;
   Display.setContrast(config.Display.Contrast);
   Display.setLanguage(config.Display.Language);
   Display.setStartupDisplay();
   MessageOutput.println("done");

   // Initialize Single LEDs
   MessageOutput.print("Initialize LEDs... ");
   LedSingle.init(scheduler);
   MessageOutput.println("done");

   // Check for default DTU serial
   MessageOutput.print("Check for default DTU serial... ");
   if (config.Dtu.Serial == DTU_SERIAL) {
       MessageOutput.print("generate serial based on ESP chip id: ");
       const uint64_t dtuId = Utils::generateDtuSerial();
       MessageOutput.printf("%0x%08x... ",
           ((uint32_t)((dtuId >> 32) & 0xFFFFFFFF)),
           ((uint32_t)(dtuId & 0xFFFFFFFF)));
       config.Dtu.Serial = dtuId;
       Configuration.write();
   }
   MessageOutput.println("done");

   // lora
   spiV.begin(LORA_SCLK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN);
   delay(1500);
   pinMode(spiV.pinSS(), OUTPUT); //HSPI SS // neu
   LoRa.setPins(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
   if (!LoRa.begin(868E6)) {
      Serial.println("Starting LoRa failed!");
      //displayText("Starting LoRa failed!");
      delay(5000);
   } else {      
      LoRa.setSpreadingFactor(6);  // Supported values are between `6` and `12`. defaults to `7`
      // If a spreading factor of `6` is set, implicit header mode must be used to transmit and receive packets.
      LoRa.setCodingRate4(5); // default `5`, Supported values `5` - `8`, correspond to coding rates of `4/5` and `4/8
      LoRa.disableCrc();  // by default a CRC is not used
      // LoRa.parsePacket(9); // in der loop, aktiviert Implicit Header Mode mit fixer Länge
      LoRa.setPreambleLength(8); // default 8, Supported values are between `6` and `65535`
      LoRa.setSyncWord(0x12); // byte value to use as the sync word, defaults to `0x12`
      LoRa.setSignalBandwidth(125E3);  // signal bandwidth in Hz, defaults to `125E3`
      // Supported values are `7.8E3`, `10.4E3`, `15.6E3`, `20.8E3`, `31.25E3`, `41.7E3`, `62.5E3`, `125E3`, `250E3`, and `500E3`.
      Serial.println("LoRa was started!");
      //displayText("LoRa was started!");
      delay(1000);
   }

   CloudLogger::begin();

   // pin 2 für display-ein-aus-taster
   pinMode(2, INPUT_PULLUP);

   InverterSettings.init(scheduler);
   Datastore.init(scheduler);

   uint32_t now = millis();
   cmdwaitperiodstart = now;
   taktstart = now;
   _lastWattaenderung = now;
   lastLoRaMillis = now;
}
///////////////////////////////////////// ende von setup //////////////////////////////////

int func_Wattanpassung(int temp) {
    if (temp > 30) {
        return temp * 0.7;
    } else if (temp > 20) {
        return temp * 0.8;
    } else if (temp > 10) {
        return temp * 0.88;
    } else if (temp > 5) {
        return temp * 0.95;
    }
    return temp; // Keine Änderung, falls temp <= 5
}


void loop()
{
   scheduler.execute();

   if (nureinmal == false){
      inv = Hoymiles.getInverterByPos(0);
      if (inv){
          nureinmal = true;
      }
   }

   if(jetzteinschalten == true){
      if ((millis() - _lastWattaenderung) < POWER_ON_OFF_WAIT) { // wartezeit, z.b. 3 s
         return;
      }
      if (inv){
         inv->sendPowerControlRequest(1); // hoymiles einschalten
         jetzteinschalten = false;
         MessageOutput.printf("#################### Einschaltbefehl gesendet ########################\r\n");
         hoy_aktiv = true;
         cmdwaitperiod = true;
         cmdwaitperiodstart = millis();
      }
   }

   if(jetztabschalten == true){
      if ((millis() - _lastWattaenderung) < POWER_ON_OFF_WAIT) { // wartezeit, z.b. 3 s
         return;
      }
      if (inv){
         inv->sendPowerControlRequest(0); // hoymiles ausschalten
         jetztabschalten = false;
         MessageOutput.printf("!!!!!!!!!!!!!!!!!!! Ausschaltbefehl gesendet !!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
         hoy_aktiv = false;
         akPower = 0;
         NewPower = 0;
         cmdwaitperiod = true;
         cmdwaitperiodstart = millis();
      }
   }

   int packetSize = LoRa.parsePacket(9); // aktiviert Implicit Header Mode mit fixer Länge
   if (packetSize == 9) { // received a packet
      MessageOutput.printf("XXXXXXXXXX packet received XXXXXXXXX  ");

      i = 0;
      while (LoRa.available() && i < 9) {
         payload[i++] = LoRa.read();
      }

      //MessageOutput.printf("\n");
      //for (int i = 0; i < 9; i++) {
      //   MessageOutput.printf("%02X ", payload[i]);
      //}
      //MessageOutput.printf("\n");
      //MessageOutput.printf("\n");

      if (i == 9 && memcmp(payload, identifier, 6) == 0) {       // Vergleiche Identifier
         //#################### gültige message erhalten ########################################
         lastLoRaMillis = millis(); // Zeitstempel bei gültigem Paket setzen
         // Rekonstruiere powerSaldo (signed int32 aus 3 Bytes)
         powerSaldo = ((int32_t)payload[6] << 16) | (payload[7] << 8) | payload[8];
         if (powerSaldo & 0x800000) { // Falls negativ: prüfe höchstes Bit im 24-Bit-Wert
            powerSaldo |= 0xFF000000;  // Sign extension
         }

         if (powerSaldo < Configuration.get().AutoLimit.MaxLeistung * 8 * -1.15) { // Falls negativer als möglich: verwerfen
            altpowerSaldo = 0; 
            return;
         }
         if (powerSaldo > 23000) { // Falls höher als möglich: verwerfen
            altpowerSaldo = 0; 
            return;
         }
         // ausreisser detektieren und verwerfen
         if (powerSaldo > 600) { // verdächtig hoher wert
            if (altpowerSaldo <= 600){ // altpowerSaldo ist zunächst 0 --> altpowerSaldo setzen --> return
               // wenn altpowerSaldo gesetzt, ist er > 600 --> return wird übersprungen, wert wird akzeptiert 
               altpowerSaldo = powerSaldo; 
               return; // verdächtig hoher wert wird verworfen
            }
         } else {
            altpowerSaldo = 0; 
         }


         // zum senden von powersaldo an browser -----
         WebApi.publishPowerSaldo(powerSaldo);
         //-------------------------------------------
         MessageOutput.printf("Lora-Empfang: %d W\r\n", powerSaldo);
         wattprozent = powerSaldo / 8;
         if (wattprozent < -99){
            wattprozent = -99;
         }
         if (wattprozent > 99){
            wattprozent = 99;
         }

         wattprozent = func_Wattanpassung(wattprozent);
         MessageOutput.printf("  Wattprozent (angepasst): %d %%\r\n", wattprozent);

         //Serial.print("Batteriespannung: ");
         //Serial.println(dcVoltage_float);
         MessageOutput.printf("Batteriespannung nach Lora-Empfang: %f V\r\n", dcVoltage_float);

         NewPower = akPower + wattprozent;
         wattprozent = 0; // damit beim nächsten durchlauf ohne explizites neusetzen von wattprozent nix geändert wird

         if (NewPower < 2){
            NewPower = 2;
         }


         if (NewPower > Configuration.get().AutoLimit.MaxLeistung){ 
            NewPower = Configuration.get().AutoLimit.MaxLeistung; // max. leistung im automatik-modus
         }
         MessageOutput.printf("akPower: %d %%  ", akPower);
         MessageOutput.printf("NewPower: %d %%\r\n", NewPower);
         MessageOutput.printf("\r\n");

         if (inv ) {
            inverterTemp = inv->Statistics()->getChannelFieldValue(TYPE_INV, CH0, FLD_T);
         }

         CloudSample s;
         s.timeStamp = time(nullptr);
         s.powerSaldo = powerSaldo;
         s.watts = watts;
         s.dcVoltage = dcVoltage_float;
         s.invTemp = inverterTemp;
         s.yield_day = YieldDay_float;
         CloudLogger::addSample(s);

         if (aufmachen == true){
            // einspeisung bei voller batterie -------------------------------------------------------------
            for (int i = 0; i < NUM_LEVELS; i++) {
               if (dcVoltage_float < VOLTAGE_LEVELS[i]) {
                     upperThresholds[i] = VOLTAGE_LEVELS[i] + HYSTERESIS_VOLTAGE;
               }
               if (dcVoltage_float > upperThresholds[i] && NewPower < POWER_LEVELS[i]) {
                     upperThresholds[i] = VOLTAGE_LEVELS[i];
                     NewPower = POWER_LEVELS[i];
                     MessageOutput.printf("NewPower weil Batterie voll: %d Wprozent\r\n", NewPower);
               }
            }
         }
         // ende einspeisung bei voller batterie --------------------------------------------------------
      } else {
         MessageOutput.printf("verstümmeltes Paket oder Paket nicht für mich !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n");
         // LEEREN des Lora-Empfang-Puffers, falls noch Bytes im Puffer sind (i != 9 oder packetSize <> 9)
         if (LoRa.available()) {
            while (LoRa.available()) {
                  LoRa.read(); // Einfach auslesen und ignorieren
            }
         }
      }   

      if ((millis() - _lastWattaenderung) > POWER_ADJUST_WAIT) { // nach wattänderung wartezeit, z.b. 4,5 s
         if ((NewPower != akPower) && (hoy_aktiv == true)){
            if (LoraOn == true){
               if (inv){
                  inv->sendActivePowerControlRequest(static_cast<float>(NewPower), PowerLimitControlType::RelativNonPersistent);
                  MessageOutput.printf("###################### Wattänderung ######################\r\n");
                  if(akPower - NewPower > 20){ // starke reduktion der leistung
                     abschaltwert_belassen = true;
                  }
                  akPower = NewPower;
                  _lastWattaenderung = millis();
               }
            }
         }
      }
   }
   // received a packet ENDE
   //################################## Hoy_Switch ##################################################
   if (cmdwaitperiod == true){
      if ((millis() - cmdwaitperiodstart) > CMD_WAIT_PERIOD) { // wartezeit nach ein-/ausschalten nach z.b. 6 sekunden beenden
         cmdwaitperiod = false;
      }
   }

   if (cmdwaitperiod == false){
      if ((millis() - taktstart) > TAKT_INTERVAL) { // takt für den nachfolgenden code, z.b. 2 s
         taktstart = millis();
         bool reachable = false;
         bool fresh = false;
         bool loraFresh = (millis() - lastLoRaMillis) < LORA_TIMEOUT; // neue lora-daten innerhalb von z.b. 30 s --> true
         if (inv){
            reachable = inv->isReachable();
            fresh = (millis() - inv->Statistics()->getLastUpdate()) < INV_DATA_TIMEOUT; // z.b. 30 s
         }   
         if (!reachable) {
            delay(1);
            return; // inverter offline --> do nothing
         }
         else if (!fresh || !loraFresh) { // inverter reachable but no recent data OR no lora data
            hoy_aktiv = false;
            akPower = 0; // no recent data --> wahrscheinlich produziert hoymiles nicht
            NewPower = 2; // leistung auf minimum setzen, zur sicherheit
            inv->sendActivePowerControlRequest(static_cast<float>(NewPower), PowerLimitControlType::RelativNonPersistent);
            _lastWattaenderung = millis();
         }
         else {  // data valid
            if (inv){
               dcVoltage_float = inv->Statistics()->getChannelFieldValue(TYPE_DC, CH0, FLD_UDC);
            }
            //MessageOutput.printf("Batteriespannung im 2-Sek-Takt: %f V\r\n", dcVoltage_float);
            //MessageOutput.printf("NewPower im 2-Sek-Takt: %d Wprozent\r\n", NewPower);
            watts = Datastore.getTotalAcPowerEnabled();
            //Serial.print("watts: ");
            //Serial.println(watts);
            if (watts > 0){
               hoy_aktiv = true;
               MessageOutput.printf("\r\n");
               MessageOutput.printf("hoy_aktiv auf true gesetzt !!! \r\n");
            } else {
               hoy_aktiv = false;
               akPower = 0;
            }
         }

         if (man_aend_wattprozent == true){ // manuelle leistungänderung ist in WebApi_limit.cpp erfolgt
            MessageOutput.printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
            MessageOutput.printf("manuelle wattänderung\r\n");
            MessageOutput.printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");
            man_aend_wattprozent = false;  
            // damit manuelle leistungänderung korrekt verarbeitet wird ("wattprozent" wurde in WebApi_limit.cpp manuell gesetzt)
            akPower = wattprozent;
            NewPower = wattprozent;
            _lastWattaenderung = millis();
            wattprozent = 0; // damit beim nächsten durchlauf ohne explizites neusetzen von wattprozent nix geändert wird
         };

         // ---------------------------abschaltwerte -----------------------------------
         if (abschaltwert_belassen == true){ 
            zaehl2++;
            MessageOutput.printf("zaehl2: %d \r\n", zaehl2);
            if(zaehl2 > 15){
               abschaltwert_belassen = false;
               zaehl2 = 0;
            }
         }
         if (abschaltwert_belassen == false) {  // wenn true: abschaltwert bleibt unverändert 
            // Berechnet Abschaltschwelle basierend auf aktueller Leistung
            // Formel: V = -0.0008*P² - 0.008*P + 25.4
            // Ergibt: 25.4V bei 0% und 22.0V bei 60%
            // Logik: Bei höherer Leistung früher abschalten (Batterieschutz)
            abschaltwert = (-0.0008 * NewPower * NewPower) - (0.008 * NewPower) + 25.4;
            if (abschaltwert < 22.6) abschaltwert = 22.6;
         }

         MessageOutput.printf("abschaltwert: %f V\r\n", abschaltwert);
         MessageOutput.printf("dcVoltage_float: %f V\r\n", dcVoltage_float);
         if (dcVoltage_float > 26.2 && (hoy_aktiv == false) && NewPower > 0 && (LoraOn == true)){
            //inv->sendRestartControlRequest(); // hoymiles restarten
            //Serial.println("&&&&& Restart-Befehl &&&&&&&&&&&&&&&");
            //MessageOutput.printf("&&&&& Restart-Befehl &&&&&&&&&&&&&&&\r\n");
            if (inv){
               NewPower = 5; // erst leistung einstellen
               inv->sendActivePowerControlRequest(static_cast<float>(NewPower), PowerLimitControlType::RelativNonPersistent);
               _lastWattaenderung = millis();
               jetzteinschalten = true;
               MessageOutput.printf("#### Einschaltung gewünscht ####################\r\n");
            }
         }

         if ((dcVoltage_float < abschaltwert) && hoy_aktiv){ // ausschaltung
            if (inv){
               NewPower = 5;
               inv->sendActivePowerControlRequest(static_cast<float>(NewPower), PowerLimitControlType::RelativNonPersistent);
               _lastWattaenderung = millis();
               MessageOutput.printf("#### Watt auf 5 Prozent setzen vor Abschaltung  ####################\r\n");
               jetztabschalten = true;
               MessageOutput.printf("#### Abschaltwert-Ausschaltung gewünscht  ####################\r\n");
            }
         }

         if (dcVoltage_float < 22.5){  // notfall-ausschaltung zum batterieschutz
            // sorgt auch für initiale ausschaltung beim starten der LoraDTU
            if (inv){
               NewPower = 5;
               inv->sendActivePowerControlRequest(static_cast<float>(NewPower), PowerLimitControlType::RelativNonPersistent);
               _lastWattaenderung = millis();
               MessageOutput.printf("#### Watt auf 5 Prozent setzen vor Abschaltung  ####################\r\n");
               jetztabschalten = true;
               MessageOutput.printf("#### Batterieschutz-Ausschaltung gewünscht  ####################\r\n");
            }
         }
      }
   }
}
