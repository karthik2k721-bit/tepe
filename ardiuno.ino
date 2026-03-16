#define TRIG 9
#define ECHO 10
#define BUZZER 8

enum BuzzerMode
{
    BUZZER_OFF,
    BUZZER_TRACK_NEAR,
    BUZZER_TRACK_FAR,
    BUZZER_ALERT
};

BuzzerMode buzzerMode = BUZZER_OFF;

unsigned long lastDistanceSend = 0;

unsigned long lastBurstStart = 0;
unsigned long lastBurstToggle = 0;
bool burstActive = false;
bool burstOutputHigh = false;
int burstStep = 0;

const unsigned long DISTANCE_SEND_MS = 100;
const unsigned long BURST_TOGGLE_MS = 120;
const int BURST_STEPS = 6; // 3 ON + 3 OFF toggles = beep beep beep
const unsigned long NEAR_BURST_INTERVAL_MS = 2000;
const unsigned long FAR_BURST_INTERVAL_MS = 5000;

float getDistance()
{

    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH, 30000);

    if (duration == 0)
    {
        return 400.0;
    }

    return duration * 0.034 / 2;
}

void resetBurst()
{
    burstActive = false;
    burstOutputHigh = false;
    burstStep = 0;
    digitalWrite(BUZZER, LOW);
}

void handleSerialCommands()
{
    while (Serial.available())
    {
        char cmd = Serial.read();

        if (cmd == 'A')
        {
            buzzerMode = BUZZER_ALERT;
            resetBurst();
        }
        else if (cmd == 'N')
        {
            buzzerMode = BUZZER_TRACK_NEAR;
            resetBurst();
        }
        else if (cmd == 'F')
        {
            buzzerMode = BUZZER_TRACK_FAR;
            resetBurst();
        }
        else if (cmd == 'S' || cmd == 'C')
        {
            buzzerMode = BUZZER_OFF;
            resetBurst();
        }
    }
}

void updateBuzzer()
{
    if (buzzerMode == BUZZER_ALERT)
    {
        digitalWrite(BUZZER, HIGH);
        return;
    }

    if (buzzerMode == BUZZER_OFF)
    {
        digitalWrite(BUZZER, LOW);
        return;
    }

    unsigned long now = millis();
    unsigned long interval = (buzzerMode == BUZZER_TRACK_NEAR) ? NEAR_BURST_INTERVAL_MS : FAR_BURST_INTERVAL_MS;

    if (!burstActive)
    {
        if (now - lastBurstStart >= interval)
        {
            burstActive = true;
            burstStep = 0;
            lastBurstToggle = now;
            burstOutputHigh = true;
            digitalWrite(BUZZER, HIGH);
        }
        return;
    }

    if (now - lastBurstToggle >= BURST_TOGGLE_MS)
    {
        lastBurstToggle = now;
        burstStep++;

        if (burstStep >= BURST_STEPS)
        {
            digitalWrite(BUZZER, LOW);
            burstActive = false;
            burstOutputHigh = false;
            lastBurstStart = now;
            return;
        }

        burstOutputHigh = !burstOutputHigh;
        digitalWrite(BUZZER, burstOutputHigh ? HIGH : LOW);
    }
}

void setup()
{

    Serial.begin(9600);

    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);
}

void loop()
{
    unsigned long now = millis();

    if (now - lastDistanceSend >= DISTANCE_SEND_MS)
    {
        float distance = getDistance();
        Serial.println(distance);
        lastDistanceSend = now;
    }

    handleSerialCommands();
    updateBuzzer();
}