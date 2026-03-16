#include <WiFi.h>

String targetSSID = "";
bool tracking = false;

unsigned long lastScanRequest = 0;
const unsigned long SCAN_INTERVAL_MS = 700;

bool scanRunning = false;
int latestRssi = -100;
float filteredRssi = -100.0;
bool rssiInitialized = false;
bool nearMode = false;

String distanceLine = "";

float ultrasonicDistance = 100;
char lastCommand = '\0';

const int RSSI_NEAR_ENTER = -62;
const int RSSI_FAR_ENTER = -68;
const float RSSI_ALPHA = 0.35f;

void requestScan()
{
    if (scanRunning)
    {
        return;
    }

    WiFi.scanDelete();
    WiFi.scanNetworks(true, false);
    scanRunning = true;
    lastScanRequest = millis();
}

void processScanResult()
{
    if (!scanRunning)
    {
        return;
    }

    int scanStatus = WiFi.scanComplete();
    if (scanStatus == WIFI_SCAN_RUNNING || scanStatus == -1)
    {
        return;
    }

    scanRunning = false;
    latestRssi = -100;

    if (scanStatus <= 0)
    {
        return;
    }

    for (int i = 0; i < scanStatus; i++)
    {
        if (WiFi.SSID(i) == targetSSID)
        {
            latestRssi = WiFi.RSSI(i);
            break;
        }
    }

    if (!rssiInitialized)
    {
        filteredRssi = latestRssi;
        rssiInitialized = true;
    }
    else
    {
        filteredRssi = (RSSI_ALPHA * latestRssi) + ((1.0f - RSSI_ALPHA) * filteredRssi);
    }
}

void readUltrasonicDistance()
{
    while (Serial2.available())
    {
        char c = (char)Serial2.read();

        if (c == '\n')
        {
            if (distanceLine.length() > 0)
            {
                ultrasonicDistance = distanceLine.toFloat();
                distanceLine = "";
            }
        }
        else if (c != '\r')
        {
            distanceLine += c;

            if (distanceLine.length() > 16)
            {
                distanceLine = "";
            }
        }
    }
}

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
    Serial.setTimeout(20);

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
    rssiInitialized = false;
    nearMode = false;

    Serial.print("Tracking: ");
    Serial.println(targetSSID);

    requestScan();
}

void loop()
{
    readUltrasonicDistance();

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

        if (millis() - lastScanRequest >= SCAN_INTERVAL_MS)
        {
            scanNetworks();
            Serial.println("Enter network number:");
            lastScanRequest = millis();
        }

        handleUserSelection();
    }

    else
    {
        if (millis() - lastScanRequest >= SCAN_INTERVAL_MS)
        {
            requestScan();
        }

        processScanResult();

        if (!rssiInitialized)
        {
            sendCommandIfChanged('S');
            return;
        }

        Serial.print("RSSI raw: ");
        Serial.print(latestRssi);
        Serial.print(" | RSSI filtered: ");
        Serial.print(filteredRssi, 1);
        Serial.print(" | Ultrasonic: ");
        Serial.println(ultrasonicDistance);

        if (!nearMode && filteredRssi >= RSSI_NEAR_ENTER)
        {
            nearMode = true;
        }
        else if (nearMode && filteredRssi <= RSSI_FAR_ENTER)
        {
            nearMode = false;
        }

        sendCommandIfChanged(nearMode ? 'N' : 'F');
    }
}