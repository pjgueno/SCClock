// Language config
#define CURRENT_LANG INTL_LANG

// Wifi config
const char WLANSSID[] PROGMEM = "luftdaten";
const char WLANPWD[] PROGMEM = "luftd4ten";

// BasicAuth config
const char WWW_USERNAME[] PROGMEM = "admin";
const char WWW_PASSWORD[] PROGMEM = "admin";
const char SENSORS_CHOSEN[] PROGMEM = "2492,4686,10557";
#define WWW_BASICAUTH_ENABLED 0
#define RANDOM_ORDER 0
#define AUTO_CHANGE 0
#define FADE_LED 0
#define PM_CHOICE 2
#define TIME_FOR_WIFI_CONFIG 600000
#define CALLING_INTERVALL_MS 300000
#define TIME_OFFSET 1

// Sensor Wifi config (config mode)
#define FS_SSID ""
#define FS_PWD ""

    static const char URL_API_SENSORCOMMUNITY[] PROGMEM = "https://data.sensor.community/airrohr/v1/sensor/";
