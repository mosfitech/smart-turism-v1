#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <AltSoftSerial.h>

#include "SIM800L.h"


#define SIM800_RX_PIN 2
#define SIM800_TX_PIN 3
#define SIM800_RST_PIN 6
#define Relay_Lock 12
#define Relay_Warning 13

boolean newData;
String latitude;      //Latitude (North - South)
String longitude;     //Longitude (East - West)

const char APN[] = "3gprs";
const char URL0[] = "http://34.101.234.235/api/vehicle/status/1";
const char URL[] = "http://34.101.234.235/api/vehicle/status-gmaps/1?";
const char CONTENT_TYPE[] = "application/x-www-form-urlencoded";
const char PAYLOAD[] = "lat=-5.346021&long=105.300720";


String reportString ="";

SIM800L* sim800l;
AltSoftSerial neogps;
TinyGPSPlus gps;
void setup() {
  pinMode(Relay_Lock, OUTPUT);
  pinMode(Relay_Warning, OUTPUT);
  
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  while(!Serial);
  neogps.begin(9600);
  // Initialize a SoftwareSerial
  SoftwareSerial* serial = new SoftwareSerial(SIM800_RX_PIN, SIM800_TX_PIN);
  serial->begin(9600);
  delay(1000);
  
  // Initialize SIM800L driver with an internal buffer of 200 bytes and a reception buffer of 512 bytes, debug disabled
  sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 200, 512);

  setupModule();
}
 
void loop() {
    boolean newData = false;
    for (unsigned long start = millis(); millis() - start < 2000;){
      while (neogps.available()){
        if (gps.encode(neogps.read())){
          newData = true;
          break;
        }
      }
    }

  // Establish GPRS connectivity (5 trials)
  bool connected = false;
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    connected = sim800l->connectGPRS();
  }

  // Check if connected, if not reset the module and setup the config again
  if(connected) {
    Serial.print(F("GPRS connected with IP "));
    Serial.println(sim800l->getIP());
  } else {
    Serial.println(F("GPRS not connected !"));
    Serial.println(F("Reset the module."));
    sim800l->reset();
    setupModule();
    return;
  }


   Serial.println(F("Start HTTP GET..."));
   uint16_t ra = sim800l->doGet(URL0, 10000);
   if(ra == 200) {
//    Serial.print("relay kunci on");
//    Serial.print(F("HTTP GET successful ("));
    Serial.print(sim800l->getDataSizeReceived());
    Serial.println(F(" bytes)"));
    Serial.print(F("Received : "));
    Serial.println(sim800l->getDataReceived());
    
         String response = sim800l->getDataReceived();
         String Status_1 = "1";
         String Status_2 = "2";
    
     if(response == Status_1){   
      postGPS();
     }else {
        Serial.println("relay kunci off, vehicle mati");
        digitalWrite(Relay_Lock, LOW);
     }
   }else{
    // Failed...
    Serial.print(F("HTTP GET error "));
    Serial.println(ra);
   }
  delay(1000);
}

void setupModule() {
    // Wait until the module is ready to accept AT commands
  while(!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }
  
  Serial.println(F("Setup Complete!"));

  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while(signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
  }
  Serial.print(F("Signal OK (strenght: "));
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
  }
  Serial.println(F("Network registration OK"));
  delay(1000);

  // Setup APN for GPRS configuration
  bool success = sim800l->setupGPRS(APN);
  while(!success) {
    success = sim800l->setupGPRS(APN);
    delay(5000);
  }
  Serial.println(F("GPRS config OK"));
}


void postGPS () {
      if(newData){
      newData = false;
      latitude = String(gps.location.lat(), 6);
      longitude = String(gps.location.lng(), 6);
      Serial.println(latitude);
 
         String payload = "lat=";
                payload += latitude;
                payload +="&long=";
                payload += longitude;
                Serial.println(payload);
                
          char URI[80];
          
          sprintf(URI,"%s%s",URL,payload.c_str());
          Serial.println(URI);
        
        Serial.println(F("Start HTTP POST..."));
        // Do HTTP POST communication with 10s for the timeout (read and write)
        uint16_t rc = sim800l->doPost(URI, CONTENT_TYPE, payload.c_str(), 10000, 10000);
         if(rc == 200) {
   
          // Success, output the data received on the serial
          Serial.print(F("HTTP POST successful ("));
          Serial.print(sim800l->getDataSizeReceived());
          Serial.println(F(" bytes)"));
          Serial.print(F("Received : "));
          Serial.println(sim800l->getDataReceived());
            String response_gps = sim800l->getDataReceived();
            String response1 = "1";
            String response2 = "2";
//               if(response_gps == response2){
////                  Serial.println("relay bel aktif");
////                  for(int i = 0; i <=5; i++){
////                    digitalWrite(Relay_Warning, HIGH);
////                    delay(1000);
////                    digitalWrite(Relay_Warning, LOW);
////                    delay(1000);
////                  }
//               } else if(response_gps.equals(response1)){
//                  Serial.println("Kendaraan Dalam area peminjaman");
//               }else {
//                Serial.println("Kendaraan sedag tidak aktif");
//               }

        } else {
          // Failed...
          Serial.print(F("HTTP POST error "));
          Serial.println(rc);
          relayLock();
        }
      }
}

void relayLock(){
 
//     digitalWrite(Relay_Lock, HIGH);
//     delay(8000);
//     digitalWrite(Relay_Lock, LOW);
}
