#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <RFM69_OTA.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <SPIFlash.h>      //get it here: https://github.com/lowpowerlab/spiflash
#include <SPI.h>           //included with Arduino IDE (www.arduino.cc)

//****************************************************************************************************************
#define NODEID          1 //the ID of this node
#define NETWORKID     100 //the network ID of all nodes this node listens/talks to
#define FREQUENCY     RF69_433MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY    "sampleEncryptKey" //identical 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
#define ACK_TIME       30  // # of ms to wait for an ack packet

//*****************************************************************************************************************************
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75  //target RSSI for RFM69_ATC (recommended > -80)

//*****************************************************************************************************************************
// Serial baud rate must match your Pi/host computer serial port baud rate!
#define SERIAL_EN     //comment out if you don't want any serial verbose output

//*****************************************************************************************************************************
#ifdef __AVR_ATmega1284P__
//#define LED           15 // Moteino MEGAs have LEDs on D15
#define FLASH_SS      23 // and FLASH SS on D23
#else
//#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8
#endif

#ifdef SERIAL_EN
#define DEBUG(input)   {Serial.print(input); delay(1);}
#define DEBUGln(input) {Serial.println(input); delay(1);}
#else
#define DEBUG(input);
#define DEBUGln(input);
#endif

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit Windbond FlashMEM chip


//*****************************************************************************************************************************
int alertPins[] = {4, 5}; // digital pins in array for pulsing | vibration and led
int pulseTimer = 60;

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(alertPins, OUTPUT);

  radio.initialize(FREQUENCY, NODEID, NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", radio.getFrequency() / 1000000);
  DEBUGln(buff);

  if (flash.initialize())
  {
    DEBUGln("SPI Flash Init OK!");
  }
  else
  {
    DEBUGln("SPI FlashMEM not found (is chip onboard?)");
  }
}

byte ackCount = 0;
                byte inputLen = 0;
                char input[64];
                byte buff[61];
                String inputstr;

  void loop()
{
  inputLen = readSerialLine(input);
  inputstr = String(input);
  inputstr.toUpperCase();


  if (radio.receiveDone())
  {
    int rssi = radio.RSSI;
    Serial.println("gate 1");


    //DEBUG('['); DEBUG(radio.SENDERID); DEBUG("] ");
    if (radio.DATALEN > 0)
    {
      Serial.println("gate 2");
      //for (byte i = 0; i < radio.DATALEN; i++)
      //DEBUG((char)radio.DATA[i]);
      DEBUG("   [RSSI:"); DEBUG(rssi); DEBUG("]");
    }

    // Part that when signal fades starts alet function
    if (rssi <= -60)
    {
      alert();
    }
    else
    {
      alertOff();
    }


    CheckForWirelessHEX(radio, flash, false); //non verbose DEBUG

    if (radio.ACKRequested())
    {
      Serial.println("gate 3");
      radio.sendACK();
      DEBUG("[ACK-sent]");
    }
  }
}

void alert()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(alertPins, HIGH);
    delay(pulseTimer);
    for (int i = 0; i < 3; i++)
    {
      digitalWrite(alertPins, LOW);
      delay(pulseTimer);
    }
  }
}

void alertOff()
{
  digitalWrite(alertPins, LOW);
}
