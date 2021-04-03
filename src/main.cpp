#include <Arduino.h> 
#include <Ticker.h>
#include <PxMatrix.h>

#include <PubSubClient.h> 
#include "banana.h"
#include <Time.h>

#include <ArduinoJson.h>
#include <StreamUtils.h>

#include "LittleFS.h"
#include "WiFiManager.h"
#include "webServer.h"
#include "updater.h"
#include "fetch.h"
#include "configManager.h"
#include "timeSync.h"
#include "dashboard.h"

#include <TetrisMatrixDraw.h>



//#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_OE 2
#define P_D 12
#define P_E 0


const char* mqttServer = "192.168.1.7";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

WiFiClient espClient;
PubSubClient client(espClient);

PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
TetrisMatrixDraw tetris(display); //Pass it into the library

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);
uint16_t myCONSO = display.color565(75, 0, 130);
uint16_t color_hours = myGREEN;
uint16_t color_minutes = myYELLOW;


uint16 myCOLORS[8] = {myRED, myGREEN, myBLUE, myWHITE, myYELLOW, myCYAN, myMAGENTA, myBLACK};

bool shouldSaveWifiConfig = true;

int brightness=10;
time_t current_time;





const char *TIME_SERVER = "fr.pool.ntp.org";
int myTimeZone = 2; // change this to your time zone (see in timezone.h)
const int EPOCH_1_1_2019 = 1546300800; //1546300800 =  01/01/2019 @ 12:00am (UTC)


//unsigned long button_press_time=0;

union single_double{
  uint8_t two[2];
  uint16_t one;

} this_single_double;



time_t compileTime()
{
    const time_t FUDGE(10);     // fudge factor to allow for compile time (seconds, YMMV)
    const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char chMon[4], *m;
    tmElements_t tm;

    strncpy(chMon, compDate, 3);
    chMon[3] = '\0';
    m = strstr(months, chMon);
    tm.Month = ((m - months) / 3 + 1);

    tm.Day = atoi(compDate + 4);
    tm.Year = atoi(compDate + 7) - 1970;
    tm.Hour = atoi(compTime);
    tm.Minute = atoi(compTime + 3);
    tm.Second = atoi(compTime + 6);
    time_t t = makeTime(tm);
    return t + FUDGE;           // add fudge factor to allow for compile time
}

// format and print a time_t value, with a time zone appended.
void printDateTime(time_t t, const char *tz)
{
    char buf[32];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
    Serial.println(buf);
}

// ISR for display refresh
void display_updater(){
  display.display(70);
}

#define frame_size 1024

void drawFrame(uint16_t *frame) {
  display.clearDisplay();
  int imageHeight = 32;
  int imageWidth = 64;
  int counter = 0;
  for (int yy = 0; yy < imageHeight; yy++) {
    for (int xx = 0; xx < imageWidth; xx++) {
      display.drawPixel(xx, yy, frame[counter]);
      counter++;
    }
  }
  delay(100);
}

void banana(){
     display.setCursor(0,0);
   display.clearDisplay();
  
   for (int banana = 0; banana <=10; banana++){
    drawFrame( banana_frame_1);
    drawFrame( banana_frame_2);
    drawFrame( banana_frame_4);
    drawFrame( banana_frame_6);
    drawFrame( banana_frame_8);
    yield();
}
}

class buffer_draw : public Adafruit_GFX {
public:
  uint8_t frame_buffer[frame_size]={0};
  buffer_draw() : Adafruit_GFX(40,16) {}
  uint8_t bg_color[3];

  // We need that one so we can call ".print"
  void drawPixel(int16_t x, int16_t y, uint16_t color)
  {
    if ((x>31)| (y>15))
      return;

#if RGB==565
    this_single_double.two[0]=frame_buffer[40];
    this_single_double.two[1]=frame_buffer[41];
    uint8_t r = ((((this_single_double.one >> 11) & 0x1F) * 527) + 23) >> 6;
    uint8_t g = ((((this_single_double.one >> 5) & 0x3F) * 259) + 33) >> 6;
    uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
    // We only do black or white for the writing
    if (((r+g+b)/3)<120)
    {
      frame_buffer[x*2+y*64]=255;
      frame_buffer[x*2+y*64+1]=255;

    }
    else
    {
      frame_buffer[x*2+y*64]=0;
      frame_buffer[x*2+y*64+1]=0;
    }
#else
    // We only do black or white for the writing
    if (((frame_buffer[60]+frame_buffer[61]+frame_buffer[62])/3)<120)
    {
      frame_buffer[x*3+y*96]=255;
      frame_buffer[x*3+y*96+1]=255;
      frame_buffer[x*3+y*96+2]=255;
    }
    else
    {
      frame_buffer[x*3+y*96]=0;
      frame_buffer[x*3+y*96+1]=0;
      frame_buffer[x*3+y*96+2]=0;
    }
#endif
  }
};

buffer_draw buffer_drawer;

String Argument_Name;
String Clients_Response;
String message;
int message_time;

        



void display_info( String string, uint16 color, uint idelay){
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor( color);
  display.println ( string);
  delay ( idelay);
}


void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveWifiConfig = true;
}
/*void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}*/


void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> doc;
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  //ReadLoggingStream loggingStream( (char*)bpayload, Serial);

  DeserializationError error = deserializeJson(doc, payload, length);
  JsonObject obj = doc.as<JsonObject>();
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  String type = obj["type"];
  if( type.compareTo( "command") == 0){
    String command = obj["command"];
    Serial.println( command);
    if( command.compareTo( "banana") == 0)
      banana();
    else 
    if ( command.compareTo( "brightness") == 0){
      int bri = obj["brightness"];
      if( bri > 0 && bri < 100 )
        brightness = bri;
    }
    if ( command.compareTo( "hour_color") == 0){
      const char* color = obj["color"];
      long rgb = strtol( color, 0, 16); // parse as Hex, skipping the leading '#'
      int r = (rgb >> 16) & 0xFF;
      int g = (rgb >> 8) & 0xFF;
      int b = rgb & 0xFF;
      Serial.printf("r=%d, g=%d, b=%d\n", r, g, b);
      color_hours = display.color565( r, g, b);
    }
    if ( command.compareTo( "minute_color") == 0){
      const char* color = obj["color"];
      long rgb = strtol( color, 0, 16); // parse as Hex, skipping the leading '#'
      int r = (rgb >> 16) & 0xFF;
      int g = (rgb >> 8) & 0xFF;
      int b = rgb & 0xFF;
      Serial.printf("r=%d, g=%d, b=%d\n", r, g, b);
      color_minutes = display.color565( r, g, b);
    }
  }else if ( type.compareTo( "info") == 0){
    String text = obj["text"];
    uint16 color = obj["color"];
    uint idelay = obj["delay"];
    display_info( text, color, idelay);
  }

  



 
//String sPayload = String( (char*)bpayload);
//if( sPayload.startsWith( "cmd->banana"))
//  banana();
}

void setup() {

  Serial.begin(115200);

  Serial.println("Setup: before wifi");

  display.begin(16);
  display.setBrightness( brightness);

  display.clearDisplay();
  Serial.print("Pixel draw latency in us: ");
  unsigned long start_timer = micros();
  display.drawPixel(1, 1, 0);
  unsigned long delta_timer = micros() - start_timer;
  Serial.println(delta_timer);

  Serial.print("Display update latency in us: ");
  start_timer = micros();
  display.display(0);
  delta_timer = micros() - start_timer;
  Serial.println(delta_timer);

  display_ticker.attach(0.002, display_updater);

  //WiFiManager wifiManager; 

  LittleFS.begin();
  GUI.begin();
  
  configManager.begin();
  WiFiManager.begin(configManager.data.projectName);
  //timeSync.begin();


if( WiFi.isConnected()){
  Serial.println("Wifi OK");
  Serial.println( WiFi.localIP());

  client.setServer(mqttServer, mqttPort);
  
  client.setCallback(callback);

   while (!client.connected()) {
     Serial.println("Connecting to MQTT...");

    String mac = WiFi.macAddress();
    char copy[50];
    mac.toCharArray( copy, 50);

    if (client.connect( copy , mqttUser, mqttPassword )) {
       Serial.println("Connected to mqtt");  
     } else {

      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
    }
   configTime(0, 0, TIME_SERVER);
   setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);

  while (current_time < EPOCH_1_1_2019)
  {
    current_time = time(nullptr);
    delay(500);
    Serial.print("*");
  }
    Serial.print("Ok with time");

  client.publish("domo/clock", "Hello from ESP8266");
  client.subscribe("domo/clock/#");

  yield();
}
}


void loop() {
  client.loop();

  WiFiManager.loop();
  updater.loop();  
  dash.loop();

  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize( 1);
  
  display.setBrightness( brightness);

  //uint8_t this_hour= hour( current_time);
  //uint8_t this_minute = minute( current_time);
  struct tm *timeinfo;

  time(&current_time);
  timeinfo = localtime(&current_time);

  uint8_t this_hour = timeinfo->tm_hour;
  uint8_t this_minute = timeinfo->tm_min;


  if(!client.connected()) {
    Serial.println("Connecting to MQTT...");

    String mac = WiFi.macAddress();
    char copy[50];
    mac.toCharArray( copy, 50);

    if (client.connect( copy , mqttUser, mqttPassword )) {
       Serial.println("Connected to mqtt");  
       client.publish("domoticz/out", "Hello from ESP8266");
       client.subscribe("domo/clock/#");
     } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
  

  String shour;
  #include "switch_hour_en.h"


  display.setCursor(0,0);
  display.setTextColor( color_hours);
  display.println( shour);
  display.setTextColor( color_minutes);
  display.setCursor(0,16);
  display.println( sminute);

}



