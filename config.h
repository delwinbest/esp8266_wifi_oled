//#define OTA
//#define SERIALDEBUG

// Hostname
const char *myHostname = "ESP_OLED";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";

/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_ssid = "ESP_ap";
const char *softAP_password = "12345678";


// DNS server
const byte DNS_PORT = 53;

// ESP Wifi oled Board Pin Defnitions
const byte  pin_UP = 12;
const byte  pin_DOWN = 14;
const byte  pin_ENTER = 13;
// D5 (Arduino), 1 NodeMCU -> SDA
// D4 (Arduino), 2 NodeMCU -> SCL
const byte  pin_SDA = 5;
const byte  pin_SCL = 4;
const byte  pin_LED = 16;