#include <SPI.h>
#include <LoRa.h>
#include <MKRWAN.h>

/* Adaptation pour MKr 1300 2019-04-01 Joseph Ciccarello */
#define RST_PIN         6 //9          // Configurable, see typical pin layout above
#define SS_PIN          7 //10         // Configurable, see typical pin layout above

#define PACKET_SIZE 6
#define NET_CODE 0x34
#define EMITTER_CODE 0x1A
#define STATION 0x00
#define OPEN 0x01

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
  LoRa.enableCrc();
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
  static unsigned long card_uid;
  static boolean sending = false;

  // TODO Lire la carte
  if(mfrc522.PICC_IsNewCardPresent()) {
    card_uid = getID();
    if(card_uid != -1){
      Serial.print("Card detected, UID: ");
      Serial.println(card_uid);
      //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));
      sending = true;
    }
  }
  
  // Tous les 1000 passages ...
  // Todo : Utiliser le temps sytème pour plus de précision
  if (sending) {    
    sendUID(card_uid);
    counter++;
    sending = false;
  }

  //Serial.print("Check for packet  ");
  readNetwork();
}

void onReceive(int packetSize) {
  if(packetSize != 7) {
    return; // Not a valid packet
  }

  
}

void readNetwork() {
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    byte message[PACKET_SIZE];
    
    byte sourceID = LoRa.read();
    message[0] = sourceID;
    byte destID = LoRa.read();
    message[1] = destID;

    int i = 2;
    message[2] = LoRa.read();
    message[3] = LoRa.read();
    message[4] = LoRa.read();
    message[5] = LoRa.read();

    Serial.print("Received: (" + String(packetSize) + ")");
    printPacket(message);
    Serial.println(" (0x" + String(sourceID, HEX) + " => 0x" + String(destID, HEX) + ")");

    if(destID != EMITTER_CODE) {
      Serial.println("NOT FOR ME !");
      return;
    }

    if(sourceID != STATION) {
      Serial.println("NOT STATION !");
      return;
    }

    if(message[5] == OPEN) {
      Serial.println("Yay ! =)");
      beep();
    } else {
      unauthorized();
    }
  }
}

void unauthorized() {
  for(int i = 0; i < 3; i++) {
    beep();
    delay(50);
  }
}

void printPacket(byte* message) {
  String text = String(message[0], HEX) + ":" + String(message[1], HEX) + ":" + String(message[2], HEX) + ":" + String(message[3], HEX) + ":" + String(message[4], HEX) + ":" + String(message[5], HEX);
  Serial.print(text);
}

void beep() {
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
}

void sendUID(unsigned long uid) {
  digitalWrite(led, HIGH);
  Serial.println("Sending packet: " + String(counter));
  Serial.print("Sending UID : ");
  String textUID = "";
    
  //LoRa.send packet
  LoRa.beginPacket();
  //LoRa.print("Msg N° : ");
  //LoRa.print(counter);
  LoRa.write(EMITTER_CODE); // Emitter
  LoRa.write(STATION); // Receiver
  // In Data, we put the 4 bytes UID of the card
  LoRa.write(mfrc522.uid.uidByte[0]);
  LoRa.write(mfrc522.uid.uidByte[1]);
  LoRa.write(mfrc522.uid.uidByte[2]);
  LoRa.write(mfrc522.uid.uidByte[3]);

  textUID = String(mfrc522.uid.uidByte[0], HEX) + ":" + String(mfrc522.uid.uidByte[1], HEX) + ":" + String(mfrc522.uid.uidByte[2], HEX) + ":" + String(mfrc522.uid.uidByte[3], HEX) + ";";
  Serial.println(textUID);
  
  //LoRa.write(0x00); // Checksum
  LoRa.endPacket();
  delay(50);

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
