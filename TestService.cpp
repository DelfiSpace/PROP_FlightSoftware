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
#define TIMER_PERIOD 127
#define DUTY_CYCLE1 32
#define DUTY_CYCLE2 96

bool TestService::process(PQ9Frame &command, PQ9Bus &interface, PQ9Frame &workingBuffer)
{
    if (command.getPayload()[0] == 0)
    {
        serial.println("TestService");

        MicroResistojetHandler *mr;
        if (command.getPayload()[1] == 1)
        {
            serial.print("LPM - ");
            mr = &lpm;
        }
        else if (command.getPayload()[1] == 2)
        {
            serial.print("VLM - ");
            mr = &vlm;
        }
        else
        {
            return true;
        }

        const unsigned char val = command.getPayload()[2];
        if (val == 0)
        {
            mr->stopMR();
            serial.println("Stopped");
        }
        else if (val <= DUTY_CYCLE1 && ((val&(val-1)) == 0))
        {
            const auto timerPeriod = TIMER_PERIOD/val;
            const auto dutyCycle1 = DUTY_CYCLE1/val;
            const auto dutyCycle2 = DUTY_CYCLE2/val;
            mr->startMR(timerPeriod, dutyCycle1, dutyCycle2);

            serial.print("Started with timer period ");
            serial.print(timerPeriod, DEC);
            serial.print(" and duty cycles ");
            serial.print(dutyCycle1, DEC);
            serial.print(" and ");
            serial.print(dutyCycle2, DEC);
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


