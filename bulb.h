#ifndef BULB_H
#define BULB_H
#include <sys/time.h>
#include <mraa.h>

#define ST_START 0
#define ST_SYNCIN 1
#define ST_STOP 2
#define ST_SYNCOUT 3
#define ST_END 4
#define ST_ERROR 5

//#define SHUTTER_PIN 15
//#define FOCUS_PIN 44
//#define SYNC_PIN 14
#define SHUTTER_PIN 33
#define FOCUS_PIN 47
#define SYNC_PIN 36
#define SYNC_PIN2 48

#define USE_MMAP 1

#define TIMEOUT_US 500000

#define ERROR_FAILED_TO_INIT 1
#define ERROR_INVALID_PC_STATE 2
#define ERROR_PC_TIMEOUT 3
#define ERROR_STATE_SEQUENCE 4

typedef struct {
    int32_t bulbMicroSeconds;
    int32_t preFocusMs;
    int32_t endLagMicroSeconds;
    int32_t startLagMicroSeconds;
    uint8_t expectSync;
    uint8_t runTest;
} bulb_config_t;

typedef struct {
    int64_t startDiff;
    int64_t stopDiff;
    int64_t actualTime;
    float errPercent;
} bulb_result_t;

uint8_t bulb(bulb_config_t config, bulb_result_t *result);
#endif