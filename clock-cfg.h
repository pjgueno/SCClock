

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

enum ConfigShapeId {
	Config_current_lang,
	Config_wlanssid,
	Config_wlanpwd,
	Config_www_username,
	Config_www_password,
	Config_fs_ssid,
	Config_fs_pwd,
	Config_www_basicauth_enabled,
	Config_calling_intervall_ms,
	Config_time_for_wifi_config,
	Config_sensors_chosen,
	Config_random_order,
	Config_auto_change,
	Config_fade_led,
	Config_pm_choice,
	Config_time_offset,
};
static constexpr char CFG_KEY_CURRENT_LANG[] PROGMEM = "current_lang";
static constexpr char CFG_KEY_WLANSSID[] PROGMEM = "wlanssid";
static constexpr char CFG_KEY_WLANPWD[] PROGMEM = "wlanpwd";
static constexpr char CFG_KEY_WWW_USERNAME[] PROGMEM = "www_username";
static constexpr char CFG_KEY_WWW_PASSWORD[] PROGMEM = "www_password";
static constexpr char CFG_KEY_FS_SSID[] PROGMEM = "fs_ssid";
static constexpr char CFG_KEY_FS_PWD[] PROGMEM = "fs_pwd";
static constexpr char CFG_KEY_WWW_BASICAUTH_ENABLED[] PROGMEM = "www_basicauth_enabled";
static constexpr char CFG_KEY_CALLING_INTERVALL_MS[] PROGMEM = "calling_intervall_ms";
static constexpr char CFG_KEY_TIME_FOR_WIFI_CONFIG[] PROGMEM = "time_for_wifi_config";
static constexpr char CFG_KEY_SENSORS_CHOSEN[] PROGMEM = "sensors_chosen";
static constexpr char CFG_KEY_RANDOM_ORDER[] PROGMEM = "random_order";
static constexpr char CFG_KEY_AUTO_CHANGE[] PROGMEM = "auto_change";
static constexpr char CFG_KEY_FADE_LED[] PROGMEM = "fade_led";
static constexpr char CFG_KEY_PM_CHOICE[] PROGMEM = "pm_choice";
static constexpr char CFG_KEY_TIME_OFFSET[] PROGMEM = "time_offset";
static constexpr ConfigShapeEntry configShape[] PROGMEM = {
	{ Config_Type_String, sizeof(cfg::current_lang)-1, CFG_KEY_CURRENT_LANG, cfg::current_lang },
	{ Config_Type_String, sizeof(cfg::wlanssid)-1, CFG_KEY_WLANSSID, cfg::wlanssid },
	{ Config_Type_Password, sizeof(cfg::wlanpwd)-1, CFG_KEY_WLANPWD, cfg::wlanpwd },
	{ Config_Type_String, sizeof(cfg::www_username)-1, CFG_KEY_WWW_USERNAME, cfg::www_username },
	{ Config_Type_Password, sizeof(cfg::www_password)-1, CFG_KEY_WWW_PASSWORD, cfg::www_password },
	{ Config_Type_String, sizeof(cfg::fs_ssid)-1, CFG_KEY_FS_SSID, cfg::fs_ssid },
	{ Config_Type_Password, sizeof(cfg::fs_pwd)-1, CFG_KEY_FS_PWD, cfg::fs_pwd },
	{ Config_Type_Bool, 0, CFG_KEY_WWW_BASICAUTH_ENABLED, &cfg::www_basicauth_enabled },
	{ Config_Type_Time, 0, CFG_KEY_CALLING_INTERVALL_MS, &cfg::calling_intervall_ms },
	{ Config_Type_Time, 0, CFG_KEY_TIME_FOR_WIFI_CONFIG, &cfg::time_for_wifi_config },
	{ Config_Type_String, sizeof(cfg::sensors_chosen)-1, CFG_KEY_SENSORS_CHOSEN, cfg::sensors_chosen },
	{ Config_Type_Bool, 0, CFG_KEY_RANDOM_ORDER, &cfg::random_order },
	{ Config_Type_Bool, 0, CFG_KEY_AUTO_CHANGE, &cfg::auto_change },
	{ Config_Type_Bool, 0, CFG_KEY_FADE_LED, &cfg::fade_led },
	{ Config_Type_UInt, 0, CFG_KEY_PM_CHOICE, &cfg::pm_choice },
	{ Config_Type_Int, 0, CFG_KEY_TIME_OFFSET, &cfg::time_offset },
};
