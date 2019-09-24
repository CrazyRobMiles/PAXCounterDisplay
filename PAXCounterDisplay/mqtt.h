#define MQTT_DISPLAY_TIMEOUT 3000

#define MQTT_RECONNECT_TIMEOUT 20000

#define MQTT_SERVER_NAME_LENGTH 100
#define MQTT_USER_NAME_LENGTH 100
#define MQTT_PASSWORD_LENGTH 200
#define MQTT_DEVICE_NAME_LENGTH 100
#define MQTT_PUBLISH_TOPIC_LENGTH 100
#define MQTT_SUBSCRIBE_TOPIC_LENGTH 100

enum MQTTState { AwaitingWiFi, ConnectingToMQTTServer, ShowingConnectedToMQTTServer, ShowingConnectToMQTTServerFailed, ConnectedToMQTTServer, ConnectToMQTTServerFailed };

MQTTState mqttState;

int mqtt_message_count = 0;
int mqtt_disconnect_count = 0;

struct MqttSettings
{
	char mqttServer[MQTT_SERVER_NAME_LENGTH];
	int mqttPort;
	char mqttUser[MQTT_USER_NAME_LENGTH];
	char mqttPassword[MQTT_PASSWORD_LENGTH];
	char mqttName[MQTT_DEVICE_NAME_LENGTH];
	char mqttPublishTopic[MQTT_PUBLISH_TOPIC_LENGTH];
	char mqttSubscribeTopic[MQTT_SUBSCRIBE_TOPIC_LENGTH];
} mqttSettings;


void reset_MQTT_settings()
{
	strcpy(mqttSettings.mqttServer, "mqtt.connectedhumber.org");
	mqttSettings.mqttPort = 1883;
	strcpy(mqttSettings.mqttUser, "sdfsdf");
	strcpy(mqttSettings.mqttPassword, "sdfsdsdf");
	strcpy(mqttSettings.mqttName, "Sensor01");
	strcpy(mqttSettings.mqttPublishTopic, "sensor01/datazump");
	strcpy(mqttSettings.mqttSubscribeTopic, "sensor01/commands");
}

unsigned long mqtt_timer_start;

Client * espClient = NULL;

PubSubClient * mqttPubSubClient = NULL;

#define MQTT_RECEIVE_BUFFER_SIZE 1000
char mqtt_receive_buffer[MQTT_RECEIVE_BUFFER_SIZE];

#define MQTT_SEND_BUFFER_SIZE 1000

char mqtt_send_buffer[MQTT_SEND_BUFFER_SIZE];

boolean send_buffer_to_mqtt( char * buffer)
{
	TRACELN("Sending to MQTT");
	if (mqttState != ConnectedToMQTTServer)
	{
		TRACELN("MQTT not connected yet");
		return false;
	}

	TRACELN(mqtt_send_buffer);

	if (mqttPubSubClient->publish(mqttSettings.mqttPublishTopic, buffer))
	{
		mqtt_message_count++;
		TRACELN("MQTT sent");
	}
	else {
		TRACELN("MQTT send failed");
	}

	return true;
}

void mqtt_deliver_command_result(char * result)
{
	send_buffer_to_mqtt(result);
}

void act_TTN_Message(char* json, void(*deliverResult) (char* resultText));

void callback(char* topic, byte* payload, unsigned int length)
{
	TRACE("Message arrived in topic: ");
	TRACELN(topic);

	TRACE("Message:");
	int i;
	for (i = 0; i < length; i++) {
		TRACE((char)payload[i]);
		mqtt_receive_buffer[i] = (char)payload[i];
	}

	// Put the terminator on the string

	mqtt_receive_buffer[i] = 0;

	Serial.println("YAY!");
	Serial.println(mqtt_receive_buffer);

	TRACELN();
	TRACELN("-----------------------");

	act_TTN_Message(mqtt_receive_buffer, mqtt_deliver_command_result);
}

void mqtt_connect_failed()
{
	TRACE("MQTT failed with state: ");

	TRACELN(mqttPubSubClient->state());
	mqtt_timer_start = millis();

	sprintf(mqtt_send_buffer,
		"Connect Failed: %d",
		mqttPubSubClient->state());

	update_action(mqttSettings.mqttName, mqtt_send_buffer);
	mqttState = ShowingConnectToMQTTServerFailed;
}

void mqtt_connected()
{
	TRACELN("MQTT connected");
	mqtt_timer_start = millis();
	update_action(mqttSettings.mqttName, "Connected OK");
	mqttPubSubClient->subscribe(mqttSettings.mqttSubscribeTopic);
	mqttState = ShowingConnectedToMQTTServer;
}

void attempt_mqtt_connect()
{
	if (mqttPubSubClient->connect(mqttSettings.mqttName, mqttSettings.mqttUser, mqttSettings.mqttPassword))
	{
		mqtt_connected();
	}
	else
	{
		mqtt_connect_failed();
	}
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "publish", "val":"sensor01/data" }
void do_mqtt_publish_location(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, mqttSettings.mqttPublishTopic, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(mqttSettings.mqttPublishTopic, root, "val", MQTT_PUBLISH_TOPIC_LENGTH - 1);

		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "subscribe", "val":"sensor01/commands" }
void do_mqtt_subscribe_location(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, mqttSettings.mqttSubscribeTopic, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(mqttSettings.mqttSubscribeTopic, root, "val", MQTT_SUBSCRIBE_TOPIC_LENGTH - 1);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "id", "val":"sensor01" }
void do_mqtt_device_id(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, mqttSettings.mqttName, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(mqttSettings.mqttName, root, "val", MQTT_DEVICE_NAME_LENGTH - 1);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "host", "val":"mqtt.connectedhumber.org" }
void do_mqtt_host(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, mqttSettings.mqttServer, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(mqttSettings.mqttServer, root, "val", MQTT_SERVER_NAME_LENGTH - 1);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "user", "val":"username" }
void do_mqtt_user(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, mqttSettings.mqttUser, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(mqttSettings.mqttUser, root, "val", MQTT_USER_NAME_LENGTH - 1);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "pwd", "val":"123456" }
void do_mqtt_password(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_text_value_command_reply(WORKED_OK, mqttSettings.mqttPassword, root, resultBuffer);
			return;
		}

		reply = decodeStringValue(mqttSettings.mqttPassword, root, "val", MQTT_PASSWORD_LENGTH - 1);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

// {"v":1, "t" : "sensor01", "c" : "mqtt", "o" : "port", "val":1883 }
void do_mqtt_port(JsonObject& root, char* resultBuffer)
{
	int reply = checkTargetDeviceName(root);

	if (reply == WORKED_OK)
	{
		const char* option = root["val"];

		if (!option)
		{
			build_number_value_command_reply(WORKED_OK, mqttSettings.mqttPort, root, resultBuffer);
			return;
		}

		reply = decodeNumericValue(&mqttSettings.mqttPort, root, "val", 0, 9999);
		if (reply == WORKED_OK)
		{
			save_settings();
		}
	}

	build_command_reply(reply, root, resultBuffer);
}

OptionDecodeItems mqttOptionDecodeItems[] = {
	{"publish", 
		"Set the publish topic for any data sent from the device to MQTT", 
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"publish\", \"val\":\"sensor01/data\" }",
		do_mqtt_publish_location},
	{"subscribe", 
		"Set the subscribe topic for any commands sent to the device from MQTT",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"subscribe\", \"val\":\"sensor01/commands\" }",
		do_mqtt_subscribe_location},
	{"id", 
		"Set the MQTT device id",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"id\", \"val\":\"sensor01\" }",
		do_mqtt_device_id},
	{"user",
		"Set the username for MQTT login",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"user\", \"val\":\"username\" }",
		do_mqtt_user},
	{"pwd", 
		"Set the password for MQTT login",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"pwd\", \"val\":\"123456\" }",
		do_mqtt_password},
	{"host", 
		"Set the host for MQTT login",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"host\", \"val\":\"mqtt.connectedhumber.org\" }",
		do_mqtt_host},
	{"port", 
		"Set the port on the MQTT server",
		"{\"v\":1, \"t\" : \"sensor01\", \"c\" : \"mqtt\", \"o\" : \"port\", \"val\":1883 }",
		do_mqtt_port}
};


void setup_mqtt()
{
	if (espClient == NULL)
	{
		if(MQTT_MAX_PACKET_SIZE<1000)
			Serial.println("MQTT_MAX_PACKET_SIZE in PubSubClient is too small");

		espClient = new WiFiClient();

		mqttPubSubClient = new PubSubClient(*espClient);
		mqttPubSubClient->setServer(mqttSettings.mqttServer, mqttSettings.mqttPort);
		mqttPubSubClient->setCallback(callback);
	}

	mqttState = AwaitingWiFi;
}

void loop_mqtt()
{
	unsigned long elapsed_time;

	switch (mqttState)
	{
	case AwaitingWiFi:
		if (wifiState == WiFiConnected)
		{
			// Start the MQTT connection running
			start_action(mqttSettings.mqttName, "Connecting to MQTT");
			attempt_mqtt_connect();
		}
		break;

	case ConnectingToMQTTServer:
		attempt_mqtt_connect();
		break;

	case ShowingConnectedToMQTTServer:
		elapsed_time = millis() - mqtt_timer_start;

		if (elapsed_time > MQTT_DISPLAY_TIMEOUT)
		{
			end_action();
			mqttState = ConnectedToMQTTServer;
		}
		mqttPubSubClient->loop();
		break;

	case ShowingConnectToMQTTServerFailed:
		elapsed_time = millis() - mqtt_timer_start;

		if (elapsed_time > MQTT_DISPLAY_TIMEOUT)
		{
			end_action();
			mqtt_timer_start = millis();
			mqttState = ConnectToMQTTServerFailed;
		}
		break;

	case ConnectedToMQTTServer:

		if (wifiState != WiFiConnected)
		{
			mqttState = AwaitingWiFi;
		}

		if (!mqttPubSubClient->loop())
		{
		    setDigitColour(myRED);
			mqttPubSubClient->disconnect();
			mqtt_timer_start = millis();
			mqtt_disconnect_count++;
			mqttState = ConnectToMQTTServerFailed;
		}

		setDigitColour(myGREEN);

		break;

	case ConnectToMQTTServerFailed:

		elapsed_time = millis() - mqtt_timer_start;

		if (elapsed_time > MQTT_RECONNECT_TIMEOUT)
		{
			mqttState = AwaitingWiFi;
		}
		break;
	}
}

void loop_mqtt_proc1()
{
	
}
