#pragma once

void start_action(char * heading, char * message)
{
	Serial.printf("%s: %s\n", heading, message);
}

void update_action(char * heading, char * message)
{
	Serial.printf("    %s: %s\n", heading, message);
}

void end_action()
{
	Serial.println("================");
}
