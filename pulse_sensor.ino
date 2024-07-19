//Libraries to be imported are 
#include <Wire.h> //This is the library used for I2C transmission

//for interfacing sense_arr sparkFun MAX3010x Pulse and Proximity sensor Library that is arduino library for MAX30102 pulse oximeter sensor
#include "heartRate.h"
#include "MAX30105.h"

#include <Firebase_ESP_Client.h>
#include "time.h"
#include <WiFi.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const char* serverName = "http://api.thingspeak.com/update"; 
String apiKey = "Enter API";
int v;

#define I2CMux 0x70//Macro for address of I2C multiplexer 0x70 
//const int num = 3;
#define num 2 //macro for number of sensors 
// const char* sensorNames[num] = {"Vata", "Pitta", "Kapha"};
//const byte SIZE = 4
MAX30105 sense_arr[num]; 
double irvalues[2]={0,0};

// Insert your network credentials
#define WIFI_SSID "Jeevesh"
#define WIFI_PASSWORD "pathak123"

#define API_KEY "AIzaSyBLWK6Xwm6FbAvRA3z-pp2S2RC67FUYsI4"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "drishtidiwani03@gmail.com"
#define USER_PASSWORD "12345678"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://naadi-tarangini-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String vattaPath = "/vatta";
String pittaPath = "/pitta";
String kaphaPath = "/kapha";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 100;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


void setup()
{
  Serial.begin(9600); // baud rate
  Serial.println("Initializing...");
  Wire.begin();//begin transmission
  initWiFi();
  configTime(0, 0, ntpServer);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

// Assign the callback function for the long running token generation task
config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

// Assign the maximum retry of token generation
config.max_token_generation_retry = 5;

// Initialize the library with the Firebase authen and config
Firebase.begin(&config, &auth);

// Getting the user UID might take a few seconds
Serial.println("Getting User UID");
while ((auth.token.uid) == "") 
{
  Serial.print('.');
  delay(1000);
}

uid = auth.token.uid.c_str();
Serial.print("User UID: ");
Serial.print(uid);

// Update database path
databasePath = "/UsersData/" + uid + "/readings";


/*there are 3 sensors so using a for loop to select 3 sensors on mux lines and initialize them */
  for (int i = 0; i < num; i++) {
    Wire.beginTransmission(I2CMux);
    Wire.write(1 << i);
    Wire.endTransmission();

    if (!sense_arr[i].begin(Wire, I2C_SPEED_FAST)) {
      //Serial.print(sensorNames[i]);
      Serial.println(" Sensor not found. ");
      while (1); //if sensor not found stop here.
    }
   // Serial.print(sensorNames[i]) ;
    Serial.println(" MAX30102 initialization done.");
   sense_arr[i].setup();
   sense_arr[i].setPulseAmplitudeRed(0x2A); // to define the ampltude of red light for pulse sensing
   sense_arr[i].setPulseAmplitudeGreen(0); //
  }
  Serial.println("place your finger on the sensor");
}

void loop()
{
    if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    timestamp = getTime();

  for (int i = 0; i < num; i++) {
    // this activity has to be repeated for each sensor 
    Wire.beginTransmission(I2CMux);// transmission begin 
    Wire.write(1 << i);
    Wire.endTransmission();

    irvalues[i]=sense_arr[i].getIR();
    Serial.print(irvalues[0]);
    Serial.print(" , ");
    parentPath= databasePath + "/" + String(timestamp);

    json.set(vattaPath.c_str(), String(irvalues[0]));
    json.set(timePath, String(timestamp));
    Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json);

    Serial.println(irvalues[1]);
       parentPath= databasePath + "/" + String(timestamp);

    json.set(pittaPath.c_str(), String(irvalues[1]));
    json.set(timePath, String(timestamp));
    Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json);
    //Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
  delay(10);
    }

  if (WiFi.status()== WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(serverName);
    v = irvalues[0];
    String DataSent = "API_KEY=" + apiKey+ "&field1=" + String(v);
    int Response = http.POST (DataSent);
    Serial.print (v);
    Serial.print(" Response: ");
    Serial.println(Response);
    http.end();
  }
}
