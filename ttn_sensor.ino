#include <TTN_esp32.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "TTN_CayenneLPP.h"

// Define device EUI, application EUI, and application key
const char* devEui = "70B3D57ED00651B9"; // Change to TTN Device EUI
const char* appEui = "0000000000000000"; // Change to TTN Application EUI
const char* appKey = "E641FBD79972F3868986453857823E61"; // Change to TTN Application Key

// Define battery monitoring pin
const uint8_t vbatPin = 35; // Battery pin

TTN_esp32 ttn;
TTN_CayenneLPP lpp;

Adafruit_BME280 bme;

float VBAT; // Battery voltage from ESP32 ADC read

// Rest of your code...


void message(const uint8_t* payload, size_t size, uint8_t port, int rssi)
{
    Serial.println("-- MESSAGE");
    Serial.printf("Received %d bytes on port %d (RSSI=%ddB) :", size, port, rssi);
    for (int i = 0; i < size; i++)
    {
        Serial.printf(" %02X", payload[i]);
    }
    Serial.println();
}

void print_wakeup_reason()
{
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason)
    {
    case 1:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case 2:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case 3:
        Serial.println("Wakeup caused by timer");
        break;
    case 4:
        Serial.println("Wakeup caused by touchpad");
        break;
    case 5:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.println("Wakeup was not caused by deep sleep");
        break;
    }
}

void waitForTransactions()
{
    Serial.println("Waiting for pending transactions... ");
    Serial.println("Waiting took " + String(ttn.waitForPendingTransactions()) + "ms");
}

void sendData()
{
    float temperature = bme.readTemperature(); // Read temperature from BME280 sensor
    float humidity = bme.readHumidity();       // Read humidity from BME280 sensor
    float pressure = bme.readPressure() / 100.0; // Read pressure from BME280 sensor and convert to hPa

    // Battery Voltage
    VBAT = ((float)analogRead(vbatPin) / 4095.0) * 2 * 3.95 * (1 / 1.1);

    lpp.reset();
    lpp.addTemperature(1, temperature);
    lpp.addRelativeHumidity(2, humidity);
    lpp.addBarometricPressure(3, pressure);
    lpp.addAnalogInput(4, VBAT); // Add battery voltage to CayenneLPP payload

    if (ttn.sendBytes(lpp.getBuffer(), lpp.getSize()))
    {
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print("°C, Humidity: ");
        Serial.print(humidity);
        Serial.print("%, Pressure: ");
        Serial.print(pressure);
        Serial.println(" hPa");

        Serial.print("Battery Voltage: ");
        Serial.print(VBAT);
        Serial.println("V");

        Serial.print("TTN_CayenneLPP: ");
        for (int i = 0; i < lpp.getSize(); i++)
        {
            Serial.print(lpp.getBuffer()[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting");

    if (!bme.begin(0x76))
    {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

    // Print the wakeup reason for ESP32
    print_wakeup_reason();

    ttn.begin();
    ttn.onMessage(message);
    ttn.join(devEui, appEui, appKey);
    Serial.print("Joining TTN ");
    while (!ttn.isJoined())
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\njoined!");

    waitForTransactions();
    sendData();
    waitForTransactions();

    esp_sleep_enable_timer_wakeup(600 * 1000000);
    Serial.println("Going to sleep!");
    esp_deep_sleep_start();
}

void loop()
{
    // Empty loop as it's never called
}
