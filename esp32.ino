#include <WiFi.h>

String targetSSID = "";
bool tracking = false;

unsigned long lastScan = 0;
const unsigned long SCAN_INTERVAL_MS = 3000;

float ultrasonicDistance = 100;
char lastCommand = '\0';

void sendCommandIfChanged(char cmd)
{
    if (cmd != lastCommand)
    {
        Serial2.write(cmd);
        lastCommand = cmd;
    }
}

void setup()
{

    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.println("WiFi Tracker Ready");
    Serial.println("Type an index number to track a network.");
}

int scanNetworks()
{

    int n = WiFi.scanNetworks();

    Serial.println("\nAvailable WiFi:");

    for (int i = 0; i < n; i++)
    {

        Serial.print(i);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" RSSI: ");
        Serial.println(WiFi.RSSI(i));
    }

    if (n == 0)
    {
        Serial.println("No networks found.");
    }

    return n;
}

void handleUserSelection()
{
    if (!Serial.available())
    {
        return;
    }

    int choice = Serial.parseInt();

    if (choice < 0)
    {
        Serial.println("Invalid selection.");
        return;
    }

    int n = WiFi.scanNetworks();
    if (choice >= n)
    {
        Serial.println("Selection out of range. Scan again and choose a valid index.");
        return;
    }

    targetSSID = WiFi.SSID(choice);
    tracking = true;

    Serial.print("Tracking: ");
    Serial.println(targetSSID);
}

void loop()
{

    // receive ultrasonic data
    if (Serial2.available())
    {
        String distanceStr = Serial2.readStringUntil('\n');
        distanceStr.trim();
        if (distanceStr.length() > 0)
        {
            ultrasonicDistance = distanceStr.toFloat();
        }
    }

    // ultrasonic override
    if (ultrasonicDistance < 5)
    {

        Serial.println("ULTRASONIC ALERT");
        sendCommandIfChanged('A');
        return;
    }

    if (!tracking)
    {
        sendCommandIfChanged('S');

        if (millis() - lastScan >= SCAN_INTERVAL_MS)
        {
            scanNetworks();
            Serial.println("Enter network number:");
            lastScan = millis();
        }

        handleUserSelection();
    }

    else
    {

        int n = WiFi.scanNetworks();
        bool foundTarget = false;
        int targetRssi = -100;

        for (int i = 0; i < n; i++)
        {

            if (WiFi.SSID(i) == targetSSID)
            {
                foundTarget = true;
                targetRssi = WiFi.RSSI(i);
                break;
            }
        }

        if (!foundTarget)
        {
            Serial.println("Target network not found. Buzzer stopped.");
            sendCommandIfChanged('S');
            return;
        }

        Serial.print("RSSI: ");
        Serial.print(targetRssi);
        Serial.print(" | Ultrasonic: ");
        Serial.println(ultrasonicDistance);

        if (targetRssi > -60)
        {
            sendCommandIfChanged('N');
        }
        else
        {
            sendCommandIfChanged('F');
        }

        delay(400);
    }
}