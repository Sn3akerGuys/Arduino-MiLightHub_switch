/* Written by JelleWho https://github.com/jellewie
   https://github.com/jellewie/Arduino-WiFiManager

   1. Load hardcoded data
   2. Load EEPROM data and save data
   3. while(no data) Set up AP mode and wait for user data
   4. try connecting, if (not) {GOTO3}

   NOTES
   DO NOT USE: Prefix 'WiFiManager' for anything
   DO NOT USE: Global variables of 'i' 'j' 'k'
   Use char(") in any of input stings on the webpage, use char(') if you need it. char(") will break a lot
   WIFI does not auto disconnect after you change these settings (altough settings are saved)

   You can use "strip_ip, gateway_ip, subnet_mask" to set a static connection

   HOW TO ADD CUSTOM CALUES
   -"WiFiManager_VariableNames" Add the 'AP settings portal' name
   -"WiFiManager_EEPROM_SIZE"   [optional] Make sure it is big enough for your needs, SIZE_SSID+SIZE_PASS+YourValues (1 byte = 1 character)
   -"WiFiManager_Set_Value"     Set the action on what to do on startup with this value
   -"WiFiManager_Get_Value"     [optional] Set the action on what to fill in in the boxes in the 'AP settings portal'
*/

#define WiFiManager_ConnectionTimeOutMS 10000
#define WiFiManager_APSSID "ESP32"
#define WiFiManager_EEPROM_SIZE 512           //Max Amount of chars of 'SSID + PASSWORD' (+1) (+extra custom vars) 
#define WiFiManager_EEPROM_Seperator char(9)  //use 'TAB' as a seperator 
#ifdef SerialEnabled
#define WiFiManager_SerialEnabled             //Disable to not send Serial debug feedback
#endif //SerialEnabled

#ifdef SecondSwitch
const String WiFiManager_VariableNames[] {"SSID", "Password", "MiLight_IP", "Button A1", "Button A2", "Button A3", "Button A4", "LightA ID", "LightA type", "LightA group", "Button B1", "Button B2", "Button B3", "Button B4", "LightB ID", "LightB type", "LightB group"};
#else
const String WiFiManager_VariableNames[] {"SSID", "Password", "MiLight_IP", "Button A1", "Button A2", "Button A3", "Button A4", "LightA ID", "LightA type", "LightA group"};
#endif //SecondSwitch
const byte WiFiManager_Settings = sizeof(WiFiManager_VariableNames) / sizeof(WiFiManager_VariableNames[0]); //Why filling this in if we can automate that? :)
const byte WiFiManager_EEPROM_SIZE_SSID = 16;    //Howmany characters can be in the SSID
const byte WiFiManager_EEPROM_SIZE_PASS = 16;

bool WiFiManager_WaitOnAPMode = true;       //This holds the flag if we should wait in Apmode for data
bool WiFiManager_SettingsEnabled = false;   //This holds the flag to enable settings, else it would not responce to settings commands
int  WiFiManager_EEPROM_USED = 0;           //Howmany bytes we have used for data in the EEPROM
//#define strip_ip, gateway_ip, subnet_mask to use static IP

#ifndef ssid
char ssid[WiFiManager_EEPROM_SIZE_SSID] = "";
#endif //ssid

#ifndef password
char password[WiFiManager_EEPROM_SIZE_PASS] = "";
#endif //password

#include <EEPROM.h>

byte WiFiManager_Start() {
  WiFiManager_Status_Start();
  //starts wifi stuff, only returns when connected. will create Acces Point when needed
  /* <Return> <meaning>
     2 Can't begin EEPROM
     3 Can't write [all] data to EEPROM
  */
  if (byte temp = LoadData()) return temp;
  bool WiFiManager_Connected = false;
  while (!WiFiManager_Connected) {
    if ((strlen(ssid) == 0 or strlen(password) == 0))
      WiFiManager_APMode();                 //No good ssid or password, entering APmode
    else {
      if (WiFiManager_Connect(WiFiManager_ConnectionTimeOutMS)) //try to connected to ssid password
        WiFiManager_Connected = true;
      else
        password[0] = (char)0;              //Clear this so we will enter AP mode (*just clearing first bit)
    }
  }
  WiFiManager_Status_Done();
#ifdef WiFiManager_SerialEnabled
  Serial.print("WM: My ip = ");
  Serial.println(WiFi.localIP()); //Just send it's IP on boot to let you know
#endif //WiFiManager_SerialEnabled
  return 1;
}
byte LoadData() {     //Only load data from EEPROM to memory
  if (!EEPROM.begin(WiFiManager_EEPROM_SIZE))
    return 2;
  String WiFiManager_a = WiFiManager_LoadEEPROM();
#ifdef WiFiManager_SerialEnabled
  Serial.println("WM: EEPROM data=" + WiFiManager_a);
#endif //WiFiManager_SerialEnabled
  if (WiFiManager_a != String(WiFiManager_EEPROM_Seperator)) {    //If there is data in EEPROM
    for (byte i = 1; i < WiFiManager_Settings + 1; i++) {
      byte j = WiFiManager_a.indexOf(char(WiFiManager_EEPROM_Seperator));
      if (j == 255)
        j = WiFiManager_a.length();
      String WiFiManager_TEMP_Value = WiFiManager_a.substring(0, j);
      if (WiFiManager_TEMP_Value != "")                     //If there is a value
        WiFiManager_Set_Value(i, WiFiManager_TEMP_Value);   //set the value in memory (and thus overwrite the Hardcoded stuff)
      WiFiManager_a = WiFiManager_a.substring(j + 1);
    }
  }
  return 0;
}
String WiFiManager_LoadEEPROM() {
  String WiFiManager_Value;
#ifdef WiFiManager_SerialEnabled
  Serial.print("WM: EEPROM LOAD");
#endif //WiFiManager_SerialEnabled
  for (int i = 0; i < WiFiManager_EEPROM_SIZE; i++) {
    byte WiFiManager_Input = EEPROM.read(i);
    WiFiManager_EEPROM_USED = WiFiManager_Value.length();
    if (WiFiManager_Input == 255) {              //If at the end of data
#ifdef WiFiManager_SerialEnabled
      Serial.println();
#endif //WiFiManager_SerialEnabled
      return WiFiManager_Value;                 //Stop and return all data stored
    }
    if (WiFiManager_Input == 0)                 //If no data found (NULL)
      return String(WiFiManager_EEPROM_Seperator);
    WiFiManager_Value += char(WiFiManager_Input);
#ifdef WiFiManager_SerialEnabled
    Serial.print("_" + String(char(WiFiManager_Input)) + "_");
#endif //WiFiManager_SerialEnabled
  }
#ifdef WiFiManager_SerialEnabled
  Serial.println();
#endif //WiFiManager_SerialEnabled
  return String(WiFiManager_EEPROM_Seperator);  //ERROR; [maybe] not enough space
}
bool WiFiManager_WriteEEPROM() {
  String WiFiManager_StringToWrite;                                   //Save to mem: <SSID>
  for (byte i = 0; i < WiFiManager_Settings; i++) {
    WiFiManager_StringToWrite += WiFiManager_Get_Value(i + 1, true);  //^            <Seperator>
    if (WiFiManager_Settings - i > 1)
      WiFiManager_StringToWrite += WiFiManager_EEPROM_Seperator;      //^            <Value>  (only if there more values)
  }
  WiFiManager_StringToWrite += char(255);                             //^            <emthy bit> (we use a emthy bit to mark the end)
#ifdef WiFiManager_SerialEnabled
  Serial.println("WM: EEPROM WRITE; '" + WiFiManager_StringToWrite + "'");
#endif //WiFiManager_SerialEnabled
  if (WiFiManager_StringToWrite.length() > WiFiManager_EEPROM_SIZE)   //If not enough room in the EEPROM
    return false;                                                     //Return false; not all data is stored
  for (int i = 0; i < WiFiManager_StringToWrite.length(); i++)        //For each character to save
    EEPROM.write(i, (int)WiFiManager_StringToWrite.charAt(i));        //Write it to the EEPROM
  EEPROM.commit();
  WiFiManager_EEPROM_USED = WiFiManager_StringToWrite.length();
  return true;
}
void WiFiManager_handle_Connect() {
  if (!WiFiManager_SettingsEnabled)   //If settingscommand are disabled
    return;                           //Stop right away, and do noting
  String WiFiManager_Temp_HTML = "<strong>" + String(WiFiManager_APSSID) + " settings</strong><br><br><form action=\"/setup?\" method=\"get\">";
  for (byte i = 1; i < WiFiManager_Settings + 1; i++)
    WiFiManager_Temp_HTML += "<div><label>" + WiFiManager_VariableNames[i - 1] + "</label><input type=\"text\" name=\"" + i + "\" value=\"" + WiFiManager_Get_Value(i, false) + "\"></div>";
  WiFiManager_Temp_HTML += "<button>Send</button></form>";
  WiFiManager_Temp_HTML += String(WiFiManager_EEPROM_USED) + "/" + String(WiFiManager_EEPROM_SIZE) + " Bytes used";
  server.send(200, "text/html", WiFiManager_Temp_HTML);
}
void WiFiManager_handle_Settings() {
  if (!WiFiManager_SettingsEnabled)   //If settingscommand are disabled
    return;                           //Stop right away, and do noting
  String WiFiManager_MSG = "";
  int   WiFiManager_Code = 200;
  for (int i = 0; i < server.args(); i++) {
    String WiFiManager_ArguName = server.argName(i);
    String WiFiManager_ArgValue = server.arg(i);
    WiFiManager_ArgValue.trim();
    WiFiManager_ArgValue.replace("'", "\"");            //Make sure to change char(') since we can't use that, change to char(")
    int j = WiFiManager_ArguName.toInt();
    if (j > 0 and j < 255 and WiFiManager_ArgValue != "") {
      if (WiFiManager_Set_Value(j, WiFiManager_ArgValue))
        WiFiManager_MSG += "Succesfull '" + WiFiManager_ArguName + "'='" + WiFiManager_ArgValue + "'" + char(13);
      else
        WiFiManager_MSG += "ERROR Set; '" + WiFiManager_ArguName + "'='" + WiFiManager_ArgValue + "'" + char(13);
    } else {
      WiFiManager_Code = 422;   //Flag we had a error
      WiFiManager_MSG += "ERROR ID; '" + WiFiManager_ArguName + "'='" + WiFiManager_ArgValue + "'" + char(13);
    }
  }
  WiFiManager_WaitOnAPMode = false;     //Flag we have input data, and we can stop waiting in APmode on data
  WiFiManager_WriteEEPROM();
  WiFiManager_MSG += String(WiFiManager_EEPROM_USED) + "/" + String(WiFiManager_EEPROM_SIZE) + " Bytes used";
  server.send(WiFiManager_Code, "text/plain", WiFiManager_MSG);
  for (byte i = 50; i > 0; i--) {   //Add some delay here, to send feedback to the client, i is delay in MS to still wait
    server.handleClient();
    delay(1);
  }
}
void WiFiManager_StartServer() {
  static bool ServerStarted = false;
  if (!ServerStarted) {             //If the server hasn't started yet
    ServerStarted = true;
    server.begin();                 //Begin server
  }
}
void WiFiManager_EnableSetup(bool WiFiManager_TEMP_State) {
#ifdef WiFiManager_SerialEnabled
  if (WiFiManager_TEMP_State) {
    Serial.print("WM: Settings page online ip=");
    Serial.println(WiFi.softAPIP());
  } else
    Serial.print("WM: Settings page offline");
#endif //WiFiManager_SerialEnabled
  WiFiManager_SettingsEnabled = WiFiManager_TEMP_State;
}
byte WiFiManager_APMode() {
  //IP of AP = 192.168.4.1
  /* <Return> <meaning>
    2 soft-AP setup Failed
  */
  if (!WiFi.softAP(WiFiManager_APSSID))
    return 2;
  WiFiManager_EnableSetup(true); //Flag we need to responce to settings commands
  WiFiManager_StartServer();          //start server (if we havn't already)
#ifdef WiFiManager_SerialEnabled
  Serial.print("WM: APMode on; SSID=" + String(WiFiManager_APSSID) + "ip=");
  Serial.println(WiFi.softAPIP());
#endif //WiFiManager_SerialEnabled
  while (WiFiManager_WaitOnAPMode) {
    if (TickEveryMS(100)) WiFiManager_Status_Blink(); //Let the LED blink to show we are not connected
    server.handleClient();
  }
  WiFiManager_WaitOnAPMode = true;      //reset flag for next time
  WiFiManager_EnableSetup(false);  //Flag to stop responce to settings commands
  return 1;
}
bool WiFiManager_Connect(int WiFiManager_TimeOutMS) {
#ifdef WiFiManager_SerialEnabled
  Serial.println("WM: Connecting to ssid='" + String(ssid) + "' password='" + String(password) + "'");
#endif //WiFiManager_SerialEnabled
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
#if defined(strip_ip) && defined(gateway_ip) && defined(subnet_mask)
  WiFi.config(strip_ip, gateway_ip, subnet_mask);
#endif
  unsigned long WiFiManager_StopTime = millis() + WiFiManager_TimeOutMS;
  while (WiFi.status() != WL_CONNECTED) {
    if (WiFiManager_StopTime < millis()) {    //If we are in overtime
#ifdef WiFiManager_SerialEnabled
      Serial.println("WM: Could not connect withing ='" + String(WiFiManager_TimeOutMS) + "'ms to given WIFI, aborting :" + String(WiFi.status()));
#endif //WiFiManager_SerialEnabled
      return false;
    }
    if (TickEveryMS(500)) WiFiManager_Status_Blink(); //Let the LED blink to show we are trying to connect
  }
  return true;
}
bool WiFiManager_Set_Value(byte WiFiManager_ValueID, String WiFiManager_Temp) {
#ifdef WiFiManager_SerialEnabled
  Serial.println("WM: Set current value: " + String(WiFiManager_ValueID) + " = " + WiFiManager_Temp);
#endif //WiFiManager_SerialEnabled
  switch (WiFiManager_ValueID) {
    case 1:
      WiFiManager_Temp.toCharArray(ssid, WiFiManager_Temp.length() + 1);
      break;
    case 2:
      for (byte i = 0; i < String(WiFiManager_Temp).length(); i++) {
        if (WiFiManager_Temp.charAt(i) != '*') {              //if the password is set (and not just the '*****' we have given the client)
          WiFiManager_Temp.toCharArray(password, WiFiManager_Temp.length() + 1);
          return true;                                        //Stop for loop
        }
      }
      return false;                                           //Not set, the password was just '*****'
      break;
    case 3:
      WiFiManager_Temp.toCharArray(MiLight_IP, 16);
      break;
    case 4:
      CommandsA[0] = WiFiManager_Temp;
      break;
    case 5:
      CommandsA[1] = WiFiManager_Temp;
      break;
    case 6:
      CommandsA[2] = WiFiManager_Temp;
      break;
    case 7:
      CommandsA[3] = WiFiManager_Temp;
      break;
    case 8:
      LightA.device_id = WiFiManager_Temp;
      break;
    case 9:
      LightA.remote_type = WiFiManager_Temp;
      break;
    case 10:
      LightA.group_id = WiFiManager_Temp.toInt();
      break;
#ifdef SecondSwitch
    case 11:
      CommandsB[0] = WiFiManager_Temp;
      break;
    case 12:
      CommandsB[1] = WiFiManager_Temp;
      break;
    case 13:
      CommandsB[2] = WiFiManager_Temp;
      break;
    case 14:
      CommandsB[3] = WiFiManager_Temp;
      break;
    case 15:
      LightB.device_id = WiFiManager_Temp;
      break;
    case 16:
      LightB.remote_type = WiFiManager_Temp;
      break;
    case 17:
      LightB.group_id = WiFiManager_Temp.toInt();
      break;
#endif //SecondSwitch
  }
  return true;
}
String WiFiManager_Get_Value(byte WiFiManager_ValueID, bool WiFiManager_Safe) {
#ifdef WiFiManager_SerialEnabled
  Serial.print("WM: Get current value of: " + String(WiFiManager_ValueID));
#endif //WiFiManager_SerialEnabled
  String WiFiManager_Temp_Return = "";                //Make sure to return something, if we return bad data of NULL, the HTML page will break
  switch (WiFiManager_ValueID) {
    case 1:
      WiFiManager_Temp_Return += String(ssid);
      break;
    case 2:
      if (WiFiManager_Safe)                           //If's it's safe to return password.
        WiFiManager_Temp_Return += String(password);
      else {
        for (byte i = 0; i < String(password).length(); i++)
          WiFiManager_Temp_Return += "*";
      }
      break;
    case 3:
      WiFiManager_Temp_Return = String(MiLight_IP);
      break;
    case 4:
      WiFiManager_Temp_Return = CommandsA[0];
      break;
    case 5:
      WiFiManager_Temp_Return = CommandsA[1];
      break;
    case 6:
      WiFiManager_Temp_Return = CommandsA[2];
      break;
    case 7:
      WiFiManager_Temp_Return = CommandsA[3];
      break;
    case 8:
      WiFiManager_Temp_Return = LightA.device_id;
      break;
    case 9:
      WiFiManager_Temp_Return = LightA.remote_type;
      break;
    case 10:
      WiFiManager_Temp_Return = LightA.group_id;
      break;
#ifdef SecondSwitch
    case 11:
      WiFiManager_Temp_Return = CommandsB[0];
      break;
    case 12:
      WiFiManager_Temp_Return = CommandsB[1];
      break;
    case 13:
      WiFiManager_Temp_Return = CommandsB[2];
      break;
    case 14:
      WiFiManager_Temp_Return = CommandsB[3];
      break;
    case 15:
      WiFiManager_Temp_Return = LightB.device_id;
      break;
    case 16:
      WiFiManager_Temp_Return = LightB.remote_type;
      break;
    case 17:
      WiFiManager_Temp_Return = LightB.group_id;
      break;
#endif //SecondSwitch
  }
#ifdef WiFiManager_SerialEnabled
  Serial.println(" = " + WiFiManager_Temp_Return);
#endif //WiFiManager_SerialEnabled
  WiFiManager_Temp_Return.replace("\"", "'");   //Make sure to change char(") since we can't use that, change to char(')
  return String(WiFiManager_Temp_Return);
}

//bool TickEveryMS(int _Delay) {
//  static unsigned long _LastTime = 0;       //Make it so it returns 1 if called for the FIST time
//  if (millis() > _LastTime + _Delay) {
//    _LastTime = millis();
//    return true;
//  }
//  return false;
//}
//Some status updates to the user
#define WiFiManager_LED LED_BUILTIN           //The LED to give feedback on (like blink on error)
void WiFiManager_Status_Start() {
  pinMode(WiFiManager_LED, OUTPUT);
  digitalWrite(WiFiManager_LED, HIGH);
}
void WiFiManager_Status_Done() {
  digitalWrite(WiFiManager_LED, LOW);
}
void WiFiManager_Status_Blink() {
  digitalWrite(WiFiManager_LED, !digitalRead(WiFiManager_LED));
}
//Some debug functions
//void WiFiManager_ClearEEPROM() {
//  EEPROM.write(0, 0);   //We just need to clear the first one, this will spare the EEPROM write cycles and would work fine
//  EEPROM.commit();
//}
//void WiFiManager_ClearMEM() {
//  ssid[0] = (char)0;                  //Clear these so we will enter AP mode (*just clearing first bit)
//  password[0] = (char)0;              //Clear these so we will enter AP mode
//}
//String WiFiManager_return_ssid() {
//  return ssid;
//}
//String WiFiManager_return_password() {
//  return password;
//}
