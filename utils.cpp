/************************************************************************
 *                                                                      *
 *    airRohr firmware                                                  *
 *    Copyright (C) 2016-2020  Code for Stuttgart a.o.                  *
 *    Copyright (C) 2019-2020  Dirk Mueller                             *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                      *
 ************************************************************************
 */

#include <WString.h>

#include "./intl.h"
#include "./utils.h"
#include "./defines.h"
#include "./ext_def.h"


String tmpl(const __FlashStringHelper* patt, const String& value) {
	String s = patt;
	s.replace("{v}", value);
	return s;
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* sensor, const __FlashStringHelper* param, const String& value, const String& unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{s}</td><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{s}", sensor);
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", unit);
	page_content += s;
}

void add_table_row_from_value(String& page_content, const __FlashStringHelper* param, const String& value, const char* unit) {
	RESERVE_STRING(s, MED_STR);
	s = F("<tr><td>{p}</td><td class='r'>{v}&nbsp;{u}</td></tr>");
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", String(unit));
	page_content += s;
}

int32_t calcWiFiSignalQuality(int32_t rssi) {
	// Treat 0 or positive values as 0%
	if (rssi >= 0 || rssi < -100) {
		rssi = -100;
	}
	if (rssi > -50) {
		rssi = -50;
	}
	return (rssi + 100) * 2;
}


String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi) {
	String s = F(	"<tr>"
					"<td>"
					"<a href='#wlanpwd' onclick='setSSID(this)' class='wifi'>{n}</a>&nbsp;{e}"
					"</td>"
					"<td style='width:80%;vertical-align:middle;'>"
					"{v}%"
					"</td>"
					"</tr>");
	s.replace("{n}", ssid);
	s.replace("{e}", encryption);
	s.replace("{v}", String(calcWiFiSignalQuality(rssi)));
	return s;
}

String delayToString(unsigned time_ms) {

	char buf[64];
	String s;

	if (time_ms > 2 * 1000 * 60 * 60 * 24) {
		sprintf_P(buf, PSTR("%d days, "), time_ms / (1000 * 60 * 60 * 24));
		s += buf;
		time_ms %= 1000 * 60 * 60 * 24;
	}

	if (time_ms > 2 * 1000 * 60 * 60) {
		sprintf_P(buf, PSTR("%d hours, "), time_ms / (1000 * 60 * 60));
		s += buf;
		time_ms %= 1000 * 60 * 60;
	}

	if (time_ms > 2 * 1000 * 60) {
		sprintf_P(buf, PSTR("%d min, "), time_ms / (1000 * 60));
		s += buf;
		time_ms %= 1000 * 60;
	}

	if (time_ms > 2 * 1000) {
		sprintf_P(buf, PSTR("%ds, "), time_ms / 1000);
		s += buf;
	}

	if (s.length() > 2) {
		s = s.substring(0, s.length() - 2);
	}

	return s;
}

bool launchUpdateLoader(const String& md5) {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wdeprecated-declarations"

	File loaderFile = SPIFFS.open(F("/loader.bin"), "r");
	if (!loaderFile) {
		return false;
	}

	if (!Update.begin(loaderFile.size(), U_FLASH)) {
		return false;
	}

	if (md5.length() && !Update.setMD5(md5.c_str())) {
		return false;
	}

	if (Update.writeStream(loaderFile) != loaderFile.size()) {
		return false;
	}
	loaderFile.close();

	if (!Update.end()) {
		return false;
	}

	Debug.println("Erasing SDK config.");
	ESP.eraseConfig();
	return true;
#pragma GCC diagnostic pop
}


LoggingSerial Debug;

#if defined(ESP8266)
LoggingSerial::LoggingSerial()
	: HardwareSerial(UART0), m_buffer(new circular_queue<uint8_t>(LARGE_STR))
{
}
#endif

size_t LoggingSerial::write(uint8_t c)
{
	m_buffer->push(c);
	return HardwareSerial::write(c);
}

size_t LoggingSerial::write(const uint8_t *buffer, size_t size)
{
	m_buffer->push_n(buffer, size);
	return HardwareSerial::write(buffer, size);
}

String LoggingSerial::popLines()
{
	String r;
	while (m_buffer->available() > 0)
	{
		uint8_t c = m_buffer->pop();
		r += (char)c;

		if (c == '\n' && r.length() > m_buffer->available())
			break;
	}
	return r;
}
