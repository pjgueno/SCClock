#include <WString.h>
#include <pgmspace.h>

#define SOFTWARE_VERSION_STR "Clock-V1-102021"
String SOFTWARE_VERSION(SOFTWARE_VERSION_STR);

#include <Arduino.h>
#include <FS.h> // must be first
#include <sntp.h>

#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_DECODE_UNICODE 0

#include <ArduinoJson.h>
#define JSON_BUFFER_SIZE 2300
#define JSON_BUFFER_SIZE2 2500

#include <FastLED.h>
#include <TM1637Display.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <StreamString.h>
#include <FastLED.h>

//Own includes
#include "./intl.h"
#include "./utils.h"
#include "./defines.h"
#include "./ext_def.h"
#include "./html-content.h"


//FastLED
//Eviter extinction si reponse lente

//config variables

namespace cfg
{
  unsigned int time_for_wifi_config = TIME_FOR_WIFI_CONFIG;
  unsigned int calling_intervall_ms = CALLING_INTERVALL_MS;
  int time_offset = TIME_OFFSET;

  char sensors_chosen[LEN_SENSORS_LIST];
  char current_lang[3];

  bool random_order = RANDOM_ORDER;
  bool auto_change = AUTO_CHANGE;
  bool fade_led = FADE_LED;
  unsigned int pm_choice = PM_CHOICE; //uint8_t
  unsigned int brightness = BRIGHTNESS;

  // credentials for basic auth of internal web server
  bool www_basicauth_enabled = WWW_BASICAUTH_ENABLED;
  char www_username[LEN_WWW_USERNAME];
  char www_password[LEN_CFG_PASSWORD];

  // wifi credentials
  char wlanssid[LEN_WLANSSID];
  char wlanpwd[LEN_CFG_PASSWORD];

  // credentials of the sensor in access point mode
  char fs_ssid[LEN_FS_SSID] = FS_SSID;
  char fs_pwd[LEN_CFG_PASSWORD] = FS_PWD;

  void initNonTrivials(const char *id)
  {
    strcpy(cfg::current_lang, CURRENT_LANG);
    strcpy_P(www_username, WWW_USERNAME);
    strcpy_P(www_password, WWW_PASSWORD);
    strcpy_P(wlanssid, WLANSSID);
    strcpy_P(wlanpwd, WLANPWD);
    strcpy_P(cfg::sensors_chosen, SENSORS_CHOSEN);

    if (!*fs_ssid)
    {
      strcpy(fs_ssid, SSID_BASENAME);
      strcat(fs_ssid, id);
    }
  }
}

//must be after init of variables

#include "./clock-cfg.h"

char array_sensors[10][6];
unsigned int array_size;
unsigned int last_index;
bool clock_selftest_failed = false;
String liste_save;
String currentSensor;

//LED declarations

#define LED_PIN 2
#define LED_COUNT 64
CRGB colorLED;
bool down = true;
unsigned int fader = cfg::brightness;

CRGB leds[LED_COUNT];

struct RGB
    {
      byte R;
      byte G;
      byte B;
};

struct RGB displayColor
{
  0, 0, 0
};

const int httpPort = 80;

float PMvalue = -1.0;

//4-digit declarations

#define CLK 2
#define DIO 3

TM1637Display display(CLK, DIO);

//Messages

const uint8_t initialisation[] = {
    SEG_E,                                        // i
    SEG_C | SEG_E | SEG_G,                        // n
    SEG_E,                                        // i
    SEG_E | SEG_F | SEG_G                         // t
};

const uint8_t done[] = {
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,        // d
    SEG_C | SEG_D | SEG_E | SEG_G,                // o
    SEG_C | SEG_E | SEG_G,                        // n
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_F | SEG_G // E
};

const uint8_t nop1[] = {
    SEG_C | SEG_E | SEG_G,                        // n
    SEG_C | SEG_D | SEG_E | SEG_G,                // o
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,        // P
    SEG_B | SEG_C                                 // 1
};

const uint8_t oups[] = {
    SEG_C | SEG_D | SEG_E | SEG_G,                // o
    SEG_C | SEG_D | SEG_E,                        // u
    SEG_A | SEG_B | SEG_E | SEG_F | SEG_G,        // p
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G         // s
};

//Wifi & Server declarations

ESP8266WebServer server(httpPort);
WiFiClient client;

//NTP & time declarations

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.obspm.fr", 0, 3600000);

int hours;
int minutes;

//Control variables

int precmin;
unsigned long blinker;
unsigned long SCcall;
bool dots;


String esp_chipid;
String esp_mac_id;

//millis() && server time/counts

int last_signal_strength;
int last_disconnect_reason;
unsigned long last_page_load = millis();
unsigned long LED_milli;

bool wificonfig_loop = false;
bool connected = false;

//Values table

StaticJsonDocument<1000> sensors_json;
JsonArray sensors_list = sensors_json.to<JsonArray>();

//WiFi declarations

struct struct_wifiInfo
{
  char ssid[LEN_WLANSSID];
  uint8_t encryptionType;
  int32_t RSSI;
  int32_t channel;
  bool isHidden;
  uint8_t unused[3];
};

struct struct_wifiInfo *wifiInfo;
uint8_t count_wifiInfo;



/*****************************************************************
 * Physical button                                    *
 *****************************************************************/

#define BUTTON_PIN 5
bool force_call = false;
bool show_id = false;

ICACHE_RAM_ATTR void buttonpush()
{
  if (force_call == false){  // à voir pour limiter le bouton après 1er appui

    if (cfg::random_order == true)
    {
      unsigned int random_index = rand() % (array_size);
      last_index = random_index;
    }
    else
    {
      if (last_index < (array_size - 1))
      {
        last_index += 1;
      }
      else
      {
        last_index = 0;
      }
    }
    force_call = true;
    show_id = true;
  }
}



/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/
    static bool writeConfig()
{
  DynamicJsonDocument json(JSON_BUFFER_SIZE);
  Debug.println("Saving config...");
  json["SOFTWARE_VERSION"] = SOFTWARE_VERSION;

  for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
  {
    ConfigShapeEntry c;
    memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
    switch (c.cfg_type)
    {
    case Config_Type_Bool:
      json[c.cfg_key()].set(*c.cfg_val.as_bool);
      break;
    case Config_Type_UInt:
    case Config_Type_Time:
      json[c.cfg_key()].set(*c.cfg_val.as_uint); //uint sur 4 byte pour ESP8266
      break;
    case Config_Type_Int:
      json[c.cfg_key()].set(*c.cfg_val.as_int); 
      break;
    case Config_Type_Password:
    case Config_Type_String:
      json[c.cfg_key()].set(c.cfg_val.as_str);
      break;
    };
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

  SPIFFS.remove(F("/config.json.old"));
  SPIFFS.rename(F("/config.json"), F("/config.json.old"));

  File configFile = SPIFFS.open(F("/config.json"), "w");
  if (configFile)
  {
    serializeJson(json, configFile);
    configFile.close();
    Debug.println("Config written successfully.");
  }
  else
  {
    Debug.println("failed to open config file for writing");
    return false;
  }

#pragma GCC diagnostic pop

  return true;
}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/

/* backward compatibility for the times when we stored booleans as strings */
static bool boolFromJSON(const DynamicJsonDocument &json, const __FlashStringHelper *key)
{
  if (json[key].is<char *>())
  {
    return !strcmp_P(json[key].as<char *>(), PSTR("true"));
  }
  return json[key].as<bool>();
}

static void readConfig(bool oldconfig = false)
{
  bool rewriteConfig = false;

  String cfgName(F("/config.json"));
  if (oldconfig)
  {
    cfgName += F(".old");
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  File configFile = SPIFFS.open(cfgName, "r");
  if (!configFile)
  {
    if (!oldconfig)
    {
      return readConfig(true /* oldconfig */);
    }

    Debug.println("failed to open config file.");
    return;
  }

  Debug.println("opened config file...");
  DynamicJsonDocument json(JSON_BUFFER_SIZE);
  DeserializationError err = deserializeJson(json, configFile.readString());
  configFile.close();
#pragma GCC diagnostic pop

  if (!err)
  {
    Debug.println("parsed json...");
    for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
    {
      ConfigShapeEntry c;
      memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
      if (json[c.cfg_key()].isNull())
      {
        continue;
      }
      switch (c.cfg_type)
      {
      case Config_Type_Bool:
        *(c.cfg_val.as_bool) = boolFromJSON(json, c.cfg_key());
        break;
      case Config_Type_UInt:
      case Config_Type_Time:
        *(c.cfg_val.as_uint) = json[c.cfg_key()].as<unsigned int>();
        break;
      case Config_Type_Int:
        *(c.cfg_val.as_int) = json[c.cfg_key()].as<int>();
        break;
      case Config_Type_String:
      case Config_Type_Password:
        strncpy(c.cfg_val.as_str, json[c.cfg_key()].as<char *>(), c.cfg_len);
        c.cfg_val.as_str[c.cfg_len] = '\0';
        break;
      };
    }
    String writtenVersion(json["SOFTWARE_VERSION"].as<char *>());
    if (writtenVersion.length() && writtenVersion[0] == 'N' && SOFTWARE_VERSION != writtenVersion)
    {
      Debug.println("Rewriting old config");
      // would like to do that, but this would wipe firmware.old which the two stage loader
      // might still need
      // SPIFFS.format();
      rewriteConfig = true;
    }
  }
  else
  {
    Debug.println("failed to load json config");

    if (!oldconfig)
    {
      return readConfig(true /* oldconfig */);
    }
  }

  if (rewriteConfig)
  {
    writeConfig();
  }
}

static void init_config()
{

  Debug.println("mounting FS...");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

bool spiffs_begin_ok = SPIFFS.begin();

#pragma GCC diagnostic pop

  if (!spiffs_begin_ok)
  {
    Debug.println("failed to mount FS");
    return;
  }
  readConfig();
}

/*****************************************************************
 * html helper functions                                         *
 *****************************************************************/

static void start_html_page(String &page_content, const String &title)
{
  last_page_load = millis();

  RESERVE_STRING(s, LARGE_STR);
  s = FPSTR(WEB_PAGE_HEADER);
  s.replace("{t}", title);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), s);

  server.sendContent_P(WEB_PAGE_HEADER_HEAD);

  s = FPSTR(WEB_PAGE_HEADER_BODY);
  s.replace("{t}", title);
  if (title != " ")
  {
    s.replace("{n}", F("&raquo;"));
  }
  else
  {
    s.replace("{n}", emptyString);
  }
  s.replace("{id}", esp_chipid);
  s.replace("{macid}", esp_mac_id);
  s.replace("{mac}", WiFi.macAddress());
  page_content += s;
}

static void end_html_page(String &page_content)
{
  if (page_content.length())
  {
    server.sendContent(page_content);
  }
  server.sendContent_P(WEB_PAGE_FOOTER);
}

static void add_form_input(String &page_content, const ConfigShapeId cfgid, const __FlashStringHelper *info, const int length)
{
  RESERVE_STRING(s, MED_STR);
  s = F("<tr>"
        "<td title='[&lt;= {l}]'{o}>{i}:&nbsp;</td>"
        "<td style='width:{l}em'>"
        "<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
        "</td></tr>");
  String t_value;
  ConfigShapeEntry c;
  memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));
  switch (c.cfg_type)
  {
  case Config_Type_UInt:
    t_value = String(*c.cfg_val.as_uint);
    s.replace("{t}", F("number"));
    break;
  case Config_Type_Int:
    t_value = String(*c.cfg_val.as_int);
    s.replace("{t}", F("number"));
    break;
  case Config_Type_Time:
    t_value = String((*c.cfg_val.as_uint) / 1000);
    s.replace("{t}", F("number"));
    break;
  case Config_Type_Bool:
    break;
  case Config_Type_Password:
      s.replace("{t}", F("password"));
      info = FPSTR(INTL_PASSWORD);
      break;
  case Config_Type_String:
      t_value = String(c.cfg_val.as_str);
      s.replace("{t}", F("text"));
      break;
  }
  s.replace("{i}", info);
  s.replace("{n}", String(c.cfg_key()));
  if (cfgid != Config_sensors_chosen)
  {
    s.replace("{v}", t_value);
  }
  else
  {
    s.replace("{v}", liste_save);
  }

if(length == 99){ //100-1
  s.replace("{o}"," style='width:10em'");
  s.replace("{l}", String(length-10));
}
else{
  s.replace("{o}","");
  s.replace("{l}", String(length));
}
page_content += s;
}

static void add_radio_input(String &page_content, const ConfigShapeId cfgid, const __FlashStringHelper *info)
{
  RESERVE_STRING(s, MED_STR);
  String t_value;
  ConfigShapeEntry c;
  memcpy_P(&c, &configShape[cfgid], sizeof(ConfigShapeEntry));
  t_value = String(*c.cfg_val.as_uint);

  if (t_value == "0")
  {
      s = F("{i}"
        "<div>"
        "<input type='radio' id='pm1' name='{n}' value='0' checked>"
        "<label for='pm1'>PM1</label>"
        "</div>"
        "<div>"
        "<input type='radio' id='pm10' name='{n}' value='1'>"
        "<label for='pm10'>PM10</label>"
        "</div>"
        "<div>"
        "<input type='radio' id='pm25' name='{n}' value='2'>"
        "<label for='pm25'>PM2.5</label>"
        "</div>");
    }
    if (t_value == "1")
    {
        s = F("<b>{i}</b>"
        "<div>"
        "<input type='radio' id='pm1' name='{n}' value='0'>"
        "<label for='pm1'>PM1</label>"
        "</div>"
        "<div>"
        "<input type='radio' id='pm10' name='{n}' value='1' checked>"
        "<label for='pm10'>PM10</label>"
        "</div>"
        "<div>"
        "<input type='radio' id='pm25' name='{n}' value='2'>"
        "<label for='pm25'>PM2.5</label>"
        "</div>");
   }
   if (t_value == "2")
   {
     s = F("<b>{i}</b>"
           "<div>"
           "<input type='radio' id='pm1' name='{n}' value='0'>"
           "<label for='pm1'>PM1</label>"
           "</div>"
           "<div>"
           "<input type='radio' id='pm10' name='{n}' value='1'>"
           "<label for='pm10'>PM10</label>"
           "</div>"
           "<div>"
           "<input type='radio' id='pm25' name='{n}' value='2' checked>"
           "<label for='pm25'>PM2.5</label>"
           "</div>");
      }
  
  s.replace("{i}", info);
  s.replace("{n}", String(c.cfg_key()));
  page_content += s;
  }

  static String form_checkbox(const ConfigShapeId cfgid, const String &info, const bool linebreak)
  {
    RESERVE_STRING(s, MED_STR);
    s = F("<label for='{n}'>"
          "<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
          "<input type='hidden' name='{n}' value='0'/>"
          "{i}</label><br/>");
    if (*configShape[cfgid].cfg_val.as_bool)
    {
      s.replace("{c}", F(" checked='checked'"));
    }
    else
    {
      s.replace("{c}", emptyString);
    };
    s.replace("{i}", info);
    s.replace("{n}", String(configShape[cfgid].cfg_key()));
    if (!linebreak)
    {
      s.replace("<br/>", emptyString);
    }
    return s;
  }

  static String form_submit(const String &value)
  {
    String s = F("<input type='submit' name='submit' value='{v}' />");
    s.replace("{v}", value);
    return s;
  }

  /*****************************************************************
 * Webserver request auth: prompt for BasicAuth
 *
 * -Provide BasicAuth for all page contexts except /values and images
 *****************************************************************/
  static bool webserver_request_auth()
  {
    if (cfg::www_basicauth_enabled && !wificonfig_loop)
    {
      Debug.println("validate request auth...");
      if (!server.authenticate(cfg::www_username, cfg::www_password))
      {
        server.requestAuthentication(BASIC_AUTH, "Sensor Login", F("Authentication failed"));
        return false;
      }
    }
    return true;
  }

  static void sendHttpRedirect()
  {
    server.sendHeader(F("Location"), F("http://192.168.4.1/config"));
    server.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), emptyString);
  }

  /*****************************************************************
 * Webserver root: show all options                              *
 *****************************************************************/
  static void webserver_root()
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      sendHttpRedirect();
    }
    else
    {
      if (!webserver_request_auth())
      {
        return;
      }

      RESERVE_STRING(page_content, XLARGE_STR);
      start_html_page(page_content, emptyString);
      Debug.println("ws: root ...");

      // Enable Pagination
      page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
      page_content.replace(F("{t}"), FPSTR(INTL_CURRENT_DATA));
      page_content.replace(F("{c}"), FPSTR(INTL_CHANGE_SENSOR));
      page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
      page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
      end_html_page(page_content);
    }
  }

  /*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/

  static void webserver_config_send_body_get(String & page_content)
  {
    auto add_form_checkbox = [&page_content](const ConfigShapeId cfgid, const String &info)
    {
      page_content += form_checkbox(cfgid, info, true);
    };

    // auto add_form_checkbox_sensor = [&add_form_checkbox](const ConfigShapeId cfgid, __const __FlashStringHelper *info)
    // {
    //   add_form_checkbox(cfgid, add_sensor_type(info));
    // };

    Debug.println("begin webserver_config_body_get ...");
    page_content += F("<form method='POST' action='/config' style='width:100%;'>\n"
                      "<input class='radio' id='r1' name='group' type='radio' checked>"
                      "<input class='radio' id='r2' name='group' type='radio'>"
                      "<input class='radio' id='r3' name='group' type='radio'>"
                      "<div class='tabs'>"
                      "<label class='tab' id='tab1' for='r1'>" INTL_WIFI_SETTINGS "</label>"
                      "<label class='tab' id='tab2' for='r2'>");

    page_content += FPSTR(INTL_SENSORS_CHOICE);
    page_content += F("</label>"
                      "<label class='tab' id='tab3' for='r3'>");
    page_content += FPSTR(INTL_FUNCTIONS_SETTINGS);
    page_content += F("</label></div><div class='panels'>"
                      "<div class='panel' id='panel1'>");

    if (wificonfig_loop)
    { // scan for wlan ssids
      page_content += F("<div id='wifilist'>" INTL_WIFI_NETWORKS "</div><br/>");
    }

    page_content += FPSTR(TABLE_TAG_OPEN);
    add_form_input(page_content, Config_wlanssid, FPSTR(INTL_FS_WIFI_NAME), LEN_WLANSSID - 1);
    add_form_input(page_content, Config_wlanpwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
    page_content += FPSTR(TABLE_TAG_CLOSE_BR);
    page_content += F("<hr/>\n<br/><b>");

    page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
    page_content += FPSTR(WEB_B_BR);
    page_content += FPSTR(BR_TAG);

    // Paginate page after ~ 1500 Bytes
    server.sendContent(page_content);
    page_content = emptyString;

    add_form_checkbox(Config_www_basicauth_enabled, FPSTR(INTL_BASICAUTH));
    page_content += FPSTR(TABLE_TAG_OPEN);
    add_form_input(page_content, Config_www_username, FPSTR(INTL_USER), LEN_WWW_USERNAME - 1);
    add_form_input(page_content, Config_www_password, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
    page_content += FPSTR(TABLE_TAG_CLOSE_BR);
    page_content += FPSTR(BR_TAG);

    // Paginate page after ~ 1500 Bytes
    server.sendContent(page_content);
    if (!wificonfig_loop)
      {
        page_content = FPSTR(INTL_FS_WIFI_DESCRIPTION);
        page_content += FPSTR(BR_TAG);

        page_content += FPSTR(TABLE_TAG_OPEN);
        add_form_input(page_content, Config_fs_ssid, FPSTR(INTL_FS_WIFI_NAME), LEN_FS_SSID - 1);
        add_form_input(page_content, Config_fs_pwd, FPSTR(INTL_PASSWORD), LEN_CFG_PASSWORD - 1);
        page_content += FPSTR(TABLE_TAG_CLOSE_BR);

        // Paginate page after ~ 1500 Bytes
        server.sendContent(page_content);
      }
      page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(2));
      page_content += FPSTR(TABLE_TAG_OPEN);
      add_form_input(page_content, Config_sensors_chosen, FPSTR(INTL_SENSORS_LIST), LEN_SENSORS_LIST - 1);
      page_content += FPSTR(BR_TAG);
      page_content += FPSTR(SENSORS_INFO);
      page_content += FPSTR(TABLE_TAG_CLOSE_BR);
      page_content += F("<hr/>\n<br/>");
      add_radio_input(page_content, Config_pm_choice, FPSTR(INTL_PM));

      // Paginate page after ~ 1500 Bytes
      server.sendContent(page_content);

      page_content = tmpl(FPSTR(WEB_DIV_PANEL), String(3));
      // Paginate page after ~ 1500 Bytes
      server.sendContent(page_content);
      page_content += FPSTR(TABLE_TAG_OPEN);
      add_form_input(page_content, Config_calling_intervall_ms, FPSTR(INTL_CALL_INTERVAL), 5);
      add_form_input(page_content, Config_time_for_wifi_config, FPSTR(INTL_DURATION_ROUTER_MODE), 5);
      add_form_input(page_content, Config_time_offset, FPSTR(INTL_TIME_OFFSET), 2);
      page_content += FPSTR(TABLE_TAG_CLOSE_BR);
      page_content += F("<hr/>\n<br/>");
      add_form_input(page_content, Config_brightness, FPSTR(INTL_BRIGHTNESS), 3);
      add_form_checkbox(Config_random_order, FPSTR(INTL_RANDOM));
      add_form_checkbox(Config_auto_change, FPSTR(INTL_AUTO));
      add_form_checkbox(Config_fade_led, FPSTR(INTL_FADE));
      page_content += F("</div></div>");
      page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
      page_content += FPSTR(BR_TAG);
      page_content += FPSTR(WEB_BR_FORM);
      server.sendContent(page_content);
      page_content = emptyString;
    }

static void webserver_config_send_body_post(String &page_content)
{
  String masked_pwd;

  for (unsigned e = 0; e < sizeof(configShape) / sizeof(configShape[0]); ++e)
  {
    ConfigShapeEntry c;
    memcpy_P(&c, &configShape[e], sizeof(ConfigShapeEntry));
    const String s_param(c.cfg_key());
    if (!server.hasArg(s_param))
    {
      continue;
    }
    const String server_arg(server.arg(s_param));

    switch (c.cfg_type)
    {
    case Config_Type_UInt:
      *(c.cfg_val.as_uint) = server_arg.toInt();
      break;
    case Config_Type_Int:
      *(c.cfg_val.as_int) = server_arg.toInt();
      break;
    case Config_Type_Time:
      *(c.cfg_val.as_uint) = server_arg.toInt() * 1000;
      break;
    case Config_Type_Bool:
      *(c.cfg_val.as_bool) = (server_arg == "1");
      break;
    case Config_Type_String:
      strncpy(c.cfg_val.as_str, server_arg.c_str(), c.cfg_len);
      c.cfg_val.as_str[c.cfg_len] = '\0';
      break;
    case Config_Type_Password:
      if (server_arg.length())
      {
        server_arg.toCharArray(c.cfg_val.as_str, LEN_CFG_PASSWORD);
      }
      break;
    }
  }

  page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);

  server.sendContent(page_content);
  page_content = emptyString;
}

static void sensor_restart()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

  SPIFFS.end();

#pragma GCC diagnostic pop

  Debug.println("Restart.");
  delay(500);
  ESP.restart();
  // should not be reached
  while (true)
  {
    yield();
  }
}

static void webserver_config()
{
  if (!webserver_request_auth())
  {
    return;
  }

  Debug.println("ws: config page ...");

  server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Expires"), F("0"));
  // Enable Pagination (Chunked Transfer)
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);

  RESERVE_STRING(page_content, XLARGE_STR);

  start_html_page(page_content, FPSTR(INTL_CONFIGURATION));

  if (wificonfig_loop)
  { // scan for wlan ssids
    page_content += FPSTR(WEB_CONFIG_SCRIPT);
  }

  if (server.method() == HTTP_GET)
  {
    webserver_config_send_body_get(page_content);
  }
  else
  {
    webserver_config_send_body_post(page_content);
  }
  end_html_page(page_content);

  if (server.method() == HTTP_POST)
  {
    Debug.println("Writing config");
    if (writeConfig())
    {
      Debug.println("Writing configand restarting");
      sensor_restart();
    }
  }
}


/*****************************************************************
 * Webserver root: show latest values                            *
 *****************************************************************/
static void webserver_values()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    sendHttpRedirect();
    return;
  }

  RESERVE_STRING(page_content, XLARGE_STR);
  start_html_page(page_content, FPSTR(INTL_CURRENT_DATA));

  const int signal_quality = calcWiFiSignalQuality(last_signal_strength);
   Debug.println("ws: values ...");

  String pm_type = "";
  switch (cfg::pm_choice)
  {
  case 0:
    pm_type = "PM1";
    break;
  case 1:
    pm_type = "PM10";
    break;
  case 2:
    pm_type = "PM2.5";
    break;
  }

  page_content += "PM parameter: " + pm_type + FPSTR(WEB_B_BR_BR);
  page_content += "Current sensor: " + currentSensor + FPSTR(WEB_B_BR_BR);

  auto add_table_value = [&page_content](const __FlashStringHelper *sensor, const __FlashStringHelper *param, const String &value, const String &unit)
  {
    add_table_row_from_value(page_content, sensor, param, value, unit);
  };

  server.sendContent(page_content);
  page_content = F("<table cellspacing='0' cellpadding='5' class='v'>\n"
                   "<thead><tr><th>" INTL_SENSOR "</th><th> " INTL_TIME "</th><th>" INTL_VALUE "</th></tr></thead>");

  for(JsonObject sensor : sensors_list)
  {
    RESERVE_STRING(s, MED_STR);
    s = F("<tr><td>{s}</td><td>{t}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
    s.replace("{s}", sensor["sensor"].as<String>());
    s.replace("{t}", sensor["time"].as<String>());
    s.replace("{v}", sensor["value"].as<String>());
    s.replace("{u}", String(F("µg/m³")));
    page_content += s;
  }
  page_content += FPSTR(EMPTY_ROW);

  server.sendContent(page_content);
  page_content = emptyString;

  add_table_value(F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH), String(last_signal_strength), "dBm");
  add_table_value(F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");

  page_content += FPSTR(TABLE_TAG_CLOSE_BR);
page_content += FPSTR(BR_TAG);
 end_html_page(page_content);
}


/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
static void webserver_removeConfig()
{
  if (!webserver_request_auth())
  {
    return;
  }

  RESERVE_STRING(page_content, LARGE_STR);
  start_html_page(page_content, FPSTR(INTL_DELETE_CONFIG));
  Debug.println("ws: removeConfig ...");

  if (server.method() == HTTP_GET)
  {
    page_content += FPSTR(WEB_REMOVE_CONFIG_CONTENT);
  }
  else
  {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // Silently remove the desaster backup
    SPIFFS.remove(F("/config.json.old"));
    if (SPIFFS.exists(F("/config.json")))
    { //file exists
      Debug.println("removing config.json...");
      if (SPIFFS.remove(F("/config.json")))
      {
        page_content += F("<h3>" INTL_CONFIG_DELETED ".</h3>");
      }
      else
      {
        page_content += F("<h3>" INTL_CONFIG_CAN_NOT_BE_DELETED ".</h3>");
      }
    }
    else
    {
      page_content += F("<h3>" INTL_CONFIG_NOT_FOUND ".</h3>");
    }
#pragma GCC diagnostic pop
  }
  end_html_page(page_content);
}

/*****************************************************************
 * Webserver reset NodeMCU                                       *
 *****************************************************************/
static void webserver_reset()
{
  if (!webserver_request_auth())
  {
    return;
  }

  String page_content;
  page_content.reserve(512);

  start_html_page(page_content, FPSTR(INTL_RESTART_SENSOR));
  Debug.println("ws: reset ...");

  if (server.method() == HTTP_GET)
  {
    page_content += FPSTR(WEB_RESET_CONTENT);
  }
  else
  {
    sensor_restart();
  }
  end_html_page(page_content);
}

/*****************************************************************
 * Webserver page not found                                      *
 *****************************************************************/
static void webserver_not_found()
{
  last_page_load = millis();
  Debug.println("ws: not found ...");
  if (WiFi.status() != WL_CONNECTED)
  {
    if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1))
    {
      server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
    }
    else
    {
      sendHttpRedirect();
    }
  }
  else
  {
    server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
  }
}

static void webserver_static()
{
  server.sendHeader(F("Cache-Control"), F("max-age=2592000, public"));

  if (server.arg(String('r')) == F("logo"))
  {
    server.send_P(200, TXT_CONTENT_TYPE_IMAGE_PNG,
                  LUFTDATEN_INFO_LOGO_PNG, LUFTDATEN_INFO_LOGO_PNG_SIZE);
  }
  else if (server.arg(String('r')) == F("css"))
  {
    server.send_P(200, TXT_CONTENT_TYPE_TEXT_CSS,
                  WEB_PAGE_STATIC_CSS, sizeof(WEB_PAGE_STATIC_CSS) - 1);
  }
  else
  {
    webserver_not_found();
  }
}


/*****************************************************************
 * Software button                                    *
 *****************************************************************/

void softbuttonpush()
{
    if (cfg::random_order == true)
    {
      unsigned int random_index = rand() % (array_size);
      last_index = random_index;
    }
    else
    {
      if (last_index < (array_size - 1))
      {
        last_index += 1;
      }
      else
      {
        last_index = 0;
      }
    }
    force_call = true;
    show_id = true;

    if (WiFi.status() != WL_CONNECTED)
    {
      sendHttpRedirect();
    }
    else
    {
      if (!webserver_request_auth())
      {
        return;
      }

      RESERVE_STRING(page_content, XLARGE_STR);
      start_html_page(page_content, emptyString);
      Debug.println("ws: root ...");

      // Enable Pagination
      page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
      page_content.replace(F("{t}"), FPSTR(INTL_CURRENT_DATA));
      page_content.replace(F("{c}"), FPSTR(INTL_CHANGE_SENSOR));
      page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
      page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
      end_html_page(page_content);
    }
}




/*****************************************************************
 * Webserver setup                                               *
 *****************************************************************/
static void setup_webserver()
{
  server.on("/", webserver_root);
  server.on(F("/config"), webserver_config);
  server.on(F("/values"), webserver_values);
  server.on(F("/removeConfig"), webserver_removeConfig);
  server.on(F("/reset"), webserver_reset);
  server.on(F("/change"), softbuttonpush);
  server.on(F(STATIC_PREFIX), webserver_static);
  server.onNotFound(webserver_not_found);

  Debug.print("Starting Webserver... ");
  Debug.println(WiFi.localIP().toString());
  server.begin();
}

static int selectChannelForAp()
{
  std::array<int, 14> channels_rssi;
  std::fill(channels_rssi.begin(), channels_rssi.end(), -100);

  for (unsigned i = 0; i < std::min((uint8_t)14, count_wifiInfo); i++)
  {
    if (wifiInfo[i].RSSI > channels_rssi[wifiInfo[i].channel])
    {
      channels_rssi[wifiInfo[i].channel] = wifiInfo[i].RSSI;
    }
  }

  if ((channels_rssi[1] < channels_rssi[6]) && (channels_rssi[1] < channels_rssi[11]))
  {
    return 1;
  }
  else if ((channels_rssi[6] < channels_rssi[1]) && (channels_rssi[6] < channels_rssi[11]))
  {
    return 6;
  }
  else
  {
    return 11;
  }
}

/*****************************************************************
 * WifiConfig                                                    *
 *****************************************************************/
static void wifiConfig()
{
  Debug.println("Starting WiFiManager");
  Debug.print("AP ID: ");
  Debug.println(String(cfg::fs_ssid));
  Debug.print("Password: ");
  Debug.println(String(cfg::fs_pwd));

  wificonfig_loop = true;

  WiFi.disconnect(true);

  WiFi.mode(WIFI_AP);
  const IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp());
  // In case we create a unique password at first start
  Debug.print("AP Password is: ");
  Debug.println(cfg::fs_pwd);

  DNSServer dnsServer;
  // Ensure we don't poison the client DNS cache
  dnsServer.setTTL(0);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP); // 53 is port for DNS server

  setup_webserver();

  // 10 minutes timeout for wifi config
  last_page_load = millis();
  while ((millis() - last_page_load) < cfg::time_for_wifi_config + 500)
  {
    dnsServer.processNextRequest();
    server.handleClient();
    wdt_reset(); // nodemcu is alive
    MDNS.update();
    yield();
  }

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  dnsServer.stop();
  delay(100);

  Debug.print(FPSTR(DBG_TXT_CONNECTING_TO));
  Debug.println(cfg::wlanssid);

  WiFi.begin(cfg::wlanssid, cfg::wlanpwd);
  wificonfig_loop = false;
}

static void waitForWifiToConnect(int maxRetries)
{
  int retryCount = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retryCount < maxRetries))
  {
    delay(500);
    //debug_out(".", DEBUG_MIN_INFO);
    ++retryCount;
  }
}

/*****************************************************************
 * WiFi auto connecting script                                   *
 *****************************************************************/

static WiFiEventHandler disconnectEventHandler;

static void connectWifi()
{
  // Enforce Rx/Tx calibration
  system_phy_set_powerup_option(1);
  // 20dBM == 100mW == max tx power allowed in europe
  WiFi.setOutputPower(20.0f);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  delay(100);

  disconnectEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &evt)
                                                          { last_disconnect_reason = evt.reason; });
  if (WiFi.getAutoConnect())
  {
    WiFi.setAutoConnect(false);
  }
  if (!WiFi.getAutoReconnect())
  {
    WiFi.setAutoReconnect(true);
  }

  // Use 13 channels if locale is not "EN"
  wifi_country_t wifi;
  wifi.policy = WIFI_COUNTRY_POLICY_MANUAL;
  strcpy(wifi.cc, INTL_LANG);
  wifi.nchan = (INTL_LANG[0] == 'E' && INTL_LANG[1] == 'N') ? 11 : 13;
  wifi.schan = 1;

  wifi_set_country(&wifi);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(cfg::fs_ssid);

  WiFi.begin(cfg::wlanssid, cfg::wlanpwd); // Start WiFI

  Debug.print(FPSTR(DBG_TXT_CONNECTING_TO));
  Debug.println(cfg::wlanssid);

  waitForWifiToConnect(40);
  Debug.println(emptyString);
  if (WiFi.status() != WL_CONNECTED)
  {
    String fss(cfg::fs_ssid);
    Debug.print(fss.substring(0, 16));
    Debug.println(fss.substring(16));

    wifi.policy = WIFI_COUNTRY_POLICY_AUTO;

    wifi_set_country(&wifi);

    wifiConfig();
    if (WiFi.status() != WL_CONNECTED)
    {
      waitForWifiToConnect(20);
      Debug.println(emptyString);
    }
  }else{
    connected = true;
  }



  Debug.print(F("WiFi connected, IP is: "));
  Debug.println(WiFi.localIP().toString());
  last_signal_strength = WiFi.RSSI();

  if (MDNS.begin(cfg::fs_ssid))
  {
    MDNS.addService("http", "tcp", httpPort);
    MDNS.addServiceTxt("http", "tcp", "PATH", "/config");
  }
}

/*****************************************************************
 * get data from rest api                                         *
 *****************************************************************/
float getData(const char *url, unsigned int pm_type)
{

  String pm="";

  switch (pm_type)
  {
  case 0:
    pm = "P0";
    break;
  case 1:
    pm = "P1";
    break;
  case 2:
    pm = "P2";
    break;
  }

  String reponseAPI;
  DynamicJsonDocument doc(JSON_BUFFER_SIZE2);
  char reponseJSON[JSON_BUFFER_SIZE2];
  if (!client.connect("data.sensor.community", httpPort))
  {
    Debug.println("connection failed");
    display.setSegments(oups);
    delay(1000);
    doc.garbageCollect();
    Debug.println("Check Memory:");
    Debug.print("doc:");
    Debug.println(doc.memoryUsage());
    return -1.0;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + "data.sensor.community" + "\r\n" +
               "Connection: close\r\n\r\n");

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + "data.sensor.community" + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 30000) //30s call to be sure the API answers
    {
      Debug.println("Client Timeout !");
      //client.flush();
      client.stop();
      display.setSegments(oups);
      delay(1000);
      doc.garbageCollect();
      Debug.println("Check Memory:");
      Debug.print("doc:");
      Debug.println(doc.memoryUsage());
      return -1.0;
    }
  }
  // Read all the lines of the reply from server and print them to Serial
  while (client.available())
  {
    reponseAPI = client.readStringUntil('\r');
    strcpy(reponseJSON, reponseAPI.c_str());
    DeserializationError error = deserializeJson(doc, reponseJSON);
      if(strcmp (error.c_str(),"Ok") == 0) //revérifier
      {
        JsonArray JSONarray = doc.as<JsonArray>();
        serializeJsonPretty(doc, Debug);
        Debug.println("");
        JsonArray::iterator it = JSONarray.begin();
        JsonObject object = it->as<JsonObject>();
        JsonObject sensorAPI = object["sensor"].as<JsonObject>();
        serializeJsonPretty(sensorAPI, Debug);
        Debug.println("");
        for (JsonObject sensor : sensors_list)
        {
          if (sensorAPI["id"].as<String>() == sensor["sensor"])
          {
            currentSensor = sensorAPI["id"].as<String>();
            sensor["time"] = object["timestamp"].as<String>();
            break;
          }
        }

        JsonArray dataarray = object["sensordatavalues"].as<JsonArray>();

        for (JsonObject dataobj : dataarray)
        {
          if (dataobj["value_type"].as<String>() == pm)
          {
            Debug.print("PM: ");
            Debug.println(dataobj["value"].as<String>());
            client.stop(); //OBLIGATOIRE?
            for (JsonObject sensor : sensors_list)
            {
              if (sensorAPI["id"].as<String>() == sensor["sensor"])
              {
                sensor["value"] = dataobj["value"].as<String>();
                break;
              }
            }
          //  client.flush();
          //  client.stop();
            doc.garbageCollect(); //empty buffer of JSONdocument
            Debug.println("Check Memory:");
            Debug.print("doc:");
            Debug.println(doc.memoryUsage());
            return dataobj["value"].as<String>().toFloat();
          }
        }

        if (pm_type == 0)
        {
          display.setSegments(nop1);
          delay(1000);
          //client.flush();
         // client.stop();
          doc.garbageCollect();
          Debug.println("Check Memory:");
          Debug.print("doc:");
          Debug.println(doc.memoryUsage());
          return -1.0;
        }
    }
    else
    {
      Debug.print(F("deserializeJson() failed: "));
      Debug.println(error.c_str());
    }
  }
  display.setSegments(oups);
  delay(1000);
  //client.flush();
  //client.stop();
  doc.garbageCollect();  //ENLEVE TOUS LES GARBAGE COLLECT QUNAD DERAILLE
  Debug.println("Check Memory:");
  Debug.print("doc:");
  Debug.println(doc.memoryUsage());
  return -1.0;
}

extern const uint8_t PROGMEM gamma8[];

bool gamma_correction = GAMMA;

struct RGB interpolate(float valueSensor, int step1, int step2, int step3, int step4, int step5, bool correction)
{

	byte endColorValueR;
	byte startColorValueR;
	byte endColorValueG;
	byte startColorValueG;
	byte endColorValueB;
	byte startColorValueB;

	int valueLimitHigh;
	int valueLimitLow;
	struct RGB result;
	uint16_t rgb565;

	if (valueSensor == 0)
	{

		result.R = 0;
		result.G = 121; 
		result.B = 107;
	}
	else if (valueSensor > 0 && valueSensor <= step4)
	{
		if (valueSensor <= step1)
		{
		result.R = 0;
		result.G = 121; 
		result.B = 107;
		}
		else if (valueSensor > step1 && valueSensor <= step2)
		{
			valueLimitHigh = step2;
			valueLimitLow = step1;
      endColorValueR = 249;
      startColorValueR = 0;
      endColorValueG = 168;
      startColorValueG = 121;
      endColorValueB = 37;
      startColorValueB = 107;
		}
		else if (valueSensor > step2 && valueSensor <= step3)
		{
			valueLimitHigh = step3;
			valueLimitLow = step2;
      endColorValueR = 230;
      startColorValueR = 249;
      endColorValueG = 81;
      startColorValueG = 168;
      endColorValueB = 0;
      startColorValueB = 37;
		}
		else if (valueSensor > step3 && valueSensor <= step4)
		{

			valueLimitHigh = step4;
			valueLimitLow = step3;
      endColorValueR = 221;
      startColorValueR = 230;
      endColorValueG = 44;
      startColorValueG = 81;
      endColorValueB = 0;
      startColorValueB = 0;
		}
    else if (valueSensor > step4 && valueSensor <= step5)
		{

			valueLimitHigh = step4;
			valueLimitLow = step3;
      endColorValueR = 150;
      startColorValueR = 221;
      endColorValueG = 0;
      startColorValueG = 44;
      endColorValueB = 132;
      startColorValueB = 0;
		}

		result.R = (byte)(((endColorValueR - startColorValueR) * ((valueSensor - valueLimitLow) / (valueLimitHigh - valueLimitLow))) + startColorValueR);
		result.G = (byte)(((endColorValueG - startColorValueG) * ((valueSensor - valueLimitLow) / (valueLimitHigh - valueLimitLow))) + startColorValueG);
		result.B = (byte)(((endColorValueB - startColorValueB) * ((valueSensor - valueLimitLow) / (valueLimitHigh - valueLimitLow))) + startColorValueB);
	}
	else if (valueSensor > step5)
	{
		result.R = 150;
		result.G = 0; //violet
		result.B = 132;
	}
	else
	{
		result.R = 0;
		result.G = 0;
		result.B = 0;
	}

	//Gamma Correction

	if (correction == true)
	{
		result.R = pgm_read_byte(&gamma8[result.R]);
		result.G = pgm_read_byte(&gamma8[result.G]);
		result.B = pgm_read_byte(&gamma8[result.B]);
	}

	rgb565 = ((result.R & 0b11111000) << 8) | ((result.G & 0b11111100) << 3) | (result.B >> 3);
	//Debug.println(rgb565); // to get list of color if drawGradient is acitvated
	return result;
}

void setup()
{
  Debug.begin(115200);
  Debug.println("Startup!");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonpush, FALLING);

  display.setBrightness(7);
  display.setSegments(initialisation);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_COUNT); //swap R and G !
  FastLED.setBrightness(cfg::brightness); //max=255

  esp_chipid = std::move(String(ESP.getChipId()));
  esp_mac_id = std::move(String(WiFi.macAddress().c_str()));
  esp_mac_id.replace(":", "");
  esp_mac_id.toLowerCase();

  cfg::initNonTrivials(esp_chipid.c_str());
  WiFi.persistent(false);
  if ((clock_selftest_failed = !ESP.checkFlashConfig() /* after 2.7.0 update: || !ESP.checkFlashCRC() */))
  {
    Debug.println("ERROR: SELF TEST FAILED!");
    SOFTWARE_VERSION += F("-STF");
  }

  init_config();
  connectWifi();
  setup_webserver();
  Debug.print("\nChipId: ");
  Debug.println(esp_chipid);
  Debug.print("\nMAC Id: ");
  Debug.println(esp_mac_id);

  delay(50);

  if(connected == true)
  {
  display.setSegments(done);
  liste_save = cfg::sensors_chosen;
 
  char delimiters[] = ",";
  char *ptr;
  unsigned int index = -1;

  ptr = strtok(cfg::sensors_chosen, delimiters);

  while (ptr != NULL)
  {
    index += 1;
    strcpy(array_sensors[index], ptr);
    ptr = strtok(NULL, delimiters);
  }

    Debug.print("Size of array: ");
    Debug.println(index+1);
    array_size = index + 1;

    for (unsigned int i = 0; i < (array_size); i++)
    {
      Debug.println(array_sensors[i]);
      JsonObject nested = sensors_list.createNestedObject();
      nested["sensor"] = (String)array_sensors[i];
      nested["value"] = "";
      nested["time"] = "";
      }

    char sensorNr[6];
    if (cfg::random_order == true)
    {
      unsigned int random_index = rand() % (array_size); //+2? index+1
      sprintf(sensorNr, "%s", array_sensors[random_index]);
      last_index = random_index;
    }
    else{
      sprintf(sensorNr, "%s", array_sensors[0]);
      last_index = 0;
    }
    unsigned int length_url = strlen(URL_API_SENSORCOMMUNITY) + 1 + strlen(sensorNr +1);
    char url_ok[length_url];
    strcpy(url_ok, URL_API_SENSORCOMMUNITY);
    strcat(url_ok, sensorNr);
    strcat(url_ok, "/");

    Debug.print("URL API:");
    Debug.println(url_ok);
    PMvalue = getData(url_ok, cfg::pm_choice); //first getdata
    serializeJsonPretty(sensors_json, Debug);
    Debug.println("");
    
      switch(cfg::pm_choice) {
  case 0:
    displayColor = interpolate(PMvalue, 10, 20, 40, 60, 100, gamma_correction);
    break;
  case 1:
    displayColor = interpolate(PMvalue, 20, 40, 60, 100, 500, gamma_correction);
    break;
  case 2:
    displayColor = interpolate(PMvalue,10, 20, 40, 60, 100, gamma_correction);
    break;
}

    colorLED = CRGB(displayColor.R, displayColor.G, displayColor.B);
    FastLED.setBrightness(cfg::brightness);
    fill_solid(leds, LED_COUNT, colorLED);
    FastLED.show();
    LED_milli = millis();
    SCcall = millis();
    timeClient.begin();
    timeClient.setTimeOffset(cfg::time_offset * 3600); //set offset
    timeClient.update(); //first update
    Debug.println(timeClient.getFormattedTime());
    hours = timeClient.getHours();
    minutes = timeClient.getMinutes();
    display.setBrightness(7);
    display.showNumberDecEx(hours, 0x40, true, 2, 0);
    display.showNumberDec(minutes, true, 2, 2);
    dots = true;
    blinker = millis();
    precmin = minutes;
  }
  }

void loop()
{
  if (connected == true)
  {
    timeClient.update();
    hours = timeClient.getHours();
    minutes = timeClient.getMinutes();

    // if (minutes != precmin)
    // {
      display.showNumberDec(minutes, true, 2, 2);
    //   precmin = minutes;
    // }

    if (millis() - blinker > 1000)
    {

      if (dots == true)
      {
        dots = false;
        display.showNumberDec(hours, true, 2, 0);
      }
      else
      {
        dots = true;
        display.showNumberDecEx(hours, 0x40, true, 2, 0);
      }
      blinker = millis();
    }

    if (millis() - SCcall > cfg::calling_intervall_ms) 
    {

      char sensorNr[6];
      if (cfg::random_order == true && cfg::auto_change == true)
      {
        unsigned int random_index = rand() % (array_size); //+1?
        sprintf(sensorNr, "%s", array_sensors[random_index]);
      }

      if (cfg::random_order == true && cfg::auto_change == false)
      {
        sprintf(sensorNr, "%s", array_sensors[last_index]);
      }

      if (cfg::random_order == false && cfg::auto_change == true)
      {
          if (last_index < (array_size - 1))
          {
            last_index += 1;
          }
          else
          {
            last_index = 0;
          }
    
        sprintf(sensorNr, "%s", array_sensors[last_index]);
      }

      if (cfg::random_order == false && cfg::auto_change == false)
      {
        sprintf(sensorNr, "%s", array_sensors[last_index]); //peut changer si bouton
      }

      unsigned int length_url = strlen(URL_API_SENSORCOMMUNITY) + 1 + strlen(sensorNr + 1);
      char url_ok[length_url];
      strcpy(url_ok, URL_API_SENSORCOMMUNITY);
      strcat(url_ok, sensorNr);
      strcat(url_ok, "/");
      Debug.print("URL API:");
      Debug.println(url_ok);

      PMvalue = getData(url_ok, cfg::pm_choice);
      serializeJsonPretty(sensors_json, Debug);
      Debug.println("");


  switch(cfg::pm_choice) {
  case 0:
    displayColor = interpolate(PMvalue, 10, 20, 40, 60, 100, gamma_correction);
    break;
  case 1:
    displayColor = interpolate(PMvalue, 20, 40, 60, 100, 500, gamma_correction);
    break;
  case 2:
    displayColor = interpolate(PMvalue,10, 20, 40, 60, 100, gamma_correction);
    break;
}



      

      if (!(displayColor.R == 0 && displayColor.G == 0 && displayColor.B == 0))
      {
        colorLED = CRGB(displayColor.R, displayColor.G, displayColor.B);
        FastLED.setBrightness(cfg::brightness);
        fill_solid(leds, LED_COUNT, colorLED);
        FastLED.show();
      }

      SCcall = millis();
      sensors_json.garbageCollect(); //empty buffer of JSONdocument after geting value
      Debug.println("Check Memory:");
      Debug.print("sensors_json:");
      Debug.println(sensors_json.memoryUsage());
    }

if (force_call == true){

  char sensorNr[6];
  sprintf(sensorNr, "%s", array_sensors[last_index]);

  unsigned int length_url = strlen(URL_API_SENSORCOMMUNITY) + 1 + strlen(sensorNr + 1);
  char url_ok[length_url];
  strcpy(url_ok, URL_API_SENSORCOMMUNITY);
  strcat(url_ok, sensorNr);
  strcat(url_ok, "/");
  Debug.print("URL API:");
  Debug.println(url_ok);

  PMvalue = getData(url_ok, cfg::pm_choice);
  serializeJsonPretty(sensors_json, Debug);
  Debug.println("");


   switch(cfg::pm_choice) {
  case 0:
    displayColor = interpolate(PMvalue, 10, 20, 40, 60, 100, gamma_correction);
    break;
  case 1:
    displayColor = interpolate(PMvalue, 20, 40, 60, 100, 500, gamma_correction);
    break;
  case 2:
    displayColor = interpolate(PMvalue,10, 20, 40, 60, 100, gamma_correction);
    break;
}


  if (!(displayColor.R == 0 && displayColor.G == 0 && displayColor.B == 0))
  {
    colorLED = CRGB(displayColor.R, displayColor.G, displayColor.B);
    FastLED.setBrightness(cfg::brightness);
    fill_solid(leds, LED_COUNT, colorLED);
    FastLED.show();
  }

  if (show_id == true)
  {
    
    scanf("%s", sensorNr);
    int sensorNr_as_int = atoi(sensorNr);
    if (sensorNr_as_int < 10000)
    {
      display.showNumberDec(sensorNr_as_int, false);
      delay(1000);
      display.clear();
    }
    else if (sensorNr_as_int >= 10000 && sensorNr_as_int < 100000)
    {
    char part1[4];
    part1[0] = sensorNr[0];
    part1[1] = sensorNr[1];
    part1[2] = sensorNr[2];
    part1[3] = sensorNr[3];
    scanf("%s", part1);
    int sensorNr_as_int1 = atoi(part1);
      display.showNumberDec(sensorNr_as_int1, false);
      delay(1000);
      display.clear();
      display.showNumberDec(sensorNr_as_int % 10, false, 1, 0);
      delay(1000);
      display.clear();
    }
    else if (sensorNr_as_int >= 100000 && sensorNr_as_int < 1000000)
    {
      display.showNumberDec(sensorNr_as_int, false, 6, 0);
      delay(1000);
      display.clear();
      display.showNumberDec(sensorNr_as_int % 10, false, 2, 0);
      delay(1000);
      display.clear();
    }
    show_id = false;
  }

  SCcall = millis();
  force_call = false;
}

  if (cfg::fade_led == true)
  {
    if (millis() - LED_milli > 100){
      if (fader > 5 && down == true)
      {
        fader -= 1;
      }
      else if (fader == 5 && down == true)
      {
        fader += 1;
        down = false;
      }
      else if (fader < cfg::brightness && down == false)
      {
        fader += 1;
      }
      else if (fader == cfg::brightness && down == false)
      {
        fader -= 1;
        down = true;
      }
      FastLED.setBrightness(fader);
      fill_solid(leds, LED_COUNT, colorLED);
      FastLED.show();
      LED_milli = millis();
    } 
  }

  if (millis() > 4000000000) //limit = 4294967296 = 49 days and 17 hours
  {
    sensor_restart();
  }

    server.handleClient();
  //yield();
  MDNS.update();
  }
}

const uint8_t PROGMEM gamma8[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
	2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
	5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
	10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
	17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
	25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
	37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
	51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
	69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
	90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
	115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
	144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
	177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
	215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255};
