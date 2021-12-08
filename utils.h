#ifndef utils_h
#define utils_h

#include <WString.h>

#include <Hash.h>
#include <coredecls.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

constexpr unsigned SMALL_STR = 64-1;
constexpr unsigned MED_STR = 256-1;
constexpr unsigned LARGE_STR = 512-1;
constexpr unsigned XLARGE_STR = 1024-1;

#define RESERVE_STRING(name, size) String name((const char*)nullptr); name.reserve(size)

#define UPDATE_MIN(MIN, SAMPLE) if (SAMPLE < MIN) { MIN = SAMPLE; }
#define UPDATE_MAX(MAX, SAMPLE) if (SAMPLE > MAX) { MAX = SAMPLE; }
#define UPDATE_MIN_MAX(MIN, MAX, SAMPLE) { UPDATE_MIN(MIN, SAMPLE); UPDATE_MAX(MAX, SAMPLE); }

extern String sha1Hex(const String& s);
extern String hmac1(const String& secret, const String& s);

extern String tmpl(const __FlashStringHelper* patt, const String& value);

extern void add_table_row_from_value(String& page_content, const __FlashStringHelper* sensor, const __FlashStringHelper* param, const String& value, const String& unit);
extern void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit = nullptr);

extern int32_t calcWiFiSignalQuality(int32_t rssi);

extern String add_sensor_type(const String& sensor_text);
extern String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi);
extern String delayToString(unsigned time_ms);

extern String check_display_value(double value, double undef, uint8_t len, uint8_t str_len);
extern void add_Value2Json(String& res, const __FlashStringHelper* type, const String& value);
extern void add_Value2Json(String& res, const __FlashStringHelper* type, const __FlashStringHelper* debug_type, const float& value);

extern void configureCACertTrustAnchor(WiFiClientSecure* client);
extern bool launchUpdateLoader(const String& md5);

extern float readCorrectionOffset(const char* correction);

namespace cfg {
	extern unsigned debug;
}


class LoggingSerial : public HardwareSerial
{

public:
	LoggingSerial();
	size_t write(uint8_t c) override;
	size_t write(const uint8_t *buffer, size_t size) override;
	String popLines();

private:
	std::unique_ptr<circular_queue<uint8_t>> m_buffer;
};

extern class LoggingSerial Debug;

#endif