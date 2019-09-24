#pragma once

unsigned long millis_at_last_flash;
bool want_flash = false;
bool led_lit = false;

#define WIFI_FLASH_MSECS 200
#define MQTT_FLASH_MSECS 500

struct StatusSettings {
	bool settings_led_enabled;
} statusSettings ;


// {"v":1, "t" : "Sensor01", "c" : "status", "o" : "state", "val" : "on"}
// {"v":1, "t" : "Sensor01", "c" : "status", "o" : "state", "val" : "off"}
void do_status_state(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];
		if (!option)
		{
			// no option - just a status request
			if (statusSettings.settings_led_enabled)
			{
				build_text_value_command_reply(WORKED_OK, "on", root, resultBuffer);
			}
			else
			{
				build_text_value_command_reply(WORKED_OK, "off", root, resultBuffer);
			}
			return;
		}

		if (!root["val"].is<char*>())
		{
			TRACELN("Value is missing or not a string");
			reply = INVALID_STATUS_LED_SETTING;
		}
		else
		{
			const char* option = root["val"];
			if (strcasecmp(option, "on") == 0)
			{
				statusSettings.settings_led_enabled = true;
			}
			else
			{
				if (strcasecmp(option, "off") == 0)
				{
					statusSettings.settings_led_enabled = false;
				}
				else
				{
					reply = INVALID_STATUS_LED_SETTING;
				}
			}
		}
	}

	if (reply == WORKED_OK)
		save_settings();

	build_command_reply(reply, root, resultBuffer);
}

OptionDecodeItems statusOptionDecodeItems[] = {
	{"status",
		"Controls the display of the status led",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"status\",  \"o\" : \"state\", \"val\":\"on\"}",
		do_status_state}
};


void led_off()
{
	if (!led_lit) return;
	digitalWrite(LED_BUILTIN, false);
	led_lit = false;
}

void led_on()
{
	if (led_lit) return;
	digitalWrite(LED_BUILTIN, true);
	led_lit = true;
}

void led_toggle()
{
	millis_at_last_flash = millis();
	if (led_lit)
		led_off();
	else
		led_on();
}

void update_flash(unsigned long flash_length)
{
	unsigned long millis_since_last_flash = millis() - millis_at_last_flash;

	if (millis_since_last_flash > flash_length)
		led_toggle();
}

void reset_status()
{
	statusSettings.settings_led_enabled = true;
}

void setup_status()
{
	pinMode(LED_BUILTIN, OUTPUT);
	led_off();
	millis_at_last_flash = millis();
}

void loop_status()
{
	if (!statusSettings.settings_led_enabled)
	{
		led_off();
		return;
	}

	if (wifiState == WiFiConnected)
	{
		if (mqttState == ConnectedToMQTTServer)
			led_on();
		else
			update_flash(MQTT_FLASH_MSECS);
	}
	else
	{
		update_flash(WIFI_FLASH_MSECS);
	}
}

void loop_status_proc1()
{

}
