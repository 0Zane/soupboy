#include "include/nrf_jammer.h"
#include "include/pins.h"
#include <RF24.h>
#include <SPI.h>

// Pin definitions (fallback if not in pins.h)
#ifndef PIN_NRF_CE
#define PIN_NRF_CE   16
#endif
#ifndef PIN_NRF_CSN
#define PIN_NRF_CSN  15
#endif
#ifndef PIN_NRF_SCK
#define PIN_NRF_SCK  14
#endif
#ifndef PIN_NRF_MOSI
#define PIN_NRF_MOSI 13
#endif
#ifndef PIN_NRF_MISO
#define PIN_NRF_MISO 12
#endif

namespace {
    RF24* radio = nullptr;
    SPIClass* spi = nullptr;
    JammerStatus status = { false, false, JAM_OFF, 0, 0, "Not initialized" };

    // BLE 21 channels (first 21 of 40)
    const uint8_t bleChannels21[] = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20
    };
    // WiFi channels (nRF24 channel = freq - 2400)
    const uint8_t wifiChannels[] = {
        12,17,22,27,32,37,42,47,52,57,62,67,72,84
    };
    const uint8_t zigbeeChannels[] = {
        11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26
    };
    const uint8_t droneChannels[] = {
        20,25,30,35,40,45,50,55,60,65,70,75,80,85
    };

    unsigned long lastHopTime = 0;
    int hopDelayMs = 3;
    uint8_t channelIndex = 0;
    uint8_t currentChannel = 0;
    uint8_t jammingPayload[32];
    uint32_t payloadSeed = 0xDEADBEEF;
}

void jammerBegin() {
    if (PIN_NRF_CE < 0 || PIN_NRF_CSN < 0) {
        status.message = "Pins not configured";
        return;
    }

    spi = new SPIClass(VSPI);
    spi->begin(PIN_NRF_SCK, PIN_NRF_MISO, PIN_NRF_MOSI, PIN_NRF_CSN);
    spi->setFrequency(16000000);
    spi->setBitOrder(MSBFIRST);
    spi->setDataMode(SPI_MODE0);

    radio = new RF24(PIN_NRF_CE, PIN_NRF_CSN);
    if (!radio->begin(spi)) {
        status.message = "nRF24 not detected";
        status.initialized = false;
        return;
    }

    // Configure for maximum jamming effectiveness
    radio->setPALevel(RF24_PA_MAX, true);
    radio->setDataRate(RF24_2MBPS);
    radio->setCRCLength(RF24_CRC_DISABLED);
    radio->disableCRC();
    radio->setAutoAck(false);
    radio->setRetries(0, 0);
    radio->setPayloadSize(32);
    radio->setAddressWidth(3);
    radio->disableAckPayload();
    radio->disableDynamicPayloads();
    radio->stopListening();
    radio->powerUp();

    // Pre‑generate payload
    for (int i = 0; i < 32; i++) {
        payloadSeed = payloadSeed * 1103515245 + 12345;
        jammingPayload[i] = (payloadSeed >> 16) & 0xFF;
    }

    status.initialized = true;
    status.running = false;
    status.message = "Ready";
    status.packetsSent = 0;
    Serial.println("✅ nRF24 Jammer initialized");
}

static uint8_t getNextChannel() {
    uint8_t ch = 0;
    switch(status.mode) {
        case JAM_BLE_21:
            ch = bleChannels21[channelIndex % 21];
            channelIndex++;
            break;
        case JAM_BLE_80:
            ch = channelIndex % 80;
            channelIndex++;
            break;
        case JAM_WIFI:
            ch = wifiChannels[channelIndex % 14];
            channelIndex++;
            break;
        case JAM_ZIGBEE:
            ch = zigbeeChannels[channelIndex % 16];
            channelIndex++;
            break;
        case JAM_DRONE:
            ch = droneChannels[channelIndex % 14];
            channelIndex++;
            break;
        default:
            ch = 0;
            break;
    }
    return ch;
}

void jammerStart(JamMode mode) {
    if (!status.initialized) {
        jammerBegin();
        if (!status.initialized) return;
    }
    status.mode = mode;
    status.running = true;
    status.packetsSent = 0;
    channelIndex = 0;
    lastHopTime = millis();
    Serial.printf("🔴 Jamming started: %s\n", jamModeName(mode));
}

void jammerStop() {
    status.running = false;
    if (radio) {
        radio->powerDown();
        radio->powerUp(); // ready for next start
    }
    Serial.println("⏹️ Jamming stopped");
}

void jammerToggle() {
    if (status.running) jammerStop();
    else jammerStart(status.mode == JAM_OFF ? JAM_BLE_21 : status.mode);
}

void jammerUpdate() {
    if (!status.running || !status.initialized) return;

    unsigned long now = millis();
    if (now - lastHopTime < hopDelayMs) return;
    lastHopTime = now;

    uint8_t channel = getNextChannel();
    currentChannel = channel;

    radio->setChannel(channel);

    // Fresh random payload per packet
    for (int i = 0; i < 32; i++) {
        payloadSeed = payloadSeed * 1103515245 + 12345;
        jammingPayload[i] = (payloadSeed >> 16) & 0xFF;
    }

    radio->write(jammingPayload, 32);
    status.packetsSent++;
}

JammerStatus jammerStatus() {
    status.channel = currentChannel;
    return status;
}

const char* jamModeName(JamMode mode) {
    switch(mode) {
        case JAM_BLE_21:  return "BLE 21ch";
        case JAM_BLE_80:  return "BLE 80ch";
        case JAM_WIFI:    return "WiFi";
        case JAM_ZIGBEE:  return "Zigbee";
        case JAM_DRONE:   return "Drone";
        case JAM_OFF:     return "OFF";
        default:          return "Unknown";
    }
}