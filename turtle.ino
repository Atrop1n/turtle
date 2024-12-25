#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <map>

#define LIGHT_PIN 33  // Pin for relay that controls light bulb
#define HEATING_PIN 32 // Pin for relay that controls heating
#define EEPROM_SIZE 8 //Size of used EEPROM in bytes

const char* ssid = "Telekom-792190";
const char* password = "8aubtf5d59r7tghx";
const char* ntpServer = "pool.ntp.org";
const char* timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
const long gmtOffset_sec = 3600; //Time offset in seconds
const int daylightOffset_sec = 0;

AsyncWebServer server(80);  // Server on port 80
std::map<String, unsigned long> request_times; //Map that stores request counts from IP addresses

char heating_start_string[6] = "12:22"; //Default values of timers. These timers specify start and end times for both relays.
char heating_end_string[6] = "12:22";
char light_start_string[6] = "12:22";
char light_end_string[6] = "12:22";

bool switch_manual_state = false; //This variable stores state of manual control switch
bool switch_heating_state = true;//State of heating switch
bool switch_light_state = true;//State of light switch


bool state_light; //Current state of light bulb
bool state_heating; //Current state of heating bulb

//Timers as structs of type 'tm'
//Although timers are already defined in string format, these structs contain additional information such as year,month,day, which are important when comparing time values as timestamps
struct tm current_time_struct = { 0 }, light_start_struct = { 0 }, light_end_struct = { 0 }, heating_start_struct = { 0 }, heating_end_struct = { 0 }; 
time_t current_time_timestamp, light_start_timestamp, light_end_timestamp, heating_start_timestamp, heating_end_timestamp; //Timestamp, count of seconds since epoch start

void print_times() {
  //For debug
  Serial.print("Current time: " + String(ctime(&current_time_timestamp)));
  Serial.print("Heating start: " + String(ctime(&heating_start_timestamp)));
  Serial.print("Heating end: " + String(ctime(&heating_end_timestamp)));
  Serial.print("Light start: " + String(ctime(&light_start_timestamp)));
  Serial.println("Light end: " + String(ctime(&light_end_timestamp)));
}

void compare_times(bool verbose) {
  //Evalation of start/end times and webpage switches, relays are set accordingly
  if (switch_manual_state == true) {
    //Manual switching allowed only in manual mode. In this case timers are not evaluated, only switch states
    if (switch_heating_state == true) {//Evaluate switch variable
      digitalWrite(HEATING_PIN, 1);//Activate heating
      state_heating = true;
    } else if (switch_heating_state == false) {
      digitalWrite(HEATING_PIN, 0);//Deactivate heating
      state_heating = false;
    }
    if (switch_light_state == true) {//Evaluate switch variable
      digitalWrite(LIGHT_PIN, 1);//Activate light
      state_light = true;
    } else if (switch_light_state == false) {
      digitalWrite(LIGHT_PIN, 0);//Deactivate light
      state_light = false;
    }
    return;
  } else if (switch_manual_state == false) {
    //Assign state of switch to current bulb state, so when manual mode is activated, switches on the webpage reflect the bulb states
    switch_light_state = state_light;
    switch_heating_state = state_heating;
  }
  if (current_time_timestamp > heating_start_timestamp && current_time_timestamp < heating_end_timestamp) {
    //Current time is AFTER start time and BEFORE end time (during the day)
    if (state_heating == false) {
      //For debug
      Serial.println("Heating state switched to active");
      print_times();
    }
    digitalWrite(HEATING_PIN, 1); //Activate heating
    state_heating = true; //Update heating state
  } else { //It is too early in the morning, or too late in the evening
    if (state_heating == true) {
      //For debug
      Serial.println("Heating state switched to inactive");
      print_times();
    }
    digitalWrite(HEATING_PIN, 0); //Deactivate heating
    state_heating = false; //Update heating state
  }
  if (current_time_timestamp > light_start_timestamp && current_time_timestamp < light_end_timestamp) {
    //Current time is AFTER start time and BEFORE end time (during the day)
    if (state_light == false) {
      //For debug
      Serial.println("Light state switched to active");
      print_times();
    }
    digitalWrite(LIGHT_PIN, 1);//Activate light
    state_light = true;//Update light state
  } else { //It is too early in the morning, or too late in the evening
    if (state_light == true) {
      //For debug
      Serial.println("Light state switched to inactive");
      print_times();
    }
    digitalWrite(LIGHT_PIN, 0);//Deactivate light
    state_light = false;//Update light state
  }
  if (verbose) {
    //For debug
    Serial.println("Heating: " + String(state_heating));
    Serial.println("Light: " + String(state_light));
  }
}

void update_current_time() {
  //This functions updates current time from NTP server
  bool flag_reconnected = false; //This flag indicates if client was disconnected from WiFi
  while (WiFi.status() != WL_CONNECTED) { //Checks if connection is still alive, if not, loop until reconnected
    WiFi.begin(ssid, password);
    delay(1000);
    Serial.print(".");
    flag_reconnected = true;
  }
  if (flag_reconnected) {
    //If client was reconnected, display IP address
    Serial.println("Reconnected to WiFi: "+WiFi.localIP().toString());
  }
  struct tm timeinfo;
  int retry = 10;  
  while (!getLocalTime(&timeinfo) && retry-- > 0) { //10 attempts to retrieve time info from NTP server
    Serial.println("Failed to obtain time, retrying...");
    delay(1000);  
  }
  if (retry <= 0) { //If no success
    Serial.println("Failed to update time after retries");
  }
  current_time_struct = timeinfo; //Update current time
  current_time_timestamp = mktime(&current_time_struct); //Get timestamp in integral value
}

int read_EEPROM_value(int address, int min_val, int max_val, int default_val) {
  //Reads value from a given address from EEPROM. Also checks if the value is in correct range.
  int value = EEPROM.read(address);
  if (value < min_val || value > max_val) {
    Serial.printf("EEPROM value out of range at address %d. Using default %d\n", address, default_val);
    return default_val;
  }
  return value;
}

void init_times(bool verbose) {
  heating_start_struct = current_time_struct;
  heating_end_struct = current_time_struct;
  light_start_struct = current_time_struct;
  light_end_struct = current_time_struct;

  //Extracts timer values from EEPROM
  heating_start_struct.tm_hour = read_EEPROM_value(0, 0, 23, 12);
  heating_start_struct.tm_min = read_EEPROM_value(1, 0, 59, 12);
  heating_start_struct.tm_sec = 0;
  heating_end_struct.tm_hour = read_EEPROM_value(2, 0, 23, 12);
  heating_end_struct.tm_min = read_EEPROM_value(3, 0, 59, 12);
  heating_end_struct.tm_sec = 0;
  light_start_struct.tm_hour = read_EEPROM_value(4, 0, 23, 12);
  light_start_struct.tm_min = read_EEPROM_value(5, 0, 59, 12);
  light_start_struct.tm_sec = 0;
  light_end_struct.tm_hour = read_EEPROM_value(6, 0, 23, 12);
  light_end_struct.tm_min = read_EEPROM_value(7, 0, 59, 12);
  light_end_struct.tm_sec = 0;

  //Extracts hour and minute in string format from the structs
  strftime(light_start_string, 6, "%H:%M", &light_start_struct);
  strftime(light_end_string, 6, "%H:%M", &light_end_struct);
  strftime(heating_start_string, 6, "%H:%M", &heating_start_struct);
  strftime(heating_end_string, 6, "%H:%M", &heating_end_struct);

  //Extracts timestamps
  heating_start_timestamp = mktime(&heating_start_struct);
  heating_end_timestamp = mktime(&heating_end_struct);
  light_start_timestamp = mktime(&light_start_struct);
  light_end_timestamp = mktime(&light_end_struct);

  if (verbose) {
    //For debug purposes
    Serial.println("Time initialized from EEPROM");
    print_times();
  }
}
void string_to_struct(String time_string, tm* time_struct) {
  //Updates given struct with time in %HH%MM
  time_struct->tm_hour = atoi(time_string.substring(0, 2).c_str());
  time_struct->tm_min = atoi(time_string.substring(3, 5).c_str());
  time_struct->tm_sec = 0;
}
void update_times(char* heating_start, char* heating_end, char* light_start, char* light_end, bool verbose) {
  //Updates timer values. This function is called when timer values are changed on the webserver.
  heating_start_struct = current_time_struct; //'x_timestamp' variables are integer variables and are converted from 'x_struct'. The timestamp is 
                                              //number of seconds elapsed since the beginning of the epoch, which is January 1, 1970.  
                                              //In order to be able to compare individual timestamps, all fields in the 'tm' structs must be the same,
                                              //except for hours and minutes, because we are compraring on basis of hours and minutes (to check time of the day). 
                                              //That is why the 'start time' and 'end time' timer structs must contain the same year,month,day info as the 'current time' struct.
  string_to_struct(heating_start, &heating_start_struct); 
  heating_end_struct = current_time_struct;
  string_to_struct(heating_end, &heating_end_struct);
  light_start_struct = current_time_struct;
  string_to_struct(light_start, &light_start_struct);
  light_end_struct = current_time_struct;
  string_to_struct(light_end, &light_end_struct);

  heating_start_timestamp = mktime(&heating_start_struct);//Get the timestamps
  heating_end_timestamp = mktime(&heating_end_struct);
  light_start_timestamp = mktime(&light_start_struct);
  light_end_timestamp = mktime(&light_end_struct);

  if (verbose) {
    print_times();
  }
}
void save_to_EEPROM(String time_string, uint8_t byte_1, uint8_t byte_2) {
  //Saves %HH/%MM info from string into given two bytes of EEPROM
  EEPROM.write(byte_1, atoi(time_string.substring(0, 2).c_str()));//Hours
  EEPROM.write(byte_2, atoi(time_string.substring(3, 5).c_str()));//Minutes
  EEPROM.commit();
}
String processor(const String& var) {
  //This function assigns correct local values to webpage placeholders
  if (var == "HEATING_START") return heating_start_string;
  if (var == "HEATING_END") return heating_end_string;
  if (var == "LIGHT_START") return light_start_string;
  if (var == "LIGHT_END") return light_end_string;
  if (var == "TIMESTAMP") return ctime(&current_time_timestamp);
  if (var == "SWITCH_MANUAL_STATE") {
    return switch_manual_state ? "checked" : "";
  }
  if (var == "SWITCH_HEATING_STATE") {
    return state_heating ? "checked" : "";
  }
  if (var == "SWITCH_LIGHT_STATE") {
    return state_light ? "checked" : "";
  }
  return String();
}

// HTML for the web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
   <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
         body {
         text-align: center;
         background-color: rgb(194, 212, 76);
         font-family: Verdana, Geneva, Tahoma, sans-serif;
         color: rgb(33, 27, 25);
         }
         h2 {
         font-weight: 600;
         font-size: 32px;
         text-shadow: 1px 1px 1px rgb(223, 237, 76);
         }
         .highlight {
         border: 2px solid red;
         }
         /* The switch - the box around the slider */
         .switch {
         position: relative;
         display: inline-block;
         width: 60px;
         height: 34px;
         margin: 10px;
         }
         /* Hide default HTML checkbox */
         .switch input {
         opacity: 0;
         width: 0;
         height: 0;
         }
         /* The slider */
         .slider {
         position: absolute;
         cursor: pointer;
         top: 0;
         left: 0;
         right: 0;
         bottom: 0;
         background-color: #ccc;
         -webkit-transition: .4s;
         transition: .6s;
         }
         .slider:before {
         position: absolute;
         content: "";
         height: 26px;
         width: 26px;
         left: 4px;
         bottom: 4px;
         background-color: white;
         -webkit-transition: .4s;
         transition: .4s;
         }
         input:checked + .slider {
         background-color: #2196F3;
         }
         input:focus + .slider {
         box-shadow: 0 0 1px #2196F3;
         }
         input:checked + .slider:before {
         -webkit-transform: translateX(26px);
         -ms-transform: translateX(26px);
         transform: translateX(26px);
         }
         /* Rounded sliders */
         .slider.round {
         border-radius: 34px;
         }
         .slider.round:before {
         border-radius: 50%%;
         }
         .time-container {
         display: flex;
         justify-content: center;
         align-items: center;
         gap: 20px; /* Space between the inputs */
         }
         .time-container div {
         text-align: center;
         }
         h1 {
         display: block;
         font-size: 2em;
         margin-block-start: 0;
         margin-block-end: 0;
         margin-inline-start: 0px;
         margin-inline-end: 0px;
         font-weight: bold;
         unicode-bidi: isolate;
         }
         h3 {
         font-weight: normal;
         }
         .auto_manual{
         font-weight: bold;
         }
         .greyed_out {
         opacity: 0.3;
         pointer-events: none;
         }
      </style>
   </head>
   <body>
      <h3 class="auto_manual">Auto/Manual</h3>
      <label class="switch">
      <input type="checkbox" id="switch_manual" onchange="updateSwitch('switch_manual')" %SWITCH_MANUAL_STATE%>
      <span class="slider round"></span>
      </label>
      <h1>Heating</h1>
      <div class="time-container">
         <div>
            <h3>Start</h3>
            <input type="time" id="picker_heating_start" onchange="highlight('picker_heating_start')" value="%HEATING_START%"/>
            <button onclick="updateHeatingStart()">OK</button>
         </div>
         <div>
            <h3>End</h3>
            <input type="time" id="picker_heating_end" onchange="highlight('picker_heating_end')" value="%HEATING_END%"/>
            <button onclick="updateHeatingEnd()">OK</button>
         </div>
      </div>
      <label class="switch">
      <input type="checkbox" id="switch_heating" onchange="updateSwitch('switch_heating')" %SWITCH_HEATING_STATE%>
      <span class="slider round"></span>
      </label>
      <h1>Light</h1>
      <div class="time-container">
         <div>
            <h3>Start</h3>
            <input type="time" id="picker_light_start" onchange="highlight('picker_light_start')" value="%LIGHT_START%"/>
            <button onclick="updateLightStart()">OK</button>
         </div>
         <div>
            <h3>End</h3>
            <input type="time" id="picker_light_end" onchange="highlight('picker_light_end')" value="%LIGHT_END%"/>
            <button onclick="updateLightEnd()">OK</button>
         </div>
      </div>
      <label class="switch">
      <input type="checkbox" id="switch_light" onchange="updateSwitch('switch_light')"  %SWITCH_LIGHT_STATE%>
      <span class="slider round"></span>
      </label>
      <p>
      <h3>Time:</h3>
      <span id="timestamp">%TIMESTAMP%</span></p>
      <script>
         setInterval(() => fetchSwitchState("switch_manual"), 1000);
         setInterval(() => fetchSwitchState("switch_heating"), 1000);
         setInterval(() => fetchSwitchState("switch_light"), 1000);
         setInterval(() => updateValue("timestamp"), 1000);
         const switchManual = document.getElementById("switch_manual");
         if (!switchManual.checked){
         document.getElementById("switch_light").parentElement.classList.add("greyed_out");
         document.getElementById("switch_heating").parentElement.classList.add("greyed_out");
         
         }
         else {
         document.getElementById("switch_light").parentElement.classList.remove("greyed_out");
         document.getElementById("switch_heating").parentElement.classList.remove("greyed_out");
         }
         
         function highlight(id) {
          const element = document.getElementById(id);
          if (element) {
             element.classList.add("highlight");
          }
         }
         
         function updateValue(id) {
          const xhttp = new XMLHttpRequest();
          xhttp.onload = () => {
             document.getElementById(id).innerText = xhttp.responseText;
          };
          xhttp.open("GET", `/${id}`, true);
          xhttp.send();
         }
         
         function fetchSwitchState(switchId) {
          const xhttp = new XMLHttpRequest();
          xhttp.onload = () => {
             document.getElementById(switchId).checked = xhttp.responseText === "1";
          };
          xhttp.open("GET", `/get_state?switch=${switchId}`, true);
          xhttp.send();
         const switchManual = document.getElementById("switch_manual");
         if (!switchManual.checked){
         document.getElementById("switch_light").parentElement.classList.add("greyed_out");
         document.getElementById("switch_heating").parentElement.classList.add("greyed_out");
         
         }
         else {
         document.getElementById("switch_light").parentElement.classList.remove("greyed_out");
         document.getElementById("switch_heating").parentElement.classList.remove("greyed_out");
         }
         }
         
         function updateSwitch(switchId) {
          const isChecked = document.getElementById(switchId).checked ? "1" : "0";
          const xhttp = new XMLHttpRequest();
          xhttp.open("GET", `/update?switch=${switchId}&state=${isChecked}`, true);
          xhttp.send();
          fetchSwitchState(switchId);
         }
         
         function updateHeatingStart() {
          const value = document.getElementById("picker_heating_start").value;
          sendUpdate(`web_heating_start=${value}`);
          document.getElementById("picker_heating_start").classList.remove("highlight");
         }
         
         function updateHeatingEnd() {
          const value = document.getElementById("picker_heating_end").value;
          sendUpdate(`web_heating_end=${value}`);
          document.getElementById("picker_heating_end").classList.remove("highlight");
         }
         
         function updateLightStart() {
          const value = document.getElementById("picker_light_start").value;
          sendUpdate(`web_light_start=${value}`);
          document.getElementById("picker_light_start").classList.remove("highlight");
         }
         
         function updateLightEnd() {
          const value = document.getElementById("picker_light_end").value;
          sendUpdate(`web_light_end=${value}`);
          document.getElementById("picker_light_end").classList.remove("highlight");
         }
         
         function sendUpdate(params) {
          const xhttp = new XMLHttpRequest();
          xhttp.open("GET", `/update?${params}`, true);
          xhttp.send();
         }
      </script>
   </body>
</html>
)rawliteral";
void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(HEATING_PIN, OUTPUT);
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM init failed");
  }
  if (!WiFi.begin(ssid, password)) {
    Serial.println("WiFi init failed");
  }
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov");
  setenv("TZ", timezone, 1);
  tzset();
  update_current_time();
  init_times(true);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    //Specifies response when webpage is refreshed
    request->send_P(200, "text/html; charset=UTF-8", index_html, processor);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    //'/update' requests update local timer data. 
    if (request->hasParam("web_heating_start")) {
      //Update heating start time when it is changed on the webpage
      strcpy(heating_start_string, request->getParam("web_heating_start")->value().c_str());
      Serial.println("HEATING START CHANGED TO " + String(heating_start_string) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      update_times(heating_start_string, heating_end_string, light_start_string, light_end_string, true);
      save_to_EEPROM(heating_start_string, 0, 1);
    }
    if (request->hasParam("web_heating_end")) {
      //Update heating end time when it is changed on the webpage
      strcpy(heating_end_string, request->getParam("web_heating_end")->value().c_str());
      Serial.println("HEATING END CHANGED TO " + String(heating_end_string) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      update_times(heating_start_string, heating_end_string, light_start_string, light_end_string, true);
      save_to_EEPROM(heating_end_string, 2, 3);
    }
    if (request->hasParam("web_light_start")) {
      //Update light start time when it is changed on the webpage
      strcpy(light_start_string, request->getParam("web_light_start")->value().c_str());
      Serial.println("LIGHT START CHANGED TO " + String(light_start_string) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      update_times(heating_start_string, heating_end_string, light_start_string, light_end_string, true);
      save_to_EEPROM(light_start_string, 4, 5);
    }
    if (request->hasParam("web_light_end")) {
      //Update light end time when it is changed on the webpage
      strcpy(light_end_string, request->getParam("web_light_end")->value().c_str());
      Serial.println("LIGHT END CHANGED TO " + String(light_end_string) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      update_times(heating_start_string, heating_end_string, light_start_string, light_end_string, true);
      save_to_EEPROM(light_end_string, 6, 7);
    }
    if (request->hasParam("switch")) {
      //Fetch values from switches when user clicks on them
      //Retrieved boolean values are stored locally as 'switch_{switch_name}_state'
      if (request->getParam("switch")->value() == "switch_manual") {
        switch_manual_state = request->getParam("state")->value().toInt();//Get switch value
        Serial.println("STATE OF switch_manual " + String(switch_manual_state) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      } else if (request->getParam("switch")->value() == "switch_heating" && switch_manual_state == true) {
        //Update switch heating state (only if manual mode is enabled)
        switch_heating_state = request->getParam("state")->value().toInt();//Get switch value
        Serial.println("STATE OF switch_heating " + String(switch_heating_state) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      } else if (request->getParam("switch")->value() == "switch_light" && switch_manual_state == true) {
        //Update switch light state (only if manual mode is enabled)
        switch_light_state = request->getParam("state")->value().toInt();//Get switch value
        Serial.println("STATE OF switch_light " + String(switch_light_state) + ", BASED ON SERVER REQUEST, UPDATING LOCAL VALUES");
      }
      compare_times(true);//Update relay outputs to reflect switch changes
    }

    request->send(200, "text/plain", "OK");
  });

  server.on("/get_state", HTTP_GET, [](AsyncWebServerRequest* request) {
    //'/get_state' retrieves current relay output state so that it is graphically reflected in a switch state on the webpage. 
    //If a given bulb is on, its switch is displayed in 'on' state, and vice versa 
    String clientIP = request->client()->remoteIP().toString();//Get IP of client who sent the request
    unsigned long now = millis();

    if (request_times.count(clientIP) > 2 && (now - request_times[clientIP]) < 100) {
      //Place restriction on amount of request a single client can send in a time period (in this case max 2 requests for each 100 ms)
      //This is to prevent server overload
      request->send(429, "text/plain", "Too many requests, try later.");
      return;
    }
    request_times[clientIP] = now;

    if (request->getParam("switch")->value() == "switch_manual") {//Request for state of switch_manual
      request->send_P(200, "text/plain", switch_manual_state ? "1" : "0");//Send 1 or 0 according to variable value
    } else if (request->getParam("switch")->value() == "switch_heating") {//Request for state of heating bulb
      request->send_P(200, "text/plain", state_heating ? "1" : "0");//Send 1 or 0 according to variable value
    } else if (request->getParam("switch")->value() == "switch_light") {//Request for state of light bulb
      request->send_P(200, "text/plain", state_light ? "1" : "0");//Send 1 or 0 according to variable value
    } else {
      request->send(400, "text/plain", "Invalid parameter");
    }
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest* request) {//Handles request for timestamp update
    request->send(200, "text/plain", ctime(&current_time_timestamp));//Sends current time in human readable format 
  });

  server.begin();
}

void loop() {
  if (current_time_struct.tm_hour == 0 & current_time_struct.tm_min == 0) {
    //At 00:00 of each day
    Serial.println("Updating due to new day");
    update_times(heating_start_string, heating_end_string, light_start_string, light_end_string, true);//Update all timer variables with the new day
  }
  update_current_time();//Retrieve current time info from NTP server
  delay(1000);
  compare_times(false);//Evaluate switches and set relay outputs
}