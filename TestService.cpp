/*
 * TestService.cpp
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#include "TestService.h"
#include "MicroResistojetHandler.h"

extern DSerial serial;
extern MicroResistojetHandler mr[];
extern const unsigned int num_mr;

bool TestService::operatePropulsion(DataMessage &command)
{

    if (command.getPayload()[1] != SERVICE_RESPONSE_REQUEST)
    {
        // unknown request
        return false;
    }


    serial.println("TestService: Test Request");

    bool ret;

    unsigned char _mr = command.getPayload()[2];

    if (_mr > num_mr)
    {
        serial.println("Wrong microresistojet id");
        ret = false;
    }
    else if (_mr == 0)
    {
        serial.println(
                MicroResistojetHandler::stopActiveMR() ?
                "Active microresistojet stopped" :
                "Nothing to stop");
        ret = true;
    }
    else
    {
        const unsigned char *config = (unsigned char *)(&command.getPayload()[3]);

        struct MicroResistojetHandler::params_t configMR =
        {
            (uint_fast8_t)config[0],
            (uint_fast8_t)config[1],
            (uint_fast8_t)config[2],
            ((uint_fast16_t)config[3])<<8 | ((uint_fast16_t)config[4]),
            ((uint_fast16_t)config[5])<<8 | ((uint_fast16_t)config[6]),
            ((uint_fast16_t)config[7])<<8 | ((uint_fast16_t)config[8])
        };

        MicroResistojetHandler * myMR = &mr[_mr-1];

        ret = myMR->startMR(configMR);

        serial.print(myMR->getName());
        serial.print(" - ");

        if (ret)
        {
            serial.print("Started with timer period ");
            serial.print(configMR.timerPeriod, DEC);
            serial.print(" ms, hold time ");
            serial.print(configMR.time_hold, DEC);
            serial.print(" ms and spike time ");
            serial.print(configMR.time_spike/1000, DEC);
            serial.print(".");
            serial.print((configMR.time_spike/100)%10, DEC);
            serial.print((configMR.time_spike/10)%10, DEC);
            serial.print(configMR.time_spike%10, DEC);
            serial.println(" ms");
        }
        else
        {
            serial.println("Wrong inputs");
        }
    }

    return ret;
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


