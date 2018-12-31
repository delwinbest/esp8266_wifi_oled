//#define OTA
//#define SERIALDEBUG

// Hostname
const char* HOSTNAME = "ESP_OLED";

// wifi settings
const char* ssid     = "SSID";
const char* password = "password";

// D5 (Arduino), 1 NodeMCU -> SDA
// D4 (Arduino), 2 NodeMCU -> SCL
const byte  pin_SDA = 5;
const byte  pin_SCL = 4;

const byte  pin_LED = 16;

// Probably wrong
const byte  pin_UP = 12;
const byte  pin_DOWN = 14;
const byte  pin_ENTER = 13;