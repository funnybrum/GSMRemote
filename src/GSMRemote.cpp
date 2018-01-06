#include <Arduino.h>
#include <TaskScheduler.h>
#include "LowPower.h"
#include "Pins.h"
#include "Secrets.h"
#include "SIM800.h"

// Macro converting minutes to millis.
#define MIN_TO_MILLIS(mins) (mins * 60L * 1000L)

// Keep the external deviced turned on for at most X minutes.
#define MAX_TURN_ON_DURATION MIN_TO_MILLIS(15L)

// Try to turn off/on the SIM800 if the network status is not OK after X minuites.
#define HOLD_ON_BAD_NETWORK_STATUS MIN_TO_MILLIS(20L)

// Power cycle SIM800C if no serial activity for more than X minutes.
#define HOLD_ON_NO_SERIAL_ACTIVITY MIN_TO_MILLIS(10L)

// Each X minutes check to see if there is a time constraint activity, like:
// * if the max running time for the external device has been reached
// * if the max bad network status interval was reached
// * if the max no serial activity interval was reached
// If any of the above has been detected - take action.
#define TIME_CONSTRAINS_CHECK_INTERVAL MIN_TO_MILLIS(1L)

#define NETWORK_STATUS_CHECK_INTERVAL MIN_TO_MILLIS(5L)

// If the battery voltage is >=13V - turn of the external device.
#define VBAT_TURN_OFF_VOLTAGE 13000


void HandleResponse(char*);

SIM800 sim800 = SIM800(SIM800_RX_PIN, SIM800_TX_PIN, SIM800_PW_PIN, HandleResponse);

Scheduler runner;
Task* checkNetworkStatusTask = 0;
Task* checkTimeConstraintsTask = 0;

unsigned long lastGoodNetworkStatus;
unsigned long externalDevicePoweredOnAt;
bool externalDevicePoweredOn = false;

/**
 * Get the battery voltage. Result is in millivolts. The calculations expect the analog reference
 * to be set to 1100mV.
 */
int getBatteryVoltage() {
    long vbat = analogRead(VBAT_PIN);
    vbat *= R1 + R2;
    vbat /= R2;
    vbat *= 1100;
    vbat /= 1023;
    return (int)vbat;
}

void powerOn(bool powerOn) {
    Serial.print(F("Setting external device power state to: "));
    Serial.println(powerOn?"ON":"OFF");
    digitalWrite(RELAY_PIN, powerOn?HIGH:LOW);
    externalDevicePoweredOnAt = millis();
    externalDevicePoweredOn = powerOn;
}

void CheckNetworkStatus() {
    // Weak up
    sim800.sendCommand("AT", 250);

    // Check network association status, 1 is OK, everyghint else is not OK.
    char resp[SIM800_RESPONSE_BUFFER_LENGTH];
    memset(resp, 0, sizeof(resp));
    
    sim800.sendCommandAndReadResponse("AT+CREG?", resp, 1000);
    int networkStatus = -1;

    // Check if we have suffessful response.
    if (memcmp("+CREG: 0,", resp, 9) == 0) {
        networkStatus = atoi(resp + 9);
    }

    if (networkStatus == 1) {
        lastGoodNetworkStatus = millis();
    }

    // Get back to sleep mode.
    bool sleepResult = sim800.sendCommandAndVerifyResponse("AT+CSCLK=2", "OK");
    if (!sleepResult) {
        Serial.println(F("Failed to enter sleep mode!"));
    }
}

void CheckTimeConstraints() {
    unsigned long now = millis();

    unsigned long timeSinceLastGoodNetworkStatus = now - lastGoodNetworkStatus;
    if (timeSinceLastGoodNetworkStatus > HOLD_ON_BAD_NETWORK_STATUS
        || sim800.getMillisSinceLastSerialActivity() > HOLD_ON_NO_SERIAL_ACTIVITY) {
        sim800.togglePowerState();
        checkNetworkStatusTask->disable();
        lastGoodNetworkStatus = now;
    }

    if (externalDevicePoweredOn) {
        unsigned long timeSinceExternalDeviceWasPoweredOn = now - externalDevicePoweredOnAt;
        if (timeSinceExternalDeviceWasPoweredOn > MAX_TURN_ON_DURATION) {
            powerOn(false);
        }

        if (getBatteryVoltage() >= VBAT_TURN_OFF_VOLTAGE) {
            powerOn(false);
        }
    }
}

void HandleResponse(char* response) {
    if (strcmp(response, "RDY") == 0 ||
        strcmp(response, "+CFUN: 1") == 0 ||
        strcmp(response, "+CPIN: READY") == 0 ||
        strcmp(response, "RING") == 0) {
        // Ignore these.
        return;
    }

    if (strcmp(response, "SMS Ready") == 0 ||
        strcmp(response, "Call Ready") == 0) {
        // The SIM800C is up and running. Start the regular network status checks.
        checkNetworkStatusTask->enable();
        return;
    }

    if (memcmp(response, "+CLIP: \"", 8) == 0) {
        // Incomming call.
        char callNumber[32];
        int i = 0;
        while (response[i+8] != 0 && response[i+8] != '"') {
            callNumber[i] = response[i+8];
            i++;
        }
        callNumber[i] = 0;

        if (strcmp(callNumber, AUTHORIZED_NUMBER) == 0) {
            Serial.println("Authorized call!");
            powerOn(!externalDevicePoweredOn);
            // Some times single ATH is not enough to hang up the call and another +CLIP: ... will
            // be received. To avoid this - send the ATH a few times and flush the SIM800C serial
            // interface.
            for (int i = 0; i < 3; i++) {
                sim800.sendCommand("ATH");
                delay(1000);
            }
        }

        return;
    }

    if (strcmp(response, "NORMAL POWER DOWN") == 0) {
        // Hmm. This should not be seen unless the serial interface is connected while SIM800C is
        // running. In that case setup() will power it off and the above if condition will be true.
        sim800.togglePowerState();
        checkNetworkStatusTask->disable();
        return;
    }

    Serial.print(F("Unhandled response: "));
    Serial.println(response);
}

void setup() {
    // Prepare the on/off pin.
    pinMode(SIM800_PW_PIN, OUTPUT);
    digitalWrite(SIM800_PW_PIN, HIGH);

    // Prepare the relay pin.
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // Set the analog reference for VBAT readings. The aref should become 1.1V on Arduino Nano.
    analogReference(INTERNAL);

    Serial.begin(9600);
    
    while(!Serial)
        delay(100);

    sim800.togglePowerState();
    runner.startNow();

    checkNetworkStatusTask = new Task(NETWORK_STATUS_CHECK_INTERVAL, TASK_FOREVER, &CheckNetworkStatus, &runner, false);
    checkTimeConstraintsTask = new Task(TIME_CONSTRAINS_CHECK_INTERVAL, TASK_FOREVER, &CheckTimeConstraints, &runner, true);

    Serial.println(F("Setup completed."));
}

void lowPowerDelay(unsigned long aDelay) {
    unsigned long sleepTo = millis() + aDelay;
    while (millis() < sleepTo) {
        LowPower.idle(SLEEP_15MS, ADC_ON, TIMER2_ON, TIMER1_ON, TIMER0_ON, SPI_OFF, USART0_ON, TWI_OFF);
    }
}

void loop() {
    runner.execute();
    sim800.loop();
    lowPowerDelay(100);

    // For debugging purposes. Any character received on the Arduino serial interface will be
    // forwarded to the SIM800C serial interface. There are a few special charactes:
    // * '<' will toggle the power state of the SIM800C module.
    // * '>' will send CTRL+Z. This is the SMS terminate character.
    // * '~' will execute the CheckNetworkStatus function.
    while (Serial.available()) {
        char ch = Serial.read();
        if (ch == '<') {
            sim800.togglePowerState();
            checkNetworkStatusTask->disable();
        } else if (ch =='>') {
            // Ctrl+z
            sim800.write(0x1A);
        } else if (ch == '~') {
            CheckNetworkStatus();
        } else if (ch == '!') {
            Serial.println(getBatteryVoltage());
        } else {
            sim800.write(ch);
        }
    }
}