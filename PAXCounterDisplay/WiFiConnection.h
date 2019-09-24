// timeout in millis
#define WIFI_START_TIMEOUT 20000
#define WIFI_DISPLAY_TIMEOUT 3000
#define WIFI_RECONNECT_TIMEOUT 10000
#define WIFI_SCAN_TIMEOUT 30000

#define WIFI_SSID_LENGTH 30
#define WIFI_PASSWORD_LENGTH 30

unsigned long wifi_timer_start;

enum WiFiState { WiFiOff, WiFiStarting, WiFiScanning, WiFiConnecting, WiFiConnected, ShowingWifiConnected, WiFiConnectFailed, ShowingWiFiFailed, WiFiNotConnected };

WiFiState wifiState;
int wifi_connect_count = 0;

struct ConnectionSettings
{
	char wifiSsid[WIFI_SSID_LENGTH];
	char wifiPassword[WIFI_PASSWORD_LENGTH];
} ;

#define NO_OF_WIFI_SETTINGS 5

struct WiFiSettings {
	ConnectionSettings wifiConnections[NO_OF_WIFI_SETTINGS];
	bool wiFiOn;
} wiFiSettings;

void reset_wifi_settings()
{
	wiFiSettings.wiFiOn = true;

	for (int i = 0; i < NO_OF_WIFI_SETTINGS; i++)
	{
		wiFiSettings.wifiConnections[i].wifiSsid[0] = 0;
		wiFiSettings.wifiConnections[i].wifiPassword[0] = 0;
	}
}

void start_wifi()
{
	wifi_timer_start = millis();
	wifiState = WiFiStarting;
}

#define WIFI_SETTING_NOT_FOUND -1

int find_wifi_setting(String ssidName)
{
	char ssidBuffer[WIFI_SSID_LENGTH];

	ssidName.toCharArray(ssidBuffer, WIFI_SSID_LENGTH);

	TRACE("Checking: ");
	TRACELN(ssidName);

	for (int i = 0; i < NO_OF_WIFI_SETTINGS; i++)
	{
		if (strcasecmp(wiFiSettings.wifiConnections[i].wifiSsid, ssidBuffer) == 0)
		{
			return i;
		}
	}
	return WIFI_SETTING_NOT_FOUND;
}

void start_wifi_scan()
{
	wifi_timer_start = millis();

	setDigitColour(myBLUE);
	WiFi.scanNetworks(true); // perform an asynchronous scan
	start_action("WiFi", "Scanning");
	wifiState = WiFiScanning;
	wifi_connect_count++;
}

void reset_wifi_and_scan()
{
	WiFi.disconnect();
	delay(1000);
	WiFi.begin();
	start_wifi_scan();
}


// {"v":1, "t" : "Sensor01", "c" : "wifi", "o" : "on"}
void do_wifi_on(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		TRACELN("Starting WiFi");
		start_wifi();
	}
	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "Sensor01", "c" : "wifi", "o" : "off"}
void do_wifi_off(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		TRACELN("Stopping WiFi");
		// not sure how to do this just yet...
	}
	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "wifi",  "o" : "ssid", "set":0, "val":"ssid"}
void do_wifi_ssid(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		if (!root["set"].is<int>())
		{
			TRACELN("Missing WiFi setting number");
			reply = MISSING_WIFI_SETTING_NUMBER;
		}
		else
		{
			int settingNo = root["set"];
			if (settingNo < 0 || settingNo >= NO_OF_WIFI_SETTINGS)
			{
				reply = INVALID_WIFI_SETTING_NUMBER;
			}
			else
			{
				const char* option = root["val"];

				if (!option)
				{
					build_text_value_command_reply(WORKED_OK, wiFiSettings.wifiConnections[settingNo].wifiSsid,
						root, resultBuffer);
					return;
				}

				reply = decodeStringValue(wiFiSettings.wifiConnections[settingNo].wifiSsid, root,
					"val", WIFI_PASSWORD_LENGTH - 1);
			}

			if (reply == WORKED_OK)
			{
				save_settings();
			}
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "wifi", "o" : "pwd", "set":0, "val":"password"}
void do_wifi_password(JsonObject & root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		if (!root["set"].is<int>())
		{
			TRACELN("Missing WiFi setting number");
			reply = MISSING_WIFI_SETTING_NUMBER;
		}
		else
		{
			int settingNo = root["set"];
			if (settingNo < 0 || settingNo >= NO_OF_WIFI_SETTINGS)
			{
				reply = INVALID_WIFI_SETTING_NUMBER;
			}
			else
			{
				reply = decodeStringValue(wiFiSettings.wifiConnections[settingNo].wifiPassword, root, "val", WIFI_PASSWORD_LENGTH - 1);
			}

			if (reply == WORKED_OK)
			{
				save_settings();
			}
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

OptionDecodeItems WiFiOptionDecodeItems[] = {
	{"on",
		"Turn WiFi on",
		"{\"v\":1, \"t\" : \"Sensor01\", \"c\" : \"wifi\", \"o\" : \"on\"}",
		do_wifi_on},
	{"off", 
		"Turn WiFi off",
		"{\"v\":1, \"t\" : \"Sensor01\", \"c\" : \"wifi\", \"o\" : \"off\"}",
		do_wifi_off},
	{"ssid", 
		"Set SSID for specified WiFi network",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"wifi\",  \"o\" : \"ssid\", \"set\":0, \"val\":\"ssid\"}",
		do_wifi_ssid},
	{"pwd", 
		"Set password for specified WiFi network", 
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"wifi\", \"o\" : \"pwd\", \"set\":0, \"val\":\"password\"}",
		do_wifi_password}
};


void WiFiEvent(WiFiEvent_t event) {
	Serial.printf("[WiFi-event] event: %d  - ", event);
	switch (event) {
	case SYSTEM_EVENT_STA_GOT_IP:
		Serial.println("WiFi connected");
		Serial.print("IP address: "); Serial.println(WiFi.localIP());
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		Serial.println("WiFi lost connection");
		WiFi.begin();   // <<<<<<<<<<<  added  <<<<<<<<<<<<<<<
		break;
	case SYSTEM_EVENT_STA_START:
		Serial.println("ESP32 station start");
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		Serial.println("ESP32 station connected to AP");
		break;
	}
}

void resetWiFiConnection()
{
}

void loop_wifi()
{
	unsigned long elapsed_time;
	int no_of_networks;
	int connectionNumber;

	elapsed_time = millis() - wifi_timer_start;

	switch (wifiState)
	{
	case WiFiStarting:
		TRACELN("WiFi Connection starting");
		start_wifi_scan();
		break;

	case WiFiScanning:
		
		no_of_networks = WiFi.scanComplete();

		if (no_of_networks == WIFI_SCAN_RUNNING)
		{
			if(elapsed_time > WIFI_SCAN_TIMEOUT)
			{
				// reset the device completely if the scan times out
				ESP.restart();
			}
			break;
		}
		TRACE("Networks found: ");
		TRACELN(no_of_networks);

		// if we get here we have some networks
		for (int i = 0; i < no_of_networks; ++i) {
			connectionNumber = find_wifi_setting(WiFi.SSID(i));
			if (connectionNumber != WIFI_SETTING_NOT_FOUND)
			{
				WiFi.begin(wiFiSettings.wifiConnections[connectionNumber].wifiSsid,
					wiFiSettings.wifiConnections[connectionNumber].wifiPassword);
				update_action(wiFiSettings.wifiConnections[connectionNumber].wifiSsid,
					"Connecting");
				setDigitColour(myYELLOW);
				wifiState = WiFiConnecting;
				break;
			}
		}

		if (wifiState != WiFiConnecting)
		{
			// didn't find a matching network
			update_action("WiFi", "No networks");
			wifi_timer_start = millis();
			wifiState = WiFiConnectFailed;
			setDigitColour(myRED);
		}

		break;

	case WiFiConnecting:

		if (WiFi.status() == WL_CONNECTED)
		{
			update_action("WiFi", "Connected OK");
			wifiState = ShowingWifiConnected;
			wifi_timer_start = millis();
			setDigitColour(myMAGENTA);
		}

		if (elapsed_time > WIFI_START_TIMEOUT)
		{
			wifi_timer_start = millis();
			update_action("WiFi", "Connect failed");
			wifiState = WiFiConnectFailed;
			setDigitColour(myRED);
		}

		break;

	case ShowingWifiConnected:
		if (elapsed_time > WIFI_DISPLAY_TIMEOUT)
		{
			end_action();
			wifiState = WiFiConnected;
		}
		break;

	case WiFiConnectFailed:
		if (elapsed_time > WIFI_DISPLAY_TIMEOUT)
		{
			wifi_timer_start = millis();
			wifiState = WiFiNotConnected;
			end_action();
		}
		break;

	case WiFiConnected:

		if (WiFi.status() != WL_CONNECTED)
		{
			wifi_timer_start = millis();
			start_action("WiFi", "Failed");
			wifiState = ShowingWiFiFailed;
			setDigitColour(myCYAN);
		}
		break;

	case ShowingWiFiFailed:

		if (elapsed_time > WIFI_DISPLAY_TIMEOUT)
		{
			reset_wifi_and_scan();
		}
		break;

	case WiFiNotConnected:

		if (elapsed_time > WIFI_RECONNECT_TIMEOUT)
		{
			reset_wifi_and_scan();
		}
		break;
	}
}

void setup_wifi()
{
	if (wiFiSettings.wiFiOn)
	{
		start_wifi();
		loop_wifi();
	}
	else
	{
		wifiState = WiFiOff;
	}
}

void loop_wifi_proc1()
{
	
}
