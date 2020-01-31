/*
 * PropulsionService.cpp
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#include "PropulsionService.h"
#include "DSerial.h"

extern DSerial serial;

PropulsionService * PropulsionService::prop = nullptr;

PropulsionService::PropulsionService(const char * mrName[], const unsigned int mrConfig[], unsigned int count)
{
    if (prop != nullptr)
        prop->~PropulsionService();

    prop = this;

    task1 = new Task(sendData);
    task2 = new Task(sendData);

    if (count > sizeof(mr)/sizeof(MicroResistojetHandler *))
    {
        // throw "Count must not exceed capacity";
        count = 10;
    }

    num_mr = count;

    for (unsigned int i = 0; i < num_mr; ++i)
    {
        mr[i] = new MicroResistojetHandler(mrName[i], mrConfig[i], mrAction);
    }
}

PropulsionService::~PropulsionService()
{
    for (unsigned int i = 0; i < num_mr; ++i)
        delete mr[i];

    delete task1;
    delete task2;
}

bool PropulsionService::operatePropulsion(DataMessage &command)
{

    if (command.getPayload()[1] != SERVICE_RESPONSE_REQUEST)
    {
        // unknown request
        return false;
    }


    serial.println("PropulsionService: Test Request");

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

        MicroResistojetHandler * myMR = mr[_mr-1];

        ret = myMR->startMR(configMR);

        const char * name = myMR->getName();
        if (name) serial.print(name);
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

bool PropulsionService::process(DataMessage &command, DataMessage &workingBuffer)
{
    if (command.getPayload()[0] == PROPULSION_SERVICE)
    {
        workingBuffer.setSize(2);
        workingBuffer.getPayload()[0] = PROPULSION_SERVICE;
        workingBuffer.getPayload()[1] = operatePropulsion(command) ? SERVICE_RESPONSE_REPLY : SERVICE_RESPONSE_ERROR;
        return true;
    }
    return false;
}

Task* PropulsionService::getTask1()
{
    return task1;
}

Task* PropulsionService::getTask2()
{
    return task2;
}

void PropulsionService::sendData()
{
    if (prop->mr_active)
    {
        prop->task1->notify();
        prop->task2->notify();
        prop->hk.acquireTelemetry(acquireTelemetry);
        PROPTelemetryContainer* tc = prop->hk.getTelemetry();
        tc->setUpTime(++prop->mr_uptime);
        prop->sendThisData(tc, 0); // TODO: Send global time
    }
}

void PropulsionService::mrAction(const MicroResistojetHandler * mr)
{
    switch (mr->getCurrentStatus()) {
        case MicroResistojetHandler::status_t::UNUSABLE:
        case MicroResistojetHandler::status_t::NEVER_STARTED:
            break;

        case MicroResistojetHandler::status_t::ON_HEAT_ONLY:
        case MicroResistojetHandler::status_t::ON_HEAT_AND_VALVE:
            if (!prop->mr_active) {
                prop->mr_active = true;
                prop->mr_uptime = 0;
                prop->task1->notify();
                prop->task2->notify();
            }
            break;

        default: // STOP
            prop->mr_active = false;
            break;
    }
}

void PropulsionService::sendThisData(PROPTelemetryContainer* tc, const uint_fast32_t globalTime, bool binary)
{
    if (binary)
    {
        serial.print(((unsigned char *)&globalTime)[3]);
        serial.print(((unsigned char *)&globalTime)[2]);
        serial.print(((unsigned char *)&globalTime)[1]);
        serial.print(((unsigned char *)&globalTime)[0]);

        unsigned char * telemetry = tc->getArray();

        for (int i = 0; i < tc->size(); ++i, ++telemetry)
        {
            serial.print(*telemetry);
        }

        return;
    }

    int_fast16_t i;

    serial.print(tc->getUpTime(), DEC); serial.print(',');

    serial.print(globalTime, DEC); serial.print(',');

    serial.print(tc->getBusStatus(),  DEC); serial.print(',');
    serial.print(tc->getBusVoltage(), DEC); serial.print(',');
    i = tc->getBusCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getValveHoldStatus(),  DEC); serial.print(',');
    serial.print(tc->getValveHoldVoltage(), DEC); serial.print(',');
    i = tc->getValveHoldCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getValveSpikeStatus(),  DEC); serial.print(',');
    serial.print(tc->getValveSpikeVoltage(), DEC); serial.print(',');
    i = tc->getValveSpikeCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getHeatersStatus(),  DEC); serial.print(',');
    serial.print(tc->getHeatersVoltage(), DEC); serial.print(',');
    i = tc->getHeatersCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getTmpStatus(), DEC); serial.print(',');
    i = tc->getTemperature();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.println(i, DEC);
}
