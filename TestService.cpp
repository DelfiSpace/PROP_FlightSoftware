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

    if (command.getPayload()[1] != SERVICE_RESPONSE_REQUEST)
    {
        // unknown request
        return false;
    }

    serial.println("TestService: Test Request");

    unsigned char _mr = command.getPayload()[2];

    if (_mr > 2)
    {
        serial.println("Wrong microresistojet id");
        return false;
    }

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
        MicroResistojetHandler::stopActiveMR();
        serial.println("Everything stopped");
        return true;
    }

    const unsigned char *config = (unsigned char *)(&command.getPayload()[3]);
    const uint_fast16_t time_work   = (uint_fast16_t)config[0];
    const uint_fast16_t time_before = (uint_fast16_t)config[1];
    const uint_fast16_t duty_c_heat = (uint_fast16_t)config[2];
    const uint_fast16_t timerPeriod = ((uint_fast16_t)config[3])<<8 | ((uint_fast16_t)config[4]);
    const uint_fast16_t time_hold   = ((uint_fast16_t)config[5])<<8 | ((uint_fast16_t)config[6]);
    const uint_fast16_t time_spike  = ((uint_fast16_t)config[7])<<8 | ((uint_fast16_t)config[8]);
    if (time_work   >= 1 &&
        time_spike  >= 350 &&
        time_spike  <= 3800 &&
        time_hold   >= 1 &&
        time_hold   >= (time_spike/1000) &&
        time_hold   <= timerPeriod &&
        timerPeriod >= ((time_spike+1000-1)/1000) &&
        timerPeriod <  2000 &&
        time_before <  128 &&
        time_work   <  128 &&
        duty_c_heat <= 100)
    {
        mr->startMR(time_work, duty_c_heat, time_before, timerPeriod, time_hold, time_spike);

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
        workingBuffer.getPayload()[1] = operatePropulsion(command) ? SERVICE_RESPONSE_REPLY : SERVICE_RESPONSE_ERROR;
        return true;
    }
    return false;
}


