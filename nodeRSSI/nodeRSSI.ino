// **********************************************************************************
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      //get it here: https://www.github.com/lowpowerlab/spiflash

//*********************************************************************************************
#define NODEID        2    // keep UNIQUE for each node on same network
#define NETWORKID     100  // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID     1    // "central" node

//*********************************************************************************************
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80

//*********************************************************************************************
#if defined (MOTEINO_M0) && defined(SERIAL_PORT_USBVIRTUAL)
#define Serial SERIAL_PORT_USBVIRTUAL // Required for Serial on Zero based boards
#endif

int TRANSMITPERIOD = 200; //transmit a packet to gateway so often (in ms)
char payload[] = "1";
char buff[20];
byte sendSize = 0;
boolean requestACK = false;
SPIFlash flash(SS_FLASHMEM, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

void setup() {
  Serial.begin(115200);
  radio.initialize(FREQUENCY, NODEID, NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif

#ifdef ENCRYPTKEY
  radio.encrypt(ENCRYPTKEY);
#endif

#ifdef FREQUENCY_EXACT
  radio.setFrequency(FREQUENCY_EXACT); //set frequency to some custom frequency
#endif

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY == RF69_433MHZ ? 433 : FREQUENCY == RF69_868MHZ ? 868 : 915);
  Serial.println(buff);

  if (flash.initialize())
  {
    Serial.print("SPI Flash Init OK ... UniqueID (MAC): ");
    flash.readUniqueId();
    for (byte i = 0; i < 8; i++)
    {
      Serial.print(flash.UNIQUEID[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
  }
  else
    Serial.println("SPI Flash MEM not found (is chip soldered?)...");

#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN, LOW);
}

long lastPeriod = 0;

void loop()
{
  //check for any received packets
  if (radio.receiveDone())
  {
    Serial.print('['); Serial.print(radio.SENDERID, DEC); Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:"); Serial.print(radio.RSSI); Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
    }
    Blink(LED_BUILTIN, 3);
    Serial.println();
  }


  // Sending data
  int currPeriod = millis() / TRANSMITPERIOD;
  if (currPeriod != lastPeriod)
  {
    lastPeriod = currPeriod;

    //send FLASH id
    if (sendSize == 0)
    {
      sprintf(buff, "FLASH_MEM_ID:0x%X", flash.readDeviceId());
      byte buffLen = strlen(buff);
      if (radio.sendWithRetry(GATEWAYID, buff, buffLen))
        Serial.print(" ok! 1");
      else Serial.print(" nothing... 1");
    }
    else
    {
      if (radio.sendWithRetry(GATEWAYID, payload, sendSize))
        Serial.print(" ok! 2");
      else Serial.print(" nothing...2");
    }
    sendSize = (sendSize + 1) % 31;
    Serial.println();
    Blink(LED_BUILTIN, 3);
  }
}
