#include <EEPROM.h>
#include <base64.hpp>
#include <WiFiClientSecure.h>
#include <ssl_client.h>
#include <WiFiUdp.h>
#include <WiFiType.h>
#include <WiFiSTA.h>
#include <WiFiServer.h>
#include <WiFiScan.h>
#include <WiFiMulti.h>
#include <WiFiGeneric.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFi.h>
#include <ETH.h>
#include <PubSubClient.h>

#define DEBUG

#ifdef DEBUG

#define TRACE(s) Serial.print(s)
#define TRACE_HEX(s) Serial.print(s, HEX)
#define TRACELN(s) Serial.println(s)
#define TRACE_HEXLN(s) Serial.println(s, HEX)

#else

#define TRACE(s)
#define TRACE_HEX(s)
#define TRACELN(s)
#define TRACE_HEXLN(s)

#endif

#define NO_OF_GPIO_PINS 8

#include "commands.h"
#include "messages.h"
#include "settings.h"
#include "device.h"
#include "PaxDisplay.h"
#include "WiFiConnection.h"
#include "mqtt.h"
#include "status.h"

TaskHandle_t Task1;
TaskHandle_t Task2;

struct CommandDescription commandDescriptionListSetup[] =
	{
		"device",
		"device settings",
		deviceOptionDecodeItems,
		sizeof(deviceOptionDecodeItems) / sizeof(OptionDecodeItems),
		(unsigned char *)&deviceSettings,
		sizeof(struct DeviceSettings),
		reset_device,
		setup_settings,
		loop_settings,
		loop_settings_proc1,

		"commands",
		"device command control",
		commandOptionDecodeItems,
		sizeof(commandOptionDecodeItems) / sizeof(OptionDecodeItems),
		(unsigned char *)&commandSettings,
		sizeof(struct CommandSettings),
		reset_commands,
		setup_commands,
		loop_commands,
		loop_commands_proc1,

		"mqtt",
		"Mqtt settings",
		mqttOptionDecodeItems,
		sizeof(mqttOptionDecodeItems) / sizeof(OptionDecodeItems),
		(unsigned char *)&mqttSettings,
		sizeof(struct MqttSettings),
		reset_MQTT_settings,
		setup_mqtt,
		loop_mqtt,
		loop_mqtt_proc1,

		"wifi",
		"Wifi configuration",
		WiFiOptionDecodeItems,
		sizeof(WiFiOptionDecodeItems) / sizeof(OptionDecodeItems),
		(unsigned char *)&wiFiSettings,
		sizeof(struct WiFiSettings),
		reset_wifi_settings,
		setup_wifi,
		loop_wifi,
		loop_wifi_proc1,

		"status",
		"Status display control",
		statusOptionDecodeItems,
		sizeof(statusOptionDecodeItems) / sizeof(OptionDecodeItems),
		(unsigned char *)&statusSettings,
		sizeof(struct StatusSettings),
		reset_status,
		setup_status,
		loop_status,
		loop_status_proc1,

		"pax",
		"Pax display control",
		paxOptionDecodeItems,
		sizeof(paxOptionDecodeItems) / sizeof(OptionDecodeItems),
		(unsigned char *)&paxDisplaySettings,
		sizeof(struct PaxDisplaySettings),
		reset_pax_display,
		setup_pax_display,
		loop_pax_display,
		loop_pax_display_proc1};

struct CommandDescription *commandDescriptionList = commandDescriptionListSetup;

int noOfCommands = sizeof(commandDescriptionListSetup) / sizeof(struct CommandDescription);

void setup_services()
{
	for (int i = 0; i < noOfCommands; i++)
	{
		Serial.printf("Setting up %s\n", commandDescriptionList[i].commandName);
		commandDescriptionList[i].setup();
	}
}

void loop_services(void *pvParameters)
{
	while (true)
	{
		for (int i = 0; i < noOfCommands; i++)
		{
			commandDescriptionList[i].loop();
		}
		delay(1);
		vTaskDelay(1);
	}
}

void loop_services_proc1(void *pvParameters)
{
	while (true)
	{
		for (int i = 0; i < noOfCommands; i++)
		{
			commandDescriptionList[i].proc1loop();
		}
		delay(1);
		vTaskDelay(1);
	}
}

void setup()
{
	Serial.begin(115200);
	Serial.println("PAX display starting");
	setup_services();
	list_command_help();

	xTaskCreatePinnedToCore(
		loop_services, /* Task function. */
		"Task1",	   /* name of task. */
		10000,		   /* Stack size of task */
		NULL,		   /* parameter of the task */
		1,			   /* priority of the task */
		&Task1,		   /* Task handle to keep track of created task */
		0);			   /* pin task to core 0 */

	xTaskCreatePinnedToCore(
		loop_services_proc1, /* Task function. */
		"Task2",			 /* name of task. */
		10000,				 /* Stack size of task */
		NULL,				 /* parameter of the task */
		1,					 /* priority of the task */
		&Task2,				 /* Task handle to keep track of created task */
		1);					 /* pin task to core 1 */
}

void loop()
{
	delay(1);
}
