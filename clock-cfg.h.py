#!/usr/bin/env python3

configshape_in = """
String		current_lang
String		wlanssid
Password	wlanpwd
String		www_username
Password	www_password
String		fs_ssid
Password	fs_pwd
Bool		www_basicauth_enabled
Time		calling_intervall_ms
Time		time_for_wifi_config
String      sensors_chosen
Bool		random_order
Bool		auto_change
Bool		fade_led
UInt        pm_choice 
Int  		time_offset
"""
#can use Uchar? for 0-255 

with open("/Users/PJ/CODES/PIO_projects/SC_pendule/clock-cfg.h", "w") as h:
    print("""

// This file is generated, please do not edit.
// Change clock-cfg.h.py instead.

enum ConfigEntryType : unsigned short {
	Config_Type_Bool,
	Config_Type_UInt,
	Config_Type_Time,
	Config_Type_String,
	Config_Type_Password,
    Config_Type_Int
};

struct ConfigShapeEntry {
	enum ConfigEntryType cfg_type;
	unsigned short cfg_len;
	const char* _cfg_key;
	union {
		void* as_void;
		bool* as_bool;
		unsigned int* as_uint;
		char* as_str;
        int* as_int;
	} cfg_val;
	const __FlashStringHelper* cfg_key() const { return FPSTR(_cfg_key); }
};

enum ConfigShapeId {""", file=h)

    for cfgentry in configshape_in.strip().split('\n'):
        print("\tConfig_", cfgentry.split()[1], ",", sep='', file=h)
    print("};", file=h)

    for cfgentry in configshape_in.strip().split('\n'):
        _, cfgkey = cfgentry.split()
        print("static constexpr char CFG_KEY_", cfgkey.upper(),
              "[] PROGMEM = \"", cfgkey, "\";", sep='', file=h)

    print("static constexpr ConfigShapeEntry configShape[] PROGMEM = {",
          file=h)
    for cfgentry in configshape_in.strip().split('\n'):
        cfgtype, cfgkey = cfgentry.split()
        print("\t{ Config_Type_", cfgtype,
              ", sizeof(cfg::" + cfgkey + ")-1" if cfgtype in ('String', 'Password') else ", 0",
              ", CFG_KEY_", cfgkey.upper(),
              ", ", "" if cfgtype in ('String', 'Password') else "&",
              "cfg::", cfgkey, " },", sep='', file=h)
    print("};", file=h)
