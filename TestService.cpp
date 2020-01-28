/*
 * TestService.cpp
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#include "TestService.h"
#include "MicroResistojetHandler.h"

extern DSerial serial;
extern MicroResistojetHandler lpm, vlm;

bool TestService::operatePropulsion(DataMessage &command)
{

    if (command.getPayload()[1] != TEST_REQUEST)
    {
        // unknown request
        return false;
    }

    unsigned char _mr = command.getPayload()[2];

    if (_mr > 2)
    {
        return false;
    }

    serial.println("TestService: Test Request");

    MicroResistojetHandler *mr;
    if (_mr == 1)
    {
        serial.print("LPM - ");
        mr = &lpm;
    }
    else if (_mr == 2)
    {
        serial.print("VLM - ");
        mr = &vlm;
        serial.println("Not working");
        return true;
    }
    else
    {
        int ret = MicroResistojetHandler::stopActiveMR(&lpm);
        if (ret == 1) serial.println("LPM - Stopped");
        else if (ret == 0) serial.println("VLM - Stopped");
        else serial.println("No action");
        return true;
    }

    const unsigned char *config = (unsigned char *)(&command.getPayload()[3]);
    const uint_fast16_t time_work   = ((uint_fast16_t)config[0])<<8 | ((uint_fast16_t)config[1]);
    const uint_fast16_t time_before = ((uint_fast16_t)config[2])<<8 | ((uint_fast16_t)config[3]);
    const uint_fast16_t timerPeriod = ((uint_fast16_t)config[4])<<8 | ((uint_fast16_t)config[5]);
    const uint_fast16_t time_hold   = ((uint_fast16_t)config[6])<<8 | ((uint_fast16_t)config[7]);
    const uint_fast16_t time_spike  = ((uint_fast16_t)config[8])<<8 | ((uint_fast16_t)config[9]);
    if (time_work   >= 1 &&
        time_spike  >= 350 &&
        time_spike  <= 3800 &&
        time_hold   >= 1 &&
        time_hold   >= (time_spike/1000) &&
        time_hold   <= timerPeriod &&
        timerPeriod >= ((time_spike+1000-1)/1000) &&
        timerPeriod <  2000 &&
        time_before <= 1000)
    {
        mr->startMR(time_work, time_before, timerPeriod, time_hold, time_spike);

        serial.print("Started with timer period ");
        serial.print(timerPeriod, DEC);
        serial.print(" ms, hold time ");
        serial.print(time_hold, DEC);
        serial.print(" ms and spike time ");
        serial.print(time_spike/1000, DEC);
        serial.print(".");
        serial.print((time_spike/100)%10, DEC);
        serial.print((time_spike/10)%10, DEC);
        serial.print(time_spike%10, DEC);
        serial.println(" ms");
    }
    else
    {
        serial.println("Wrong inputs");
        return false;
    }

    return true;
}

bool TestService::process(DataMessage &command, DataMessage &workingBuffer)
{
    if (command.getPayload()[0] == 0)
    {
        workingBuffer.setSize(2);
        workingBuffer.getPayload()[0] = TEST_SERVICE;
        workingBuffer.getPayload()[1] = operatePropulsion(command) ? TEST_RESPONSE : TEST_ERROR;
        return true;
    }
    return false;
}


