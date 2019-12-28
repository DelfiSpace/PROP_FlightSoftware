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

/* Application Defines */
#define TIMER_PERIOD 1000000
#define TIME_SPIKE 500

bool TestService::process(PQ9Frame &command, PQ9Bus &interface, PQ9Frame &workingBuffer)
{
    if (command.getPayload()[0] == 0)
    {
        serial.println("TestService");

        MicroResistojetHandler *mr;
        if (command.getPayload()[2] == 1)
        {
            serial.println("LPM - Not working");
            return true;
            mr = &lpm;
        }
        else if (command.getPayload()[2] == 2)
        {
            serial.print("VLM - ");
            mr = &vlm;
            serial.println("Not working");
            return true;
        }
        else
        {
            return true;
        }

        const unsigned char val = command.getPayload()[3];
        if (val == 0)
        {
            mr->stopMR();
            serial.println("Stopped");
        }
        else if (val <= 10)
        {
            const auto timerPeriod = TIMER_PERIOD/val;
            const auto time_hold = (timerPeriod/2);
            mr->startMR(timerPeriod, TIME_SPIKE, time_hold);

            serial.print("Started with timer period ");
            serial.print(timerPeriod, DEC);
            serial.print(" and spike time ");
            serial.print(TIME_SPIKE, DEC);
            serial.print(" and hold time ");
            serial.print(time_hold, DEC);
            serial.println();
        }
        else
        {
            serial.println("No action");
        }

        return true;
    }
    return false;
}


