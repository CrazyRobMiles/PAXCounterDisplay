
#include <PxMatrix.h>
#include <string.h>
#include <stdio.h>

#ifdef ESP32

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif
// Pins for LED MATRIX

#define matrix_width 32
#define matrix_height 32

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time = 10; //10-50 is usually fine

// PxMATRIX display(matrix_width,matrix_height,P_LAT, P_OE,P_A,P_B,P_C);
//PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
PxMATRIX display(matrix_width, matrix_height, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

uint16_t myCOLORS[8] = {myRED, myGREEN, myBLUE, myWHITE, myYELLOW, myCYAN, myMAGENTA, myBLACK};

#ifdef ESP8266
// ISR for display refresh
void display_updater()
{
    display.display(display_draw_time);
}
#endif

#ifdef ESP32
void IRAM_ATTR display_updater()
{
    // Increment the counter and set the time of ISR
    portENTER_CRITICAL_ISR(&timerMux);
    display.display(display_draw_time);
    portEXIT_CRITICAL_ISR(&timerMux);
}
#endif

#define PAX_DISPLAY_MESSAGE_LENGTH 3
#define PAX_DEVICE_ID_LENGTH 120

struct PaxDisplaySettings
{
    int count;
    char message[PAX_DISPLAY_MESSAGE_LENGTH + 1];
    char deviceID[PAX_DEVICE_ID_LENGTH + 1];

} paxDisplaySettings;

int matrixDisplayCounter = 0;
int paxCountValue;

int matrixDisplayUpdateIntervalMillis = 100;
unsigned long matrixDisplayLastUpdateTime;

char paxBackBuffer[PAX_DISPLAY_MESSAGE_LENGTH + 1];
char digitsBackBuffer[PAX_DISPLAY_MESSAGE_LENGTH + 1];

boolean forceTextRedraw;
uint16_t digitColourFore;
boolean forceDigitRedraw;

void drawScrunchedText(int x, int y, uint16_t fore, uint16_t back, char *text, char *backbuffer, bool *forceRedraw)
{
    for (int i = 0; i < strlen(text); i++)
    {
        char ch = text[i];
        if (ch != backbuffer[i] || *forceRedraw)
        {
            // need to force a redraw of the rest of the row so that
            // overwritten edges can be redrawn
            *forceRedraw = true;
            display.drawChar(x, y, text[i], fore, back, 2);
            backbuffer[i] = ch;
        }
        x += 11;
    }
    *forceRedraw = false;
}

void setDigitColour(uint16_t col)
{
    if (digitColourFore == col)
        return;

    digitColourFore = col;
    forceDigitRedraw = true;
}

void drawPaxDisplay()
{
    char buffer[PAX_DISPLAY_MESSAGE_LENGTH + 1];

    drawScrunchedText(0, 1, myCYAN, myBLACK, paxDisplaySettings.message, paxBackBuffer, &forceTextRedraw);

    snprintf(buffer, PAX_DISPLAY_MESSAGE_LENGTH + 1, "%3d", paxCountValue);

    drawScrunchedText(0, 17, digitColourFore, myBLACK, buffer, digitsBackBuffer, &forceDigitRedraw);
}

bool displayIsEnabled = false;

void display_update_enable(bool is_enable)
{
    if (is_enable == displayIsEnabled)
        return;

    displayIsEnabled = is_enable;

#ifdef ESP8266
    if (is_enable)
        display_ticker.attach(0.002, display_updater);
    else
        display_ticker.detach();
#endif

#ifdef ESP32
    if (is_enable)
    {
        timer = timerBegin(0, 80, true);
        timerAttachInterrupt(timer, &display_updater, true);
        timerAlarmWrite(timer, 2000, true);
        timerAlarmEnable(timer);
    }
    else
    {
        timerDetachInterrupt(timer);
        timerAlarmDisable(timer);
    }
#endif
}

// {"v":1, "t" : "Sensor01", "c" : "ledmatrix", "o" : "text", "val" : "hello"}
void do_pax_text(JsonObject &root, char *resultBuffer)
{
    int reply = checkTargetDeviceName(root);

    if (reply == WORKED_OK)
    {
        const char *option = root["val"];
        if (!option)
        {
            // no option - just a status request
            build_text_value_command_reply(WORKED_OK, paxDisplaySettings.message, root, resultBuffer);
            return;
        }

        if (!root["val"].is<char *>())
        {
            TRACELN("Value is missing or not a string");
            reply = MISSING_MATRIX_MESSAGE;
        }
        else
        {
            const char *option = root["val"];
            if (strlen(option) > PAX_DISPLAY_MESSAGE_LENGTH)
            {
                reply = INVALID_MATRIX_MESSAGE_LENGTH;
            }
            else
            {
                strcpy(paxDisplaySettings.message, option);
            }
        }
    }

    if (reply == WORKED_OK)
        save_settings();

    build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "Sensor01", "c" : "ledmatrix", "o" : "devid", "val" : "rob-sensor"}
void do_pax_device_id(JsonObject &root, char *resultBuffer)
{
    int reply = checkTargetDeviceName(root);

    if (reply == WORKED_OK)
    {
        const char *option = root["val"];
        if (!option)
        {
            // no option - just a status request
            build_text_value_command_reply(WORKED_OK, paxDisplaySettings.message, root, resultBuffer);
            return;
        }

        if (!root["val"].is<char *>())
        {
            TRACELN("Value is missing or not a string");
            reply = MISSING_MATRIX_DEV_ID;
        }
        else
        {
            const char *option = root["val"];
            if (strlen(option) > PAX_DEVICE_ID_LENGTH)
            {
                reply = INVALID_MATRIX_DEVICE_ID_LENGTH;
            }
            else
            {
                strcpy(paxDisplaySettings.deviceID, option);
            }
        }
    }

    if (reply == WORKED_OK)
        save_settings();

    build_command_reply(reply, root, resultBuffer);
}

void do_pax_count(JsonObject &root, char *resultBuffer)
{
    int reply = checkTargetDeviceName(root);

    if (reply == WORKED_OK)
    {
        const char *option = root["val"];

        if (!option)
        {
            build_number_value_command_reply(WORKED_OK, paxDisplaySettings.count, root, resultBuffer);
            return;
        }

        reply = decodeNumericValue(&paxDisplaySettings.count, root, "val", 0, 999);

        if (reply == WORKED_OK)
        {
            save_settings();
        }
    }

    build_command_reply(reply, root, resultBuffer);
}

OptionDecodeItems paxOptionDecodeItems[] = {
    {"text",
     "Sets the text display",
     "{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"pax\",  \"o\" : \"text\", \"val\":\"message\"}",
     do_pax_text},
    {"count",
     "Sets the count value",
     "{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"pax\",  \"o\" : \"count\", \"val\":10}",
     do_pax_count},
     {"devid",
     "Sets the name of the pax counter device to display",
     "{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"pax\",  \"o\" : \"devid\", \"val\":\"rob-sensor\"}",
     do_pax_device_id
     }};


void act_TTN_Message(char* json, void(*deliverResult) (char* resultText))
{
	//TRACE("Received TTN message:");
	
	//TRACELN(json);

	// Clear any previous elements from the buffer

	jsonBuffer.clear();

	JsonObject& root = jsonBuffer.parseObject(json);

	if (!root.success())
	{
		TRACELN("JSON could not be parsed");
		abort_json_command(JSON_COMMAND_COULD_NOT_BE_PARSED, root, deliverResult);
		return;
	}

	const char* appID = root["app_id"];

	if (!appID)
	{
		TRACELN("Missing app_id");
		return;
	}

    if(strcasecmp(appID, "hull-pax-counter") != 0)
    {
		TRACELN("Wrong app");
		return;
    }

	const char* deviceID = root["dev_id"];

	if (!deviceID)
	{
		TRACELN("Missing device ID");
		return;
	}

    if(strcasecmp(deviceID, paxDisplaySettings.deviceID) != 0)
    {
		TRACELN("Wrong device");
		return;
    }

    int ble = root["payload_fields"]["ble"];

    int wifi = root["payload_fields"]["wifi"];

    TRACE("BLE:"); TRACELN(ble);
    TRACE("WiFi:"); TRACELN(wifi);

    paxCountValue = ble+wifi;
}

void reset_pax_display()
{
    paxDisplaySettings.count = 0;
    strcpy(paxDisplaySettings.message, "PAX");
}

void setup_pax_display()
{
    // Define your display layout here, e.g. 1/8 step
    display.begin(16);
    //  display.setDriverChip(FM6126A);

    // Define your scan pattern here {LINE, ZIGZAG, ZAGGIZ, WZAGZIG, VZAG} (default is LINE)
    //display.setScanPattern(LINE);

    // Define multiplex implemention here {BINARY, STRAIGHT} (default is BINARY)
    //display.setMuxPattern(STRAIGHT);

    digitColourFore = myRED;

    forceDigitRedraw = true;
    forceTextRedraw = true;

    paxCountValue = paxDisplaySettings.count;

    display.setFastUpdate(true);
}

void loop_pax_display()
{
    drawPaxDisplay();
}

void loop_pax_display_proc1()
{
    display.display(display_draw_time);
}
