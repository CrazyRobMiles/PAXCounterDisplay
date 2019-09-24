#pragma once

#include "ArduinoJson-v5.13.2.h"

#define WORKED_OK 0
#define NUMERIC_VALUE_NOT_AN_INTEGER 1
#define NUMERIC_VALUE_BELOW_MINIMUM 2
#define NUMERIC_VALUE_ABOVE_MAXIMUM 3
#define INVALID_HEX_DIGIT_IN_VALUE 4
#define INCOMING_HEX_VALUE_TOO_BIG_FOR_BUFFER 5
#define VALUE_MISSING_OR_NOT_A_STRING 6
#define INCOMING_HEX_VALUE_IS_THE_WRONG_LENGTH 7
#define INVALID_OR_MISSING_TARGET_IN_RECEIVED_COMMAND 8
#define COMMAND_FOR_DIFFERENT_TARGET 9
#define INVALID_OR_MISSING_DEVICE_NAME 10
#define INVALID_DEVICE_NAME_FOR_MQTT_ON 11
#define STRING_VALUE_MISSING_OR_NOT_STRING 13
#define STRING_VALUE_TOO_LONG 14
#define INVALID_LORA_ACCESS_SETTING 15
#define INVALID_COMMAND_NAME 16
#define JSON_COMMAND_COULD_NOT_BE_PARSED 17
#define JSON_COMMAND_MISSING_VERSION 18
#define JSON_COMMAND_MISSING_COMMAND 19
#define JSON_COMMAND_MISSING_OPTION 20
#define INVALID_MQTT_STATUS_SETTING 21
#define INVALID_LORA_STATUS_SETTING 22
#define MISSING_WIFI_SETTING_NUMBER 23
#define INVALID_WIFI_SETTING_NUMBER 24
#define MISSING_GPIO_PIN_NUMBER 25
#define INVALID_GPIO_PIN_NUMBER 26
#define INVALID_STATUS_LED_SETTING 27
#define INVALID_COMMANDS_CONTROL_SETTING 28
#define INVALID_MATRIX_MESSAGE_LENGTH 29
#define MISSING_MATRIX_MESSAGE 30
#define MISSING_MATRIX_DEV_ID 31
#define INVALID_MATRIX_DEVICE_ID_LENGTH 32


#define REPLY_ELEMENT_SIZE 100
#define COMMAND_REPLY_BUFFER_SIZE 240

char command_reply_buffer[COMMAND_REPLY_BUFFER_SIZE];

StaticJsonBuffer<1000> jsonBuffer;

typedef uint8_t            bit_t;
typedef uint8_t            u1_t;
typedef int8_t             s1_t;
typedef uint16_t           u2_t;
typedef int16_t            s2_t;
typedef uint32_t           u4_t;
typedef int32_t            s4_t;
typedef unsigned int       uint;
typedef const char* str_t;

struct OptionDecodeItems
{
	const char* optionName;
	const char* optionDescription;
	const char* optionJsonSample;
	void (*optionAction) (JsonObject& root, char* resultBuffer);
};

struct CommandDescription
{
	const char* commandName;
	const char* commandDescription;
	OptionDecodeItems* items;
	int noOfOptions;
	unsigned char* settingsStoreBase;
	int settingsStoreLength;
	void (*resetSettings) ();
	void (*setup)();
	void (*loop)();
	void (*proc1loop)();
};

// declared in settings.h
void save_settings();

//void actOnCommand(const char* command, const char* option, JsonObject& root, char* resultBuffer);

void act_onJson_command(char* json, void(*deliverResult) (char* resultText));

void build_command_reply(int errorNo, JsonObject& root, char* resultBuffer)
{
	char replyBuffer[REPLY_ELEMENT_SIZE];

	const char* sequence = root["seq"];

	if (sequence)
	{
		// Got a sequence number in the command - must return the same number
		// so that the sender can identify the command that was sent
		int sequenceNo = root["seq"];
		sprintf(replyBuffer, "\"error\":%d,\"seq\":%d", errorNo, sequenceNo);
	}
	else
	{
		sprintf(replyBuffer, "\"error\":%d", errorNo);
	}
	strcat(resultBuffer, replyBuffer);
}

void build_text_value_command_reply(int errorNo, char* result, JsonObject& root, char* resultBuffer)
{
	char replyBuffer[REPLY_ELEMENT_SIZE];

	const char* sequence = root["seq"];

	if (sequence)
	{
		// Got a sequence number in the command - must return the same number
		// so that the sender can identify the command that was sent
		int sequenceNo = root["seq"];
		sprintf(replyBuffer, "\"val\":\"%s\",\"error\":%d,\"seq\":%d", result, errorNo, sequenceNo);
	}
	else
	{
		sprintf(replyBuffer, "\"val\":\"%s\",\"error\":%d", result, errorNo);
	}

	strcat(resultBuffer, replyBuffer);
}

void build_number_value_command_reply(int errorNo, int result, JsonObject& root, char* resultBuffer)
{
	char replyBuffer[REPLY_ELEMENT_SIZE];

	const char* sequence = root["seq"];

	if (sequence)
	{
		// Got a sequence number in the command - must return the same number
		// so that the sender can identify the command that was sent
		int sequenceNo = root["seq"];
		sprintf(replyBuffer, "\"val\":%d,\"error\":%d,\"seq\":%d", result, errorNo, sequenceNo);
	}
	else
	{
		sprintf(replyBuffer, "\"val\":%d,\"error\":%d", result, errorNo);
	}

	strcat(resultBuffer, replyBuffer);
}

void dump_hex(u1_t* pos, int length)
{
	while (length > 0)
	{
		// handle leading zeroes
		if (*pos < 0x10) {
			TRACE("0");
		}
		TRACE_HEX(*pos);
		pos++;
		length--;
	}
	TRACELN();
}

char hex_digit(int val)
{
	if (val < 10)
	{
		return '0' + val;
	}
	else
	{
		return 'A' + (val - 10);
	}
}

void dump_hex_string(char* dest, u1_t * pos, int length)
{
	while (length > 0)
	{
		// handle leading zeroes

		*dest = hex_digit(*pos / 16);
		dest++;
		*dest = hex_digit(*pos % 16);
		dest++;
		pos++;
		length--;
	}
	*dest = 0;
}

void dump_unsigned_long(char* dest, u4_t value)
{
	// Write backwards to put least significant values in
	// the right place

	// move to the end of the string
	int pos = 8;
	// put the terminator in position
	dest[pos] = 0;
	pos--;

	while (pos > 0)
	{
		byte b = value & 0xff;
		dest[pos] = hex_digit(b % 16);
		pos--;
		dest[pos] = hex_digit(b / 16);
		pos--;
		value = value >> 8;
	}
}

int decodeStringValue(char* dest, JsonObject& root, const char* valName, int maxLength)
{
	TRACE("Decoding string value: ");
	TRACELN(valName);

	if (!root[valName].is<char*>())
	{
		TRACELN("Value is missing or not a string");
		return STRING_VALUE_MISSING_OR_NOT_STRING;
	}
	else
	{
		String newVal = root[valName];
		if (newVal.length() > maxLength)
		{
			TRACELN("Value is too long");
			return STRING_VALUE_TOO_LONG;
		}
		newVal.toCharArray(dest, maxLength);
		return WORKED_OK;
	}
}

int decodeNumericValue(int* dest, JsonObject& root, const char* valName, int min, int max)
{
	TRACE("Decoding numeric value: ");
	TRACELN(valName);

	if (!root[valName].is<int>())
	{
		TRACELN("Value is missing or not an integer");
		return NUMERIC_VALUE_NOT_AN_INTEGER;
	}
	else
	{
		int newGapVal = root[valName];
		if (newGapVal < min)
		{
			TRACELN("Value below minimum");
			return NUMERIC_VALUE_BELOW_MINIMUM;
		}
		if (newGapVal > max)
		{
			TRACELN("Value above maximum");
			return NUMERIC_VALUE_ABOVE_MAXIMUM;
		}
		*dest = newGapVal;
		return WORKED_OK;
	}
}

int hexFromChar(char c, int* dest)
{
	if (c >= '0' && c <= '9')
	{
		*dest = (int)(c - '0');
		return WORKED_OK;
	}
	else
	{
		if (c >= 'A' && c <= 'F')
		{
			*dest = (int)(c - 'A' + 10);
			return WORKED_OK;
		}
		else
		{
			if (c >= 'a' && c <= 'f')
			{
				*dest = (int)(c - 'a' + 10);
				return WORKED_OK;
			}
		}
	}
	return INVALID_HEX_DIGIT_IN_VALUE;
}

#define MAX_DECODE_BUFFER_LENGTH 20

int decodeHexValueIntoBytes(u1_t * dest, JsonObject & root, const char* valName, int length)
{
	TRACE("Decoding array of bytes value: ");
	TRACELN(valName);

	if (!root[valName].is<char*>())
	{
		TRACELN("Value is missing or not a string");
		return VALUE_MISSING_OR_NOT_A_STRING;
	}

	if (length > MAX_DECODE_BUFFER_LENGTH)
	{
		TRACELN("Incoming hex value will not fit in the buffer");
		return INCOMING_HEX_VALUE_TOO_BIG_FOR_BUFFER;
	}

	String newVal = root[valName];
	// Each hex value is in two bytes - make sure the incoming text is the right length

	if (newVal.length() != length * 2)
	{
		TRACELN("Incoming hex value is the wrong length");
		return INCOMING_HEX_VALUE_IS_THE_WRONG_LENGTH;
	}

	int pos = 0;

	u1_t buffer[MAX_DECODE_BUFFER_LENGTH];
	u1_t* bpos = buffer;

	while (pos < newVal.length())
	{
		int d1, d2, reply;

		reply = hexFromChar(newVal[pos], &d1);
		if (reply != WORKED_OK)
			return reply;
		pos++;
		reply = hexFromChar(newVal[pos], &d2);
		if (reply != WORKED_OK)
			return reply;
		pos++;

		*bpos = (u1_t)(d1 * 16 + d2);
		bpos++;
	}

	// If we get here the buffer has been filled OK

	memcpy_P(dest, buffer, length);
	return WORKED_OK;
}

int decodeHexValueIntoUnsignedLong(u4_t * dest, JsonObject & root, const char* valName)
{
	TRACE("Decoding unsigned long value: ");
	TRACELN(valName);

	if (!root[valName].is<char*>())
	{
		TRACELN("Value is missing or not a string");
		return VALUE_MISSING_OR_NOT_A_STRING;
	}
	// Each hex value is in two bytes - make sure the incoming text is the right length

	String newVal = root[valName];
	if (newVal.length() != 8)
	{
		TRACELN("Incoming hex value is the wrong length");
		return INCOMING_HEX_VALUE_IS_THE_WRONG_LENGTH;
	}

	int pos = 0;

	u4_t result = 0;

	while (pos < newVal.length())
	{
		int d1, d2, reply;

		reply = hexFromChar(newVal[pos], &d1);
		if (reply != WORKED_OK)
			return reply;
		pos++;
		reply = hexFromChar(newVal[pos], &d2);
		if (reply != WORKED_OK)
			return reply;
		pos++;

		u4_t v = d1 * 16 + d2;
		result = result * 256 + v;
	}

	// If we get here the value has been received OK

	*dest = result;
	return WORKED_OK;
}

// declared in device.h
bool isThisDevice(const char* nameToTest);

int checkTargetDeviceName(JsonObject & root)
{
	const char* target = root["t"];

	if (!target)
	{
		TRACELN("Invalid or missing target in received command");
		return INVALID_OR_MISSING_TARGET_IN_RECEIVED_COMMAND;
	}

	if (!isThisDevice(target))
	{
		TRACE("Command for different target: ");
		TRACELN(target);
		return COMMAND_FOR_DIFFERENT_TARGET;
	}

	TRACELN("Doing a command for this device");
	return WORKED_OK;
}

extern struct CommandDescription * commandDescriptionList;
extern int noOfCommands;

void actOnCommand(const char* command, const char* option, JsonObject & root, char* resultBuffer)
{
	bool foundCommand = false;

	for (int i = 0; i < noOfCommands; i++)
	{
		if (strcasecmp(command, commandDescriptionList[i].commandName) == 0)
		{
			TRACE("Performing command: ");
			TRACELN(commandDescriptionList[i].commandName);

			OptionDecodeItems* items = commandDescriptionList[i].items;

			for (int optionNo = 0; optionNo < commandDescriptionList[i].noOfOptions; optionNo++)
			{
				if (strcasecmp(option, items[optionNo].optionName) == 0)
				{
					TRACE("    Performing option: ");
					TRACELN(items[optionNo].optionName);
					items[optionNo].optionAction(root, command_reply_buffer);
					foundCommand = true;
				}
			}
		}
	}

	if (foundCommand)
	{
		TRACELN("Command found and performed.");
	}
	else
	{
		build_command_reply(INVALID_COMMAND_NAME, root, resultBuffer);
		TRACELN("Command not found");
	}
}

void abort_json_command(int error, JsonObject & root, void(*deliverResult) (char* resultText))
{
	build_command_reply(error, root, command_reply_buffer);
	// append the version number to the invalid command message
	strcat(command_reply_buffer, ",\"v\":1}");
	deliverResult(command_reply_buffer);
}

// declared in device.h
bool isThisVersion(int versionToTest);

void act_onJson_command(char* json, void(*deliverResult) (char* resultText))
{
	command_reply_buffer[0] = 0;

	strcat(command_reply_buffer, "{");

	TRACE("Received command:");
	TRACELN(json);

	// Clear any previous elements from the buffer

	jsonBuffer.clear();

	JsonObject& root = jsonBuffer.parseObject(json);

	if (!root.success())
	{
		TRACELN("JSON could not be parsed");
		abort_json_command(JSON_COMMAND_COULD_NOT_BE_PARSED, root, deliverResult);
		return;
	}

	int v = root["v"];

	if (!isThisVersion(v))
	{
		TRACELN("Invalid or missing version in received command");
		TRACELN(v);
		abort_json_command(JSON_COMMAND_MISSING_VERSION, root, deliverResult);
		return;
	}

	const char* command = root["c"];

	if (!command)
	{
		TRACELN("Missing command");
		abort_json_command(JSON_COMMAND_MISSING_COMMAND, root, deliverResult);
		return;
	}

	TRACE("Received command: ");
	TRACELN(command);

	const char* option = root["o"];

	if (!option)
	{
		TRACELN("Missing option");
		TRACELN(v);
		abort_json_command(JSON_COMMAND_MISSING_OPTION, root, deliverResult);
		return;
	}

	TRACE("Received option: ");
	TRACELN(option);

	actOnCommand(command, option, root, command_reply_buffer);
	strcat(command_reply_buffer, "}");
	deliverResult(command_reply_buffer);
}


void serial_deliver_command_result(char* result)
{
	Serial.println(result);
}

#define SERIAL_BUFFER_SIZE 240
#define SERIAL_BUFFER_LIMIT SERIAL_BUFFER_SIZE-1

char serial_receive_buffer[SERIAL_BUFFER_SIZE];

int serial_receive_buffer_pos = 0;

void reset_serial_buffer()
{
	serial_receive_buffer_pos = 0;
}

void act_on_serial_command()
{
	act_onJson_command(serial_receive_buffer, serial_deliver_command_result);
}

void buffer_char(char ch)
{
	if ((serial_receive_buffer_pos > 0) &&
		(ch == '\n' || ch == '\r' || ch == 0))
	{
		// terminate the received string
		serial_receive_buffer[serial_receive_buffer_pos] = 0;
		act_on_serial_command();
		reset_serial_buffer();
	}
	else
	{
		if (serial_receive_buffer_pos < SERIAL_BUFFER_SIZE)
		{
			serial_receive_buffer[serial_receive_buffer_pos] = ch;
			serial_receive_buffer_pos++;
		}
	}
}


#define CHAR_FLUSH_START_TIMEOUT 1000

unsigned long character_timer_start;

void check_serial_buffer()
{
	if (Serial.available())
	{
		character_timer_start = millis();
		while (Serial.available())
		{
			buffer_char(Serial.read());
		}
	}
	else
	{
		if (serial_receive_buffer_pos > 0)
		{
			// have got some characters - if they've been here a while - discard them
			unsigned long elapsed_time = millis() - character_timer_start;
			if (elapsed_time > CHAR_FLUSH_START_TIMEOUT)
			{
				reset_serial_buffer();
			}
		}
	}
}

void list_command_help()
{
	Serial.println("Commmands");

	for (int i = 0; i < noOfCommands; i++)
	{
		Serial.println(commandDescriptionList[i].commandName);

		OptionDecodeItems* items = commandDescriptionList[i].items;

		for (int optionNo = 0; optionNo < commandDescriptionList[i].noOfOptions; optionNo++)
		{
			Serial.printf("    %s\n      %s\n      %s\n", items[optionNo].optionName, items[optionNo].optionDescription, items[optionNo].optionJsonSample);
		}
	}
}

struct CommandSettings
{
	bool commandsEnabled;
} commandSettings;


// {"v":1, "t" : "Sensor01", "c" : "commands", "o" : "debug", "val" : "on"}
// {"v":1, "t" : "Sensor01", "c" : "commands", "o" : "debug", "val" : "off"}
void do_commands_state(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];
		if (!option)
		{
			// no option - just a status request
			if (commandSettings.commandsEnabled)
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
				commandSettings.commandsEnabled = true;
			}
			else
			{
				if (strcasecmp(option, "off") == 0)
				{
					commandSettings.commandsEnabled = false;
				}
				else
				{
					reply = INVALID_COMMANDS_CONTROL_SETTING;
				}
			}
		}
	}

	if (reply == WORKED_OK)
		save_settings();

	build_command_reply(reply, root, resultBuffer);
}

OptionDecodeItems commandOptionDecodeItems[] = {
	{"commands",
		"Enables or disables remote commands",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"commands\",  \"o\" : \"state\", \"val\":\"on\"}",
		do_commands_state}
};



void reset_commands()
{
	commandSettings.commandsEnabled = true;

}

void setup_commands()
{

}

void loop_commands()
{
	check_serial_buffer();
}

void loop_commands_proc1()
{}