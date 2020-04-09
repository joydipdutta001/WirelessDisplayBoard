#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <DHT.h>


#include "FirebaseESP8266.h"
//#include <FirebaseArduino.h>
#include "time.h";


//////////////Mention Your SSID,Password of Wifi and the firebase credentials////////////////////
#define WIFI_SSID "$$$$$$$$"
#define WIFI_PASSWORD "$$$$$$$$$$"
#define FIREBASE_HOST "$$$$$$$$$$$$$$$$"
#define FIREBASE_AUTH "$$$$$$$$$$$$$$$$$"

// set to 1 if we are implementing the user interface pot, switch, etc
#define USE_UI_CONTROL 0

#if USE_UI_CONTROL
#include <MD_UISwitch.h>
#endif

// Turn on debug statements to the serial output
#define DEBUG 0

#if DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    15

//////////Temperature And Humidity Pin define//////////////

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

int Buzz = D3;

// HARDWARE SPI
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// SOFTWARE SPI

FirebaseData firebaseData;

FirebaseJson json;
// Scrolling parameters
#if USE_UI_CONTROL
const uint8_t SPEED_IN = A5;
const uint8_t DIRECTION_SET = 8;  // change the effect
const uint8_t INVERT_SET = 9;     // change the invert

const uint8_t SPEED_DEADBAND = 5;
#endif // USE_UI_CONTROL

uint8_t scrollSpeed = 150;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 20; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  1000
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "Hello! Enter new message?" };
bool newMessageAvailable = true;

#if USE_UI_CONTROL

MD_UISwitch_Digital uiDirection(DIRECTION_SET);
MD_UISwitch_Digital uiInvert(INVERT_SET);

void doUI(void)
{
  // set the speed if it has changed
  {
    int16_t speed = map(analogRead(SPEED_IN), 0, 1023, 10, 150);

    if ((speed >= ((int16_t)P.getSpeed() + SPEED_DEADBAND)) ||
      (speed <= ((int16_t)P.getSpeed() - SPEED_DEADBAND)))
    {
      P.setSpeed(speed);
      scrollSpeed = speed;
      PRINT("\nChanged speed to ", P.getSpeed());
    }
  }

  if (uiDirection.read() == MD_UISwitch::KEY_PRESS) // SCROLL DIRECTION
  {
    PRINTS("\nChanging scroll direction");
    scrollEffect = (scrollEffect == PA_SCROLL_LEFT ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayClear();
    P.displayReset();
  }

  if (uiInvert.read() == MD_UISwitch::KEY_PRESS)  // INVERT MODE
  {
    PRINTS("\nChanging invert mode");
    P.setInvert(!P.getInvert());
  }
}
#endif // USE_UI_CONTROL

void readSerial(void)
{
  static char *cp = newMessage;

  while (Serial.available())
  {
    *cp = (char)Serial.read();
    if ((*cp == '\n') || (cp - newMessage >= BUF_SIZE-2)) // end of message character or full buffer
    {
      *cp = '\0'; // end the string
      // restart the index for next filling spree and flag we have a message waiting
      cp = newMessage;
      newMessageAvailable = true;
    }
    else  // move char pointer to next position
      cp++;
  }
}

String get_time(){

  time_t now;
  time(&now);
  char time_output[30];
  strftime(time_output, 30, "%d-%m-%y %T", localtime(&now));
  return String(time_output);
  
    Serial.println("------------------------------------");


}

void setup()
{
  Serial.begin(115200);
  Serial.print("\n[Parola Scrolling Display]\nType a message for the scrolling display\nEnd message line with a newline");

#if USE_UI_CONTROL
  uiDirection.begin();
  uiInvert.begin();
  pinMode(SPEED_IN, INPUT);


  doUI();
#endif // USE_UI_CONTROL

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

    ///////////////////////////////////////////TimeZone Selection Here//////////////////////////
  configTime(0,0, "pool.ntp.org", "time.nist.gov");
 const char*  timeZone1 = "IST-5:30";
// String timeZone2 ="GMT0BST,M3.5.0/01,M10.5.0/02";
  setenv("TZ", timeZone1, 1);
  ///////////////////////////////////////////////////////////////////////////////
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);

  P.begin();
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
  dht.begin();
  pinMode(Buzz,OUTPUT);
}

void loop()
{
#if USE_UI_CONTROL
  doUI();
#endif // USE_UI_CONTROL
    float h = dht.readHumidity();                                              // Reading temperature or humidity takes about 250 milliseconds!
    float t = dht.readTemperature(); 
    float g = analogRead(A0);
   
    

  if (P.displayAnimate())
  {


     if (Firebase.pushFloat(firebaseData,"/user1/board1/smoke",get_time()+",smoke:"+String(g/1023*100)))
    {
      Serial.println("PASSED");
      Serial.print(g);
   
      Serial.println("------------------------------------");
      Serial.println();
    }
     if (Firebase.pushFloat(firebaseData,"/user1/board1/temperature",(t)))
    {
      Serial.println("PASSED");
      Serial.print(t);
   
      Serial.println("------------------------------------");
      Serial.println();
    }
    if (Firebase.pushFloat(firebaseData,"/user1/board1/humidity",(h)))
    {
      Serial.println("PASSED");
      Serial.print(h);
   
      Serial.println("------------------------------------");
      Serial.println();
    }
     if (Firebase.pushString(firebaseData,"/user1/board1/timestamp",get_time()))
    {
      Serial.println("PASSED");
      Serial.print(get_time());
   
      Serial.println("------------------------------------");
      Serial.println();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + firebaseData.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
    





// 
      Serial.print("Humidity: ");  Serial.print(h);
      String fireHumid = String(h) + String("%");  
//    Firebase.setString("/Alert/Humi/",fireHumid);//convert integer humidity to string humidity 
      Serial.print("%  Temperature: ");  Serial.print(t);  Serial.println("Â°C ");
      String fireTemp = String(t) + String("Â°C"); 
      Serial.print("Gas Level: "); Serial.print(g/1023*100);

/////////////////Condition for temperature and smoke values/////////////////////////
    if (t>35.00 || g>500){
          digitalWrite(Buzz,HIGH);
          Serial.println("BuzzerPrint");
          String s = ("Alert");

      int n = s.length();
      char char_array[n+1];
      int i;
      strcpy(char_array,s.c_str());
      strcpy(curMessage,char_array);
      P.displayReset();
      
          }
      else{
        digitalWrite(Buzz,LOW);
        Serial.println("Alert off");
        
     if (Firebase.getString(firebaseData,"/user1/board1/message"))
    {

      String s = (firebaseData.stringData());

      int n = s.length();
      char char_array[n+1];
      int i;
      strcpy(char_array,s.c_str());
      strcpy(curMessage,char_array);
      
      Serial.println("PASSED");
      Serial.println("VALUE: "+ s);

               

      Serial.println("------------------------------------");
      Serial.println();
       P.displayReset();                
    }
     
        
  }
  }
  readSerial();

                     //setup path and send readings
  }
  
