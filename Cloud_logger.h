#pragma once
#include <stdint.h>
#include <time.h>

struct CloudSample {
    time_t timeStamp;
    int32_t powerSaldo;
    float watts;
    float dcVoltage; //Batteriespannung
    float invTemp; // temp inverter
    float yield_day; // yield day
};

namespace CloudLogger {
    void begin();
    void addSample(const CloudSample& s);
}
