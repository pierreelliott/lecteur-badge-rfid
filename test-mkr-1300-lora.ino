#include <SPI.h>
#include <LoRa.h>
#include <MKRWAN.h>

/* Adaptation pour MKr 1300 2019-04-01 Joseph Ciccarello */
#define RST_PIN         6 //9          // Configurable, see typical pin layout above
#define SS_PIN          7 //10         // Configurable, see typical pin layout above
#define NET_CODE 0x5A
#define EMITTER_CODE 0x1A

#include <SPI.h>

//This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
#include <MFRC522.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance


const long freq = 868E6;
int counter = 1;
const int led = 6;
LoRaModem modem;


void setup()
{
  Serial.begin(9600);
  while (!Serial);  // On attend que le port série (série sur USBnatif) soit dispo

  modem.dumb();     // On passe le modem en mode transparent
  
  pinMode(led, OUTPUT);
  LoRa.setPins(LORA_IRQ_DUMB, 6, 1); // set CS, reset, IRQ pin
  LoRa.setTxPower(17, PA_OUTPUT_RFO_PIN);
  LoRa.setSPIFrequency(125E3);
  LoRa.setSignalBandwidth(31.25E3);
  LoRa.setSpreadingFactor(9);
  LoRa.setSyncWord(NET_CODE);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(65535);
  LoRa.begin(freq);
  Serial.println("LoRa Sender");

  if (!LoRa.begin(freq))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.print("LoRa Started, F = ");
  Serial.println(freq);

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}



void loop() {
  static int count;
  static unsigned long time = millis();
  static unsigned long card_uid;
  count ++;
  static boolean sending = false;

  // TODO Lire la carte
  if(mfrc522.PICC_IsNewCardPresent()) {
    Serial.println("Test 1");
    card_uid = getID();
    if(card_uid != -1){
      Serial.print("Card detected, UID: ");
      Serial.println(card_uid);
      mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
      sending = true;
    }
  }
  
  // Tous les 1000 passages ...
  // Todo : Utiliser le temps sytème pour plus de précision
  if (sending) {    
    sendUID(card_uid);
    counter++;
    time = millis();
    sending = false;
  }

  //Serial.print("Check for packet  ");
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("RX packet size =  ");
    Serial.println(packetSize);
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    LoRa.endPacket();
  }
}

void readNetwork() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    byte net_code = LoRa.read();
    if(net_code != 0x5A) {
      return;
    }

    byte emitter = LoRa.read();
    byte receiver = LoRa.read();

    if(receiver != EMITTER_CODE) {
      Serial.println("Message on network (not for me) received.");
      return;
    }

    Serial.print("/!\ Message received from " + String(emitter, HEX));
    Serial.print("Message: ");
    while (LoRa.available()) {
      Serial.print(String(LoRa.read(), HEX));
    }
    Serial.println("");
    Serial.println("============================");
  }
}

void sendUID(unsigned long uid) {
  digitalWrite(led, HIGH);
  Serial.print("Sending packet: ");
  Serial.println(counter);
  Serial.print("Sending UID : ");
    
  //LoRa.send packet
  LoRa.beginPacket();
  //LoRa.print("Msg N° : ");
  //LoRa.print(counter);
  LoRa.write(EMITTER_CODE); // Emitter
  LoRa.write(0x00); // Receiver
  // In Data, we put the 4 bytes UID of the card
  for(unsigned int i = 0; i < 4; i++) {
    LoRa.write(mfrc522.uid.uidByte[i]);
    Serial.print(String(mfrc522.uid.uidByte[i], HEX) + " ");
  }
  Serial.println("| End;");
  //LoRa.print(uid);
  LoRa.write(0x00); // Checksum
  LoRa.endPacket();

  digitalWrite(led, LOW);
}

unsigned long getID(){
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return -1;
  }
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  return hex_num;
}
