#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>
#include "RTClib.h"
#include <SPI.h>
#include "ThingsBoard.h"
#include <SoftwareSerial.h>
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
float Temp;

#define SCK 18
#define MOSI 23
#define DC 19 //19
#define RST 27
#define CS  5 //5

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

#define OLED_RESET     -1// Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


SoftwareSerial serialGsm(14, 12); // RX17, TX 16


uint8_t id;
int hr;
int Min;
String Entry;

// Initialize GSM modem
TinyGsm modem(serialGsm);

// Initialize GSM client
TinyGsmClient client(modem);

// Initialize ThingsBoard instance
ThingsBoard tb(client);

//#define TOKEN  "tAlL9mnM158me5tU3yJx"
//#define THINGSBOARD_SERVER "demo.thingsboard.io"

#define TOKEN               "582Bj9vQpGMM9bsvtRKf"
#define THINGSBOARD_SERVER  "thingsboard.cloud"


const char apn[]  = "safaricom";
const char user[] = "saf";
const char pass[] = "data";

float ID;



// Set to true, if modem is connected
bool modemConnected = false;

void setup() {
  
  Serial.begin(115200);
  Serial2.begin(57600);
 
  delay(100);
  mlx.begin();
  finger.begin(57600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

//   Show initial display buffer contents on the screen --
//   the library initializes this with an Adafruit splash screen.
  display.display();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(30, 10);
  display.println(("KE0438"));
  display.display();
  delay(10000);
  serialGsm.write("AT+IPR=9600\r\n");
  serialGsm.end();
  serialGsm.begin(9600);

  Serial.println(F("Initializing modem..."));
  //modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print(F("Modem: "));
  Serial.println(modemInfo);
  delay(500);

  if (finger.verifyPassword()) {
    Serial.println("Found Fingerprint Sensor!");
  } else {
    Serial.println("Did not find the Fingerprint  sensor:(");
    while (1) {
      delay(1);
    }

  }
  delay(200);
#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
delay(200);
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }

  //  display.clearDisplay();
  //  display.drawBitmap(0, 0, logo_bmp, 64, 32, WHITE);
  //  display.display();


}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}


void loop() {
  // put your main code here, to run repeatedly:

  if (!modemConnected) {
    Serial.print(F("Waiting for network..."));
    if (!modem.waitForNetwork()) {
      Serial.println(F(" fail"));
      delay(10000);
      return;
    }
    Serial.println(F(" OK"));

    Serial.print(F("Connecting to "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println(F(" fail"));
      delay(10000);
      return;
    }




    modemConnected = true;
    Serial.println(F(" OK"));
  }

  if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }
  getFingerprintID();
  delay(200);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  DateTime now = rtc.now();

  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(30, 15);
      display.println(("Scanning"));
      display.display();
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(20, 0);
      display.println(("Place Finger on"));
      display.setCursor(40, 10);
      display.println(("Scanner"));
      display.display();
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(30, 15);
      display.println(("Scanning"));
      display.display();
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(20, 0);
      display.println(("NO Match"));
      display.setCursor(10, 10);
      display.println(("Try Again "));
      display.display();
      delay(1000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(20, 0);
      display.println(("NO Match"));
      display.setCursor(10, 10);
      display.println(("Try Again "));
      display.display();
      delay(1000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(20, 0);
      display.println(("NO Match"));
      display.setCursor(10, 10);
      display.println(("Try Again "));
      display.display();
      delay(1000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(20, 0);
      display.println(("NO Match"));
      display.setCursor(10, 10);
      display.println(("Try Again "));
      display.display();
      delay(1000);
      return p;
    default:
      Serial.println("Unknown error");
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(20, 0);
      display.println(("NO Match"));
      display.setCursor(10, 10);
      display.println(("Try Again "));
      display.display();
      delay(1000);
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Temp = mlx.readObjectTempC();
    Temp = Temp +3;
    Serial.println("Found a print match!");
    hr = now.hour();
    Min = now.minute();
    ID = finger.fingerID;
    Serial.print("Temparature is: "); Serial.println(Temp);
    if (Temp < 38) {
      if (hr < 10){
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(10, 0);
      display.println(("Access Granted"));
      display.setCursor(10, 10);
      display.println(("Temp is: "));
      display.setCursor(60, 10);
      display.println(Temp);
      display.setCursor(10, 20);
      display.println(("Time: "));
      display.setCursor(50, 20);
      display.print(hr);
      display.setCursor(60, 20);
      display.print(":");
      display.setCursor(65, 20);
      display.println(Min);
      display.display();
      Serial.print("HOUR is:  ");
      Serial.println(hr);
       const int data_items = 4;
    Telemetry data[data_items] = {
      {"Temperature", Temp},
      {"ID", ID},
      {"Entry/Exit", 0},
      {"Hour", hr},
      
    };
      tb.sendTelemetry(data, data_items);
      tb.loop();
    }
 
 
    else if (hr  > 10 || hr == 10){
      display.clearDisplay();
      display.setTextSize(1.8);
      display.setTextColor(WHITE);
      display.setCursor(10, 0);
      display.println(("Access Granted"));
      display.setCursor(10, 10);
      display.println(("Temp is: "));
      display.setCursor(60, 10);
      display.println(Temp);
      display.setCursor(10, 20);
      display.println(("Time: "));
      display.setCursor(50, 20);
      display.print(hr);
      display.setCursor(60, 20);
      display.print(":");
      display.setCursor(65, 20);
      display.println(Min);
      display.display();
      Serial.print("HOUR is:  ");
      Serial.println(hr);
     const int data_items = 4;
    Telemetry data[data_items] = {
      {"Temperature", Temp},
      {"ID", ID},
      {"Entry/Exit", 1},
      {"Hour", hr},
      
    };
      tb.sendTelemetry(data, data_items);
    }
      
    }

    tb.loop();

    delay(5000);
  }
  else if (Temp > 38) {
    display.clearDisplay();
    display.setTextSize(1.8);
    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.println(("Access Denied"));
    display.setCursor(10, 10);
    display.println(("Temp is: HIGH "));
    display.setCursor(60, 10);
    display.println(Temp);
    display.setCursor(10, 20);
    display.println(("Time: "));
    display.setCursor(50, 20);
    display.print(hr);
    display.setCursor(60, 20);
    display.print(":");
    display.setCursor(65, 20);
    display.println(Min);
    display.display();
    const int data_items = 4;
    Telemetry data[data_items] = {
      {"Temperature", Temp},
      {"ID", ID},
      {"Entry/Exit", 1},
      {"Hour", hr},
      
    };
    tb.sendTelemetry(data, data_items);
    tb.loop();

    delay(5000);
}
  else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    display.clearDisplay();
    display.setTextSize(1.8);
    display.setTextColor(WHITE);
    display.setCursor(20, 0);
    display.println(("NO Match"));
    display.setCursor(10, 10);
    display.println(("Try Again "));
    display.display();
    delay(1000);
    return p;
  }

  else {
    Serial.println("Unknown error");
    display.clearDisplay();
    display.setTextSize(1.8);
    display.setTextColor(WHITE);
    display.setCursor(20, 0);
    display.println(("NO Match"));
    display.setCursor(10, 10);
    display.println(("Try Again "));
    display.display();
    delay(1000);
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
