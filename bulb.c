#include "bulb.h"

mraa_gpio_context shutterOut;
mraa_gpio_context focusOut;
mraa_gpio_context syncIn;

uint8_t _bulb_init()
{
    mraa_init();
    mraa_set_log_level(7);
    mraa_set_priority(99);

    shutterOut = mraa_gpio_init(SHUTTER_PIN);
    mraa_gpio_owner(shutterOut, 1);
    if(!shutterOut)
    {
        return 1;
    }
    mraa_gpio_use_mmaped(shutterOut, USE_MMAP);
    mraa_gpio_dir(shutterOut, MRAA_GPIO_OUT_LOW);
    mraa_gpio_mode(shutterOut, MRAA_GPIO_STRONG);

    focusOut = mraa_gpio_init(FOCUS_PIN);
    mraa_gpio_owner(focusOut, 1);
    if(!focusOut)
    {
        return 1;
    }
    mraa_gpio_use_mmaped(focusOut, USE_MMAP);
    mraa_gpio_dir(focusOut, MRAA_GPIO_OUT_LOW);
    mraa_gpio_mode(focusOut, MRAA_GPIO_STRONG);

    syncIn = mraa_gpio_init(SYNC_PIN);
    mraa_gpio_owner(syncIn, 1);
    if(!syncIn)
    {
        return 1;
    }
    mraa_gpio_use_mmaped(syncIn, USE_MMAP);
    mraa_gpio_dir(syncIn, MRAA_GPIO_IN);

    return 0;
}

uint8_t _bulb_cleanup(uint8_t passthrough_error)
{
    if(shutterOut) mraa_gpio_write(shutterOut, 0);
    if(focusOut) mraa_gpio_write(focusOut, 0);
    usleep(3000);

    if(shutterOut) mraa_gpio_close(shutterOut);
    if(focusOut) mraa_gpio_close(focusOut);
    if(syncIn) mraa_gpio_close(syncIn);

    //mraa_deinit();

    return passthrough_error;
}

int64_t _microSecondDiff(struct timeval *t1, struct timeval *t0)
{
    if(t1 && t0)
    {
        int64_t seconds = t1->tv_sec - t0->tv_sec;
        int64_t micros = t1->tv_usec - t0->tv_usec;
        return micros + (seconds * 1000000);
    }
    else
    {
        return 0;
    }
}


uint8_t bulb(bulb_config_t config, bulb_result_t *result)
{
    struct timeval startTime, syncInTime, stopTime, syncOutTime, now;
    uint8_t state = ST_START;

    if(config.runTest)
    {
        config.runTest = 1;
        config.bulbMicroSeconds = 0;
        config.startLagMicroSeconds = 0;
        config.endLagMicroSeconds = 0;
        config.expectSync = 1;
    }

    if(_bulb_init())
    {
        return _bulb_cleanup(ERROR_FAILED_TO_INIT);
    }

    if(!mraa_gpio_read(syncIn))
    {
        return _bulb_cleanup(ERROR_INVALID_PC_STATE);
    }

    mraa_gpio_write(focusOut, 1);

    usleep(config.preFocusMs * 1000);

    gettimeofday(&startTime, NULL);
    mraa_gpio_write(shutterOut, 1);
    state = ST_SYNCIN;

    uint8_t sync, lastSync = 1;
    while(state != ST_ERROR && state != ST_END)
    {
        sync = mraa_gpio_read(syncIn);
        if(sync != lastSync)
        {
            if(state == ST_SYNCIN && sync == 0)
            {
                gettimeofday(&syncInTime, NULL);
                state = ST_STOP;
            }
            else if(state == ST_SYNCOUT && sync == 1)
            {
                gettimeofday(&syncOutTime, NULL);
                state = ST_END;
            }
            else
            {
                state = ST_ERROR;
            }
            lastSync = sync;
        }
        if(state == ST_STOP)
        {
            gettimeofday(&now, NULL);
            if(_microSecondDiff(&now, &syncInTime) >= config.bulbMicroSeconds - config.endLagMicroSeconds)
            {
                gettimeofday(&stopTime, NULL);
                mraa_gpio_write(shutterOut, 0);
                mraa_gpio_write(focusOut, 0);
                state = ST_SYNCOUT;
            }
        }
        else
        {
            gettimeofday(&now, NULL);
            int64_t diff = _microSecondDiff(&now, &startTime);
            if((!config.expectSync && state == ST_SYNCIN && (diff > config.bulbMicroSeconds + config.startLagMicroSeconds - config.endLagMicroSeconds)) || diff > TIMEOUT_US + config.bulbMicroSeconds)
            {
                state = ST_ERROR;
                return _bulb_cleanup(ERROR_PC_TIMEOUT);
            }
        }
    }

    if(state != ST_ERROR)
    {
        result->startDiff = _microSecondDiff(&syncInTime, &startTime);
        result->stopDiff = _microSecondDiff(&syncOutTime, &stopTime);
        result->actualTime = _microSecondDiff(&syncOutTime, &syncInTime);
        if(!config.runTest)
        {
            result->errPercent = (float)(100.0 - (((double)config.bulbMicroSeconds / (double)result->actualTime) * 100.0));
            if(result->errPercent < 0.0) result->errPercent = 0.0 - result->errPercent;
        }
    }
    else
    {
        return _bulb_cleanup(ERROR_STATE_SEQUENCE);
    }

    mraa_gpio_write(shutterOut, 0);
    mraa_gpio_write(focusOut, 0);
    
    return _bulb_cleanup(0);
}


