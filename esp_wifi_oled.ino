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
#ifdef OTA
  #include <ArduinoOTA.h>
#endif

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
  display.setLogBuffer(8, 30);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();

  //display.println("Starting...");
  digitalWrite(pin_LED, !digitalRead(pin_LED));   // toggle LED
  // Draw it to the internal screen buffer
  display.println("Vcc: " + getVoltage());
  display.println("Configuring access point...");
  display.drawLogBuffer(0, 0);
  display.display();
  digitalWrite(pin_LED, !digitalRead(pin_LED));   // toggle LED

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  display.print("AP IP address: ");
  display.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /*
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    digitalWrite(pin_LED, !digitalRead(pin_LED));   // toggle LED
    delay(50);
  }*/
  
    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound ( handleNotFound );
  server.begin(); // Web server start

  display.print("TimeSync: ");
  display.println(timeClient.getFormattedTime());
  timeClient.begin();
  timeClient.update();
  display.drawLogBuffer(0, 0);
  display.display();
  display.println("HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
  display.drawLogBuffer(0, 0);
  display.display();
  digitalWrite(pin_LED, HIGH);   // toggle LED

#ifdef OTA
  ArduinoOTA.begin();
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
#endif

  delay(2000);
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
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
}

void loop() {

  #ifdef OTA
    ArduinoOTA.handle();
  #endif
 // display.clear();
 // staticMenu();
  delay(500);
  //display.clear();
  //display.drawString(0, 50, timeClient.getFormattedTime());
  //display.display();

  if (connect) {
    display.println ( "Connect requested" );
    display.drawLogBuffer(0, 0);
    display.display();
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + 60000) ) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connect = true;
    }
    if (status != s) { // WLAN status change
      display.print ( "Status: " );
      display.println ( s );
      display.drawLogBuffer(0, 0);
      display.display();
      status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        display.println ( "" );
        display.print ( "Connected to " );
        display.println ( ssid );
        display.print ( "IP address: " );
        display.println ( WiFi.localIP() );
        display.drawLogBuffer(0, 0);
        display.display();
        // Setup MDNS responder
        if (!MDNS.begin(myHostname)) {
          display.println("Error setting up MDNS responder!");
          display.drawLogBuffer(0, 0);
          display.display();
        } else {
          display.println("mDNS responder started");
          display.drawLogBuffer(0, 0);
          display.display();          
          // Add service to MDNS-SD
          MDNS.addService("http", "tcp", 80);
        }
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }
    }
  }
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
  
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
