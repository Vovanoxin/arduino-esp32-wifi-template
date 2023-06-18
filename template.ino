#include <WiFi.h>
#include <WebServer.h>
#include <regex.h>
#include <EEPROM.h>

/*Put your SSID & Password*/
const char* ap_ssid = "esp32s2";  // Enter SSID here
const char* ap_pass = "esp32s2s2";  //Enter Password here

int scanned_number = 0;

WebServer server(80);
const int buttonPin = 14;

const unsigned int eeprom_ssid_address = 0;
const unsigned int eeprom_pass_address = eeprom_ssid_address + 34; 
const unsigned int max_password_length = 64;

bool is_ap = false;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(115200);
  EEPROM.begin(512);

  if (digitalRead(buttonPin) == LOW) {
    is_ap = true;
    scanned_number = WiFi.scanNetworks();
    WiFi.softAP(ap_ssid, ap_pass);
    server.on("/", HTTP_GET, handle_main);
    server.on("/save", HTTP_POST, handle_save);

    server.onNotFound(handle_NotFound);
    server.begin();
    Serial.println("Server should have been started");
    return;
  }

  // default path
  String ssid = load_ssid();
  Serial.println(ssid);
  String pass = load_pass();
  Serial.println(pass); 
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

}
void loop() {
  if (is_ap) {
    server.handleClient();
  }
}

void handle_main() {
  server.send(200, "text/html", main_html());
  Serial.print("handle_OnConnect was called\n");
}

void handle_save() {
  String ssid_new = server.arg("ssid");
  String pass_new = server.arg("pass");
  bool ssid_valid = validate_ssid(ssid_new);
  bool pass_valid = validate_pass(pass_new);
  if (ssid_valid && pass_valid) {
    server.send(200, "text/html", success_html());
    save_ssid(ssid_new);
    save_pass(pass_new);
    delay(5000);
    ESP.restart();
  } else {
    server.send(200, "text/html", fail_html(ssid_valid, pass_valid));
  }
  
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String main_html(){
  String page_start="<!DOCTYPE html><html lang=\"en\"><head> <style>.page{background-color: #FF9E9E; margin: 0;}.main{width: 320px; display: flex; justify-content: center; margin: 40px auto;}.cointainer-header{max-width: 320px; max-height: 50px; background-color: #FFE5BD; padding: 16px 44px;}.header-text{color: black; font-family: 'Inter'; font-style: normal; font-weight: 400; font-size: 16px; width: 232px; text-align: center; margin: 0;}.user-input-form{display: flex; flex-direction: column; margin: 20px 0;}.user-input-input{background-color: #FFD494; margin: 0 0 10px; padding: 5px 10px; border: none;}.user-input-button{background-color: #B8CE2E; padding: 5px 10px; border: none;}.user-input-button:hover{background-color: #a5b829; cursor: pointer;}.container{background-color: #FFD494;}.networks-container{display: flex; flex-direction: column; align-items: center;}.networks-network{width: 280px; height: 20px; background-color: #B8CE2E; padding: 4px 5px; margin: 10px 0;}.network-name{margin: 0; font-family: 'Inter'; font-style: normal; font-weight: 400; font-size: 16px; line-height: 19px; color: #000000;}</style> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Arduino UI</title></head><body class=\"page\"> <main class=\"main\"> <div class=\"user-input-container\"> <div class=\"cointainer-header\"><p class=\"header-text\">Enter WiFi SSID and password</p></div><form class=\"user-input-form\" action='/save' method='POST'> <input class=\"user-input-input\" type=\"text\" name='ssid' placeholder=\"ssid\"> <input class=\"user-input-input\" type=\"text\" name='pass' placeholder=\"pass\"> <input class=\"user-input-button\" type=\"submit\" value=\"Submit\"> </form> <div class=\"container\"> <div class=\"cointainer-header\"><p class=\"header-text\">Scanned WiFi Networks</p></div><div class=\"networks-container\">";
  String scanned_networks = "";
  String page_end = "</div></div></div></main></body></html>";
  for (int i = 0; i < scanned_number; ++i) {
    scanned_networks += "<div class=\"networks-network\"><p class=\"network-name\">" + WiFi.SSID(i) + "</p></div>";
  }


  
  return page_start + scanned_networks + page_end;
}

bool validate_ssid(const String &pass) {
  if(pass.length() <= 32)
    return true;
  return false;
}

bool validate_pass(const String &pass) {
  if(pass.length() >= 8)
    return true;
  return false;
}

void save_ssid(const String &ssid) {
  int length = ssid.length();
  

  // Write the length of the SSID to the EEPROM
  EEPROM.write(eeprom_ssid_address, length);

  // Write each character of the SSID to the EEPROM
  for (int i = 0; i < length; i++) {
    EEPROM.write(eeprom_ssid_address + 1 + i, ssid[i]);
  }

  // Commit the changes to the EEPROM
  EEPROM.commit();
}

void save_pass(const String &pass) {
  int length = pass.length();
  Serial.println(length);
  // Truncate the password if it exceeds the maximum length
  if (length > max_password_length) {
    length = max_password_length;
  }

  // Write the length of the password to the EEPROM
  EEPROM.write(eeprom_pass_address, (char) length);

  // Write each character of the password to the EEPROM
  for (int i = 0; i < length; i++) {
    EEPROM.write(eeprom_pass_address + 1 + i, pass[i]);
  }

  // Commit the changes to the EEPROM
  EEPROM.commit();
}

String load_ssid() {
  int length = EEPROM.read(eeprom_ssid_address);

  String ssid = "";
  for (int i = 0; i < length; i++) {
    ssid += char(EEPROM.read(eeprom_ssid_address + 1 + i));
  }

  return ssid;
}

String load_pass() {
  int length = EEPROM.read(eeprom_pass_address);

  String password = "";
  for (int i = 0; i < length; i++) {
    password += char(EEPROM.read(eeprom_pass_address + 1 + i));
  }

  return password;
}

String success_html() {
  String page = "<!DOCTYPE html><html lang=\"en\"><head> <style>.page{background-color: #FF9E9E; margin: 0;}.main{width: 320px; display: flex; justify-content: center; margin: 40px auto;}.cointainer-header{max-width: 320px; min-height: 50px; background-color: #FFE5BD; padding: 16px 44px;}.header-title{color: black; font-family: 'Inter'; font-style: normal; font-weight: 600; font-size: 24px; width: 232px; text-align: center; margin: 0;}.header-text{color: black; font-family: 'Inter'; font-style: normal; font-weight: 400; font-size: 16px; width: 232px; text-align: center; margin: 20px 0 0;}</style> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Arduino UI</title></head><body class=\"page\"> <main class=\"main\"> <div class=\"cointainer-header\"> <p class=\"header-title\">SSID and password were saved!</p><p class=\"header-text\">Rebooting device in 5 sec.</p></div></div></main></body></html>";
  return page;
}

String fail_html(bool ssid_valid, bool pass_valid) {
  String page_start = "<!DOCTYPE html><html lang=\"en\"><head> <style>.page{background-color: #FF9E9E; margin: 0;}.main{width: 320px; display: flex; justify-content: center; margin: 40px auto;}.cointainer-header{max-width: 320px; min-height: 50px; background-color: #FFE5BD; padding: 16px 44px; display: flex; flex-direction: column;}.header-text{color: black; font-family: 'Inter'; font-style: normal; font-weight: 400; font-size: 16px; width: 232px; text-align: center; margin: 0 0 20px;}.user-input-button{background-color: #B8CE2E; padding: 5px 10px; border: none;}.user-input-button:hover{background-color: #a5b829; cursor: pointer;}</style> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Arduino UI</title></head><body class=\"page\"> <main class=\"main\"> <div class=\"user-input-container\"> <div class=\"cointainer-header\">";
  String page_middle = "";
  if (!ssid_valid) {
    page_middle += "<p class=\"header-text\">SSID is invalid</p>";
  }
  if (!pass_valid) {
    page_middle += "<p class=\"header-text\">Password is invalid</p>";    
  }
  String page_end = "<button onclick=\"window.location.href='/'\">Try again</button> </div></div></main></body></html>";
  return page_start + page_middle + page_end;
}