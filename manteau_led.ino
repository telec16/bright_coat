#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FastLED.h"
#include "hsv2rgb.h"

#define NUM_LEDS 6
#define DATA_PIN 3
#define DEFAULT_LUM 200
#define DEFAULT_MODE BRUT

#define WiFi_SSID      "Modifiez_moi"       // WiFi SSID

CRGB leds[NUM_LEDS];
CRGB brut[NUM_LEDS];
ESP8266WebServer server(80);

enum {BRUT, ANIM1, ANIM2,  NONE} anim_mode = DEFAULT_MODE;
unsigned char lum = DEFAULT_LUM;


const CRGB anim1[NUM_LEDS] = {
  CRGB(95, 33, 175),
  CRGB(42, 95, 124),
  CRGB(78, 39, 130),
  CRGB(80, 143, 178),
  CRGB(122, 80, 178),
  CRGB(24, 128, 186)
};
char htmlBody_root[] = "<!DOCTYPE html>\n"
"<html>\n"
"  <head>\n"
"   <title>COULEURS</title>\n"
"   <meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>\n"
"   <meta name='viewport' content='width=device-width, initial-scale=1' />\n"
"   <style type='text/css'>\n"
"     body { text-align: center; padding: 3%; font: 20px Helvetica, sans-serif; color: #333; }\n"
"     h1 { font-size: 50px; margin: 0; }\n"
"     article { display: block; text-align: left; max-width: 650px; margin: 0 auto; }\n"
"     a { color: #dc8100; text-decoration: none; }\n"
"     a:hover { color: #333; text-decoration: none; }\n"
"     @media only screen and (max-width : 480px) {\n"
"         h1 { font-size: 40px; }\n"
"     }\n"
"     .signature { text-align: right; font: 12px Helvetica, sans-serif; color: #975; }\n"
"   </style>\n"
"   <script>\n"
"   function changes(){\n"
"     var val = document.getElementById('action').value;\n"
"     if(val == 'brut'){\n"
"       document.getElementById('brut').removeAttribute('hidden');\n"
"     }\n"
"     else {\n"
"       document.getElementById('brut').setAttribute('hidden', true);\n"
"     }\n"
"   }\n"
"   </script>\n"
" </head>\n"
" <body>\n"
"   <article>\n"
"     <h1>Modifiez les couleurs de ma veste !</h1>\n"
"     <form method='post' action=''>\n"
"     <select name='action' id='action' onchange='changes();'>\n"
"       <option value='brut'>Modifier les couleurs individuellement</option>\n"
"       <option value='anim1'>Animation 1</option>\n"
"       <option value='anim2'>Animation 2</option>\n"
"     </select></br>\n"
"     <p id='brut'>\n"
"       <input type='color' name='l1'><label for='l1'>LED 1</label></br>\n"
"       <input type='color' name='l2'><label for='l2'>LED 2</label></br>\n"
"       <input type='color' name='l3'><label for='l3'>LED 3</label></br>\n"
"       <input type='color' name='l4'><label for='l4'>LED 4</label></br>\n"
"       <input type='color' name='l5'><label for='l5'>LED 5</label></br>\n"
"       <input type='color' name='l6'><label for='l6'>LED 6</label></br>\n"
"     </p>\n"
"     <input type='range' name='lum' min='0' max='255' value='200'><label for='lum'>Luminosit&eacute </label></br>\n"
"     <input type='submit' value='Envoyer' />\n"
"     </form>\n"
"   </article>\n"
"   <p class='signature'>by Loup Plantevin</p>\n"
" </body>\n"
"</html>\n";
char htmlBody_wrong[] = "<h1>Wrong something, pls reco</h1>\r\n";


CRGB str2CRGB(String hexstring) {
  long hexColor = strtol( &(hexstring[1]), NULL, 16);
  long r = hexColor >> 16;
  long g = hexColor >> 8 & 0xFF;
  long b = hexColor & 0xFF;
  
  return CRGB(r,g,b);
}
/*-1: Less
 * 0: Good
 *+1: More
 */
int isInInterval(int min, int val, int max) {
  int in = 0;
  
  if(min<max) {
    if(max < val)
      in = +1;
    if(val < min)
      in = -1;
  }
  else {
    if((max < val) && (val <= ((max+min)/2)))
      in = +1;
    if((((max+min)/2) <  val) && (val < min))
      in = -1;
  }
  
  return in;
}
  
CHSV flicker(const CRGB lastcr, const CRGB basecr, const int maxDist, const int maxDiff){
  CHSV lastc = rgb2hsv_approximate(lastcr);
  CHSV basec = rgb2hsv_approximate(basecr);
  CHSV newc = basec;
  Serial.print(lastc.hue+String(" "));
  newc.hue = lastc.hue + ((rand() % (2*maxDiff+1)) - maxDiff);
  int isint = isInInterval((unsigned char)(basec.hue-maxDist), newc.hue, (unsigned char)(basec.hue+maxDist));
  newc.hue = (isint==0) ? newc.hue : ((isint<0) ? (unsigned char)(basec.hue-maxDist) : (unsigned char)(basec.hue+maxDist));
  Serial.println(newc.hue);
  return newc;
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}


void setupWiFi()
{
  WiFi.mode(WIFI_AP);
  
  WiFi.softAP(WiFi_SSID);
  Serial.println("server connected !");
  
  Serial.print("IP:");
  Serial.println(WiFi.softAPIP());
  
  Serial.print("Please connect to :");
  Serial.print((IpAddress2String(WiFi.softAPIP())+"/").c_str());
}
void createServer()
{  
  WiFiClient client;
  
  setupWiFi();
  server.on("/", handleRoot);
  //server.onNotFound(htmlBody_wrong);
  server.begin();
  Serial.println("Server started");
  
  return;
}


void handleRoot() {

  if(server.hasArg("action")){
    String action = server.arg("action");
    
    if(action.equals("brut")){
      anim_mode = BRUT;
      
      for(int i=0; i<NUM_LEDS; i++)
      {
        if(server.hasArg(String("l")+(i+1))){
          String str = server.arg(String("l")+(i+1));
          brut[i] = str2CRGB(str);
        }
        else{
          leds[i] = CRGB::Black;
        }
      }
    } else if(action.equals("anim1")){
      anim_mode = ANIM1;
    } else if(action.equals("anim2")){
      anim_mode = ANIM2;
    } else {
      anim_mode = NONE;
    }
    
    if(server.hasArg("lum")){
      String strLum = server.arg("lum");
      lum = atoi(&(strLum[0]));
    }
    else{
      lum = DEFAULT_LUM;
    }
    FastLED.setBrightness(lum);
  }
    
  server.send(200, "text/html", htmlBody_root);
}


void setup() { 
  Serial.begin(115200);
  delay(10);
  
  Serial.println();
  Serial.println();
  Serial.println("Hi !");

  delay(10);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  createServer();  
}

unsigned char offset=0;

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();

  switch(anim_mode){
    case BRUT:
      for(int i=0; i<NUM_LEDS; i++){
        leds[i] = flicker(leds[i], brut[i], 40, 10);
      }
      break;
    case ANIM1:
      for(int i=0; i<NUM_LEDS; i++){
        leds[i] = flicker(leds[i], anim1[i], 60, 10);
      }
      break;
    case ANIM2:
      for(int i=0; i<NUM_LEDS; i++){
        leds[i] = CHSV( ( (int)( (i+offset) %NUM_LEDS) *255) /NUM_LEDS, 200, 150);
        offset++;//o = (o+1)%NUM_LEDS;
      }
      break;
    case NONE:
      for(int i=0; i<NUM_LEDS; i++){
        leds[i] = CRGB((((i+offset)%NUM_LEDS)*255)/NUM_LEDS, 0, 0);
        offset = (offset+1)%NUM_LEDS;
      }
      break;
  }
  FastLED.show();
  delay(100);
}
