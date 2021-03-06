// WiFi includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
// OTA Includes
#include <ArduinoOTA.h>
#include "spi_flash.h"

// Oled Includes
#include <Wire.h>
#include "SSD1306.h" 

// Custom Includes
#include "config.h"

// Initialize the OLED display using brzo_i2c
// D5 (Arduino), 1 NodeMCU -> SDA
// D4 (Arduino), 2 NodeMCU -> SCL
//SSD1306Brzo display(0x3c, pin_SDA, pin_SCL);
SSD1306  display(0x3c, pin_SDA, pin_SCL);
// Enable ADC for Vcc 
ADC_MODE(ADC_VCC);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// DNS server
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);



/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;

/** Current WLAN status */
int status = WL_IDLE_STATUS;


void setup() {
  Serial.begin(74880);
  Serial.setDebugOutput(true);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(pin_LED, OUTPUT);
  digitalWrite(pin_LED, !digitalRead(pin_LED));   // toggle LED
  
  // Configure Interrupts for input pins
  pinMode(pin_DOWN, INPUT_PULLUP);
  pinMode(pin_UP, INPUT_PULLUP);
  pinMode(pin_ENTER, INPUT_PULLUP);

  display.init();
  display.flipScreenVertically();
  display.setContrast(255);
  // Initialize the log buffer
  // allocate memory to store 8 lines of text and 30 chars per line.
  display.setLogBuffer(5, 26);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();

  //display.println("Starting...");
  digitalWrite(pin_LED, !digitalRead(pin_LED));   // toggle LED
  // Draw it to the internal screen buffer
  display.println("Vcc: " + getVoltage());
  display.println("Reset:" + ESP.getResetReason());
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display();
  //display.print("TimeSync: ");
  //display.println(timeClient.getFormattedTime());
  //timeClient.begin();
  //timeClient.update();
  //display.clear();
  //display.drawLogBuffer(0, 0);
  //display.display();
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.persistent(false);
  delay(500);
  WiFi.mode(WIFI_AP_STA);
  delay(500);
  setupCore();
  digitalWrite(pin_LED, HIGH);   // toggle LED


}

void loop() {

  // display.clear();
  // staticMenu();
  //delay(500);
  //display.clear();
  //display.drawString(0, 50, timeClient.getFormattedTime());
  //display.display();
  //int s = WiFi.status();
  //if (s == WL_IDLE_STATUS && millis() > (lastConnectTry + 360000) ) {
    /* If WLAN disconnected and idle try to connect */
    /* Don't set retry time too low as retry interfere the softAP operation */
  //  connect = true;
  //}
  //if (connect) {
  //  display.println ( "Connect requested" );
  //  display.clear();
  //  display.drawLogBuffer(0, 0);
  //  display.display();
  //  connect = false;
  //  connectWifi();
  //  lastConnectTry = millis();
  // }
  //{
  //  if (status != s) { // WLAN status change
      //display.print ( "Status: " );
      //display.println ( s );
      //display.clear();
      //display.drawLogBuffer(0, 0);
      //display.display();
  //    status = s;
  //    if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        //display.println ( "" );
  //      display.print ( "Connected to " );
  //      display.println ( ssid );
  //      display.print ( "IP address:" );
  //      display.println ( WiFi.localIP() );
  //      display.clear();
  //      display.drawLogBuffer(0, 0);
  //      display.display();
  //    } else if (s == WL_NO_SSID_AVAIL) {
        //WiFi.softAPdisconnect();
        //WiFi.disconnect(true);
        //delay(500);
  //    }
   //   setupCore();
  //  }
  //}
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
  //#ifdef OTA
  ArduinoOTA.handle();
  //#endif
  
}


void setupCore(){
  loadCredentials(); // Load WLAN credentials from network

  if (strlen(ssid) > 0) {
    connectWifi(); 
  } 
  setupAP();

  display.print("Start:");
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display(); 
  // Setup MDNS responder
  
  if (MDNS.begin(myHostname)) {
    display.print("DNS,");
    display.clear();
    display.drawLogBuffer(0, 0);
    display.display();
    /* Setup the DNS server redirecting all the domains to the apIP */  
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    MDNS.addService("http", "tcp", 80);
    startHTTP();
    configureOTA();
  }
  
  display.println(":Done");
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display(); 
}

String getVoltage() {
  uint16_t v = ESP.getVcc();
  float_t v_cal = ((float)v/1024.0f);
  char v_str[10];
  dtostrf(v_cal, 5, 3, v_str);
  sprintf(v_str,"%s V", v_str);
  return v_str;
}

void connectWifi() {
  /*
  typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
  } wl_status_t;
  */
  
  display.print("Connecting to ");
  display.println ( ssid );
  WiFi.begin ( ssid, password );
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display();
  int connRes = WiFi.waitForConnectResult();
  if ( connRes == WL_CONNECTED ) {
    display.print ( "Connected to " );
    display.println ( ssid );
    display.print ( "IP address:" );
    display.println ( WiFi.localIP() );
    display.clear();
    display.drawLogBuffer(0, 0);
    display.display();
  } else {
    display.print ( "Wifi Connect Failed: " );
    display.println ( connRes );
    display.clear();
    display.drawLogBuffer(0, 0);
    display.display();
  }
  
}

void setupAP() {
  digitalWrite(pin_LED, LOW);   // toggle LED

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  
  display.print("AP IP:");
  display.println(WiFi.softAPIP());
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display();
  
  display.print("AP ssid:");
  display.println(softAP_ssid);
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display(); 
  digitalWrite(pin_LED, HIGH);   // toggle LED
}

void startHTTP() {
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.on("/reboot", handleReboot);
  server.on("/wifisave", handleWifiSave);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound ( handleNotFound );
  server.begin(); // Web server start
  display.print("HTTP,");
}

void configureOTA(){
  //#ifdef OTA

  // Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  
  
  ArduinoOTA.onStart([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth()/2, display.getHeight()/2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth()/2, display.getHeight()/2, "Restart");
    display.display();
  });
  display.print("OTA,");
  display.clear();
  display.drawLogBuffer(0, 0);
  display.display(); 

  ArduinoOTA.begin();
}

/*
// Menu Work
int valA;
int valB;
int valC;

byte clk;
byte menuCount = 1;
byte dir = 0;
bool runState = false;


void staticMenu() {
  display.setLogBuffer(8, 30);
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.println("yyRobotic");
  //---------------------------------
  display.setFont(ArialMT_Plain_10);
  display.print("Value A:");
  display.println(valA);

  display.print("Value B:");
  display.println(valB);

  display.print("Value C:");
  display.println(valC);

  display.println("Start:");
  if (menuCount == 4) {
    display.print("Idle");
    runState = false;
  }
  //display.setCursor(2, (menuCount * 10) + 10);
  //display.println(">");
  display.drawLogBuffer(0, 0);
  display.display();
}
*/




/*
// Used interrupts for push buttons, but seems to have issues with Wifi
// Interrupts cause device to crash when using wifi :(
//attachInterrupt(digitalPinToInterrupt(pin_DOWN), testInterrupt, FALLING);
//attachInterrupt(digitalPinToInterrupt(pin_UP), testInterrupt, FALLING);
//attachInterrupt(digitalPinToInterrupt(pin_ENTER), testInterrupt, FALLING);
  
void testInterrupt() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(display.getWidth()/2, display.getHeight()/2 - 10, "TEST INTERRUPT");
  display.display();
  delay(3000);
}
*/
