#pragma once

#include <EEPROM.h>
#include "ArduinoJson-v5.13.2.h"

#define EEPROM_SIZE 2000
#define SETTINGS_EEPROM_OFFSET 0

int updateChecksum(int checksum, unsigned char* base, int length)
{
	for (int i = 0; i < length; i++)
	{
		checksum += *base;
		base++;
	}
	return checksum;
}

#define CHECK_1 0xAB
#define CHECK_2 0x55

void display_update_enable(bool is_enable) ;

void save_settings()
{
	int checksum = 0;

	EEPROM.writeByte(SETTINGS_EEPROM_OFFSET, CHECK_1);
	EEPROM.writeByte(SETTINGS_EEPROM_OFFSET+1, CHECK_2);

	int eepromPos = SETTINGS_EEPROM_OFFSET + 2 ;

	for (int i = 0; i < noOfCommands; i++)
	{
		TRACE("Saving: ");
		TRACE(commandDescriptionList[i].commandName);
		TRACE(" Size: ");
		TRACELN(commandDescriptionList[i].settingsStoreLength);
		EEPROM.writeInt(eepromPos, commandDescriptionList[i].settingsStoreLength);
		eepromPos += sizeof(int);
		EEPROM.writeBytes(eepromPos, commandDescriptionList[i].settingsStoreBase, commandDescriptionList[i].settingsStoreLength);
		checksum = updateChecksum(checksum, commandDescriptionList[i].settingsStoreBase, commandDescriptionList[i].settingsStoreLength);
		eepromPos += commandDescriptionList[i].settingsStoreLength;
	}

	EEPROM.writeInt(eepromPos, checksum);

	EEPROM.commit();
}

bool load_settings()
{
	TRACELN("Loading settings");

	if (EEPROM.readByte(SETTINGS_EEPROM_OFFSET) != CHECK_1)
		return false;

	if (EEPROM.readByte(SETTINGS_EEPROM_OFFSET+1) != CHECK_2)
		return false;

	int checksum = 0;

	int eepromPos = SETTINGS_EEPROM_OFFSET + 2 ;

	for (int i = 0; i < noOfCommands; i++)
	{
		TRACE("Loading: ");
		TRACE(commandDescriptionList[i].commandName);
		int size = EEPROM.readInt(eepromPos);
		TRACE(" Size: ");
		TRACELN(size);
		eepromPos += sizeof(int);
		EEPROM.readBytes(eepromPos, commandDescriptionList[i].settingsStoreBase, size);
		checksum = updateChecksum(checksum, commandDescriptionList[i].settingsStoreBase, commandDescriptionList[i].settingsStoreLength);
		eepromPos += commandDescriptionList[i].settingsStoreLength;
	}

	if (checksum != EEPROM.readInt(eepromPos))
		return false;
	else
		return true;
}

void reset_all_settings()
{
	for (int i = 0; i < noOfCommands; i++)
	{
		Serial.printf("Setting up %s\n", commandDescriptionList[i].commandName);
		commandDescriptionList[i].resetSettings();
	}
}


void setup_settings()
{

	if (!EEPROM.begin(EEPROM_SIZE))
	{
		Serial.println("EEPROM faulty");
		while (1) delay(1);
	}

	if (!load_settings())
	{
		Serial.println("Stored settings reset");
		reset_all_settings();
		save_settings();
	}
	else
	{
		Serial.println("Settings loaded OK");
	}
}
