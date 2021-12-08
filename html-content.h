const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_CSS[] PROGMEM = "text/css";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";
const char TXT_CONTENT_TYPE_IMAGE_PNG[] PROGMEM = "image/png";

const char DBG_TXT_START_READING[] PROGMEM = "R/ ";
const char DBG_TXT_END_READING[] PROGMEM = "/R ";
const char DBG_TXT_CHECKSUM_IS[] PROGMEM = "Checksum is: ";
const char DBG_TXT_CHECKSUM_SHOULD[] PROGMEM = "Checksum should: ";
const char DBG_TXT_DATA_READ_FAILED[] PROGMEM = "Data read failed";
const char DBG_TXT_FOUND[] PROGMEM = " ... found";
const char DBG_TXT_NOT_FOUND[] PROGMEM = " ... not found";
const char DBG_TXT_SEP[] PROGMEM = "----";

const char SENSORS_INFO[] PROGMEM = "<b>" INTL_SENSORS_INFO "</b>";

const char WEB_PAGE_HEADER[] PROGMEM = "<!DOCTYPE html><html lang='" INTL_LANG "'>\
<head>\
<meta charset='utf-8'/>\
<title>{t}</title>";

const char WEB_PAGE_STATIC_CSS[] PROGMEM = "\
body{font-family:Arial,sans-serif;margin:0}\
.content{margin:10px}\
.footer{height:48px;background:#f5f5f5;width:100%}\
.r{text-align:right}\
td{vertical-align:top}\
.v>tbody>tr:nth-child(odd){background:#efefef}\
.b{text-decoration:none;padding:10px;background:#2a9;color:#fff;display:block;width:auto;border-radius:5px;}\
.wifi{background:0 0;color:#00f;padding:5px;display:inline;border:0;}\
input[type=text]{width:100%}\
input[type=password]{width:100%}\
input[type=submit]{color:#fff;text-align:left;cursor:pointer;border-radius:5px;font-size:medium;background:#b33;padding:9px!important;width:100%;border-style:none}\
input[type=submit]:hover{background:#d44}\
.s_green{padding:9px !important;width:100%;border-style:none;background:#3ba;color:#fff;text-align:left}\
.tabs{display:flex;flex-direction:row;align-items:stretch;align-content:flex-end;justify-content:flex-start}\
.tab{padding:10px 20px;display:inline-block;color:#333}\
.panels{min-height:200px;overflow:hidden;padding:20px;border:2px solid #3ba;margin-bottom:1em;}\
.radio{display:none}.panel{display:none}\
.canvas{min-height:118px;background:#3ba;margin-bottom:20px;}\
#r4:checked~.panels>#panel4,#r1:checked~.panels>#panel1,#r3:checked~.panels>#panel3,#r2:checked~.panels>#panel2{display:block}\
#r4:checked~.tabs>#tab4,#r1:checked~.tabs>#tab1,#r3:checked~.tabs>#tab3,#r2:checked~.tabs>#tab2{background:#3ba;color:#fff}\
";

#define STATIC_PREFIX "/" INTL_LANG "_s1"

const char WEB_PAGE_HEADER_HEAD[] PROGMEM = "<meta name='viewport' content='width=device-width'/>\
<link rel='stylesheet' href='" STATIC_PREFIX "?r=css'>\
</style>\
</head><body>\
<div class='canvas'>\
<a class='b' href='/' style='background:none;display:inline'>\
<img src='" STATIC_PREFIX "?r=logo' alt='" INTL_BACK_TO_HOME "' style='float:left;margin:16px' width='100' height='89'/></a>";

const char WEB_PAGE_HEADER_BODY[] PROGMEM = "<h3 style='margin:0 10px'>" INTL_CLOCK "</h3>\
<br/><small style='color:#fff;font-weight:700'>ID: {id} ({macid})<br/>" INTL_FIRMWARE ": " SOFTWARE_VERSION_STR "/" INTL_LANG "&nbsp;(" __DATE__ ")<br/>\
</small></div><div class='content'><h4>" INTL_HOME " {n} {t}</h4>";

const char BR_TAG[] PROGMEM = "<br/>";
const char WEB_DIV_PANEL[] PROGMEM = "</div><div class='panel' id='panel{v}'>";
const char TABLE_TAG_OPEN[] PROGMEM = "<table>";
const char TABLE_TAG_CLOSE_BR[] PROGMEM = "</table>";
const char EMPTY_ROW[] PROGMEM = "<tr><td colspan='3'>&nbsp;</td></tr>";

const char WEB_PAGE_FOOTER[] PROGMEM = "<br/><br/>"
									   "<a class='b' href='/' style='display:inline;'>" INTL_BACK_TO_HOME "</a><br/><br/><br/>"
									   "</div><footer class='footer'><div style='padding:16px'>"
									   "<a href='https://github.com/pjgueno' target='_blank' rel='noreferrer' style='color:#3ba;'>"
									   "&copy;GitHub @pjgueno</a>&nbsp;&nbsp("
									   "<a href='mailto:pierre_jean@sensor.community' target='_blank' rel='noreferrer'>" INTL_REPORT_ISSUE "</a>"
									   ")</div></footer></body></html>\r\n";

const char WEB_ROOT_PAGE_CONTENT[] PROGMEM = "<a class='b' href='/values'>{t}</a><br/>\
<a class='b' href='/config'>{conf}</a><br/>\
<a class='b' href='/removeConfig'>" INTL_CONFIGURATION_DELETE "</a><br/>\
<a class='b' href='/reset'>{restart}</a><br/>\
";

const char WEB_CONFIG_SCRIPT[] PROGMEM = "<script>\
function setSSID(ssid){document.getElementById('wlanssid').value=ssid.innerText||ssid.textContent;document.getElementById('wlanpwd').focus();}\
function load_wifi_list(){var x=new XMLHttpRequest();x.open('GET','/wifi');x.onload = function(){if (x.status === 200) {document.getElementById('wifilist').innerHTML = x.responseText;}};x.send();}\
</script>";

const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM = "<h3>" INTL_CONFIGURATION_REALLY_DELETE "</h3>\
<table><tr><td><form method='POST' action='/removeConfig'>\
<input type='submit' class='s_red' name='submit' value='" INTL_DELETE "'/></form></td>\
<td><a class='b' href='/'>" INTL_CANCEL "</a></td></tr></table>\
";

const char WEB_RESET_CONTENT[] PROGMEM = "<h3>" INTL_REALLY_RESTART_SENSOR "</h3>" \
"<table><tr><td><form method='POST' action'/reset'>" \
"<input type='submit' class='s_red' name='submit' value='" INTL_RESTART "'/>"\
"</form></td><td><a class='b' href='/'>" INTL_CANCEL "</a></td></tr></table>";

const char WEB_IOS_REDIRECT[] PROGMEM = "<html><body>Redirecting...\
<script type=\"text/javascript\">\
window.location = \"http://192.168.4.1/config\";\
</script>\
</body></html>";

const char WEB_B_BR_BR[] PROGMEM = "</b><br/><br/>";
const char WEB_BRACE_BR[] PROGMEM = ")<br/>";
const char WEB_B_BR[] PROGMEM = "</b><br/>";
const char WEB_BR_BR[] PROGMEM = "<br/><br/>";
const char WEB_BR_FORM[] PROGMEM = "<br/></form>";
const char WEB_BR_LF_B[] PROGMEM = "<br/>\n<b>";
const char WEB_LF_B[] PROGMEM = "\n<b>";
const char WEB_NBSP_NBSP_BRACE[] PROGMEM = "&nbsp;&nbsp;(";
const char WEB_REPLN_REPLV[] PROGMEM = "\"{n}\":\"{v}\",";
const char WEB_PM1[] PROGMEM = "PM1";
const char WEB_PM25[] PROGMEM = "PM2.5";
const char WEB_PM10[] PROGMEM = "PM10";
