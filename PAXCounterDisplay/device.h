#pragma once


#define DEVICE_NAME_LENGTH 10

struct DeviceSettings {
	char deviceNane[DEVICE_NAME_LENGTH];
	int version;
};

struct DeviceSettings deviceSettings;

// {"v":1, "c" : "node", "o" : "ver"}
void do_device_version(JsonObject& root, char* resultBuffer)
{
	TRACELN("Getting version");
	int length = sprintf(resultBuffer, "{\"version\":%d,", deviceSettings.version);


	build_command_reply(WORKED_OK, root, resultBuffer + length);
}

// {"v":1, "c" : "node", "o" : "getdevname"}
void do_get_device_name(JsonObject & root, char* resultBuffer)
{
	TRACELN("Getting device name");

	int length = sprintf(resultBuffer, "{\"nodename\":\"%s\",", deviceSettings.deviceNane);
	build_command_reply(WORKED_OK, root, resultBuffer + length);
}

// {"v":1, "t" : "sensor01", "c" : "node", "o" : "reset"}
void do_reset(JsonObject & root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		ESP.restart();
	}
}

// {"v":1, "t" : "sensor01", "c" : "node", "o" : "devname", "val":"sensor01"}
void do_device_name(JsonObject & root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, deviceSettings.deviceNane, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(deviceSettings.deviceNane, root, "val", DEVICE_NAME_LENGTH - 1);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

OptionDecodeItems deviceOptionDecodeItems[] = {
	{"ver",
		"Get the software version of the device",
		"{\"v\":1, \"c\" : \"device\", \"o\" : \"ver\"}",
		do_device_version},
	{"getdevname",
		"Get the device name",
		"{\"v\":1, \"c\" : \"device\", \"o\" : \"getdevname\"}",
		do_get_device_name},
	{"devname",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"device\", \"o\" : \"devname\", \"val\":\"sensor01\"}",
		"Get and set the device name",
		do_device_name},
	{"reset",
		"Reset the device",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"device\", \"o\" : \"reset\"}",
		do_reset },
};

void reset_device()
{
	deviceSettings.version = 1;
	strcpy(deviceSettings.deviceNane, "sensor01");
}

bool isThisDevice(const char* nameToTest)
{
	return strcasecmp(nameToTest, deviceSettings.deviceNane) == 0;
}

bool isThisVersion(int versionToTest)
{
	return deviceSettings.version == versionToTest;
}

void setup_device()
{
}

void loop_settings()
{
}

void loop_settings_proc1()
{
}
