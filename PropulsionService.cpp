/*
 * PropulsionService.cpp
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#include "PropulsionService.h"
#include "MB85RS.h"

#define NUM_ELEM(x) (sizeof(x)/sizeof(x[0]))

#define TIMER_COUNTER TIMER_A2_BASE

#define START_SEQ_BYTE   0xAA
#define STOP_SEQ_BYTE    0xBB
#define START_FRAME_BYTE 0xCC
#define STOP_NOT_OK_BYTE 0xDD
#define STOP_OK_BYTE     0xEE

#define SEND_DATA_NOW 0xFF

extern MB85RS fram;

PropulsionService * PropulsionService::prop = nullptr;

volatile uint_fast32_t PropulsionService::num_oflw = 0;

// handler for the virtual 32-bit timer
void PropulsionService::_handler_timer_overflow()
{
    MAP_Timer_A_clearInterruptFlag(TIMER_COUNTER);
    ++num_oflw;
}

// PropulsionService constructor
PropulsionService::PropulsionService(const char * mrName[], const unsigned int mrConfig[], unsigned int count)
{
    if (prop != nullptr)
        prop->~PropulsionService();

    prop = this;

    MAP_Timer_A_configureContinuousMode(TIMER_COUNTER, &continousConfig);
    MAP_Timer_A_clearInterruptFlag(TIMER_COUNTER);
    MAP_Timer_A_registerInterrupt(TIMER_COUNTER, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, _handler_timer_overflow);
    MAP_Timer_A_startCounter(TIMER_COUNTER, TIMER_A_CONTINUOUS_MODE);

    task1 = new Task(saveData);
    task2 = new Task(saveData);

    if (count > NUM_ELEM(mr))
    {
        // throw "Count must not exceed capacity";
        count = NUM_ELEM(mr);
    }

    num_mr = count;

    for (unsigned int i = 0; i < num_mr; ++i)
    {
        mr[i] = new MicroResistojetHandler(mrName[i], mrConfig[i], mrAction);
    }
}

// PropulsionService destructor
PropulsionService::~PropulsionService()
{
    for (unsigned int i = 0; i < num_mr; ++i)
        delete mr[i];

    delete task1;
    delete task2;

    prop = nullptr;

    MAP_Timer_A_stopTimer(TIMER_COUNTER);
    MAP_Timer_A_unregisterInterrupt(TIMER_COUNTER, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT);
    MAP_Timer_A_clearInterruptFlag(TIMER_COUNTER);

    num_oflw = 0;
}

// Start a micro-resistojet
bool PropulsionService::startMR(MicroResistojetHandler * myMR, bool save, const unsigned char * config)
{
    struct MicroResistojetHandler::params_t configMR =
    {
        (uint_fast8_t)config[0],
        (uint_fast8_t)config[1],
        (uint_fast8_t)config[2],
        ((uint_fast16_t)config[3])<<8 | ((uint_fast16_t)config[4]),
        ((uint_fast16_t)config[5])<<8 | ((uint_fast16_t)config[6]),
        ((uint_fast16_t)config[7])<<8 | ((uint_fast16_t)config[8])
    };

    bool ret = myMR->startMR(configMR, save);

    const char * name = myMR->getName();
    if (name){
        Console::log("%s - ", name);
    }

    if (ret)
    {
        Console::log("Started with timer period %d ms, hold time %d ms and spike time %d.%d%d%d ms",
                     configMR.timerPeriod, configMR.time_hold, configMR.time_spike/1000, configMR.time_spike/100, configMR.time_spike/10, configMR.time_spike%10);
    }
    else
    {
        Console::log("Wrong inputs");
    }

    return ret;
}

// Handle propulsion request
bool PropulsionService::operatePropulsion(const unsigned char request, const unsigned char action,
                                          const unsigned char * payload)
{
    if (request != SERVICE_RESPONSE_REQUEST)
    {
        // unknown request
        return false;
    }


    Console::log("PropulsionService: Request");

    MicroResistojetHandler * myMR;

    switch (action)
    {
        case 0: // STOP
            myMR = (MicroResistojetHandler *)MicroResistojetHandler::stopActiveMR();

            if (myMR == nullptr)
            {
                Console::log("Nothing to stop");
            }
            else
            {
                const char * name = myMR->getName();
                if (name){
                    Console::log("%s - Stopped", name);
                }
            }
            return true;

        case 1: // START
            return startMR(mr[payload[1]], payload[0], &payload[2]);


        case 2: // FRAM
            switch (payload[0])
            {
                case 0: // SEND
                    Console::log("Sending saved data...");
                    sendSavedData();
                    Console::log("All data sent.");
                    return true;

                case 1: // COUNT
                    Console::log("Number of sequences saved: %d", num_saved_data);
                    return true;

                case 2: // ERASE
                    Console::log("Erasing saved data...");
                    eraseSavedData();
                    Console::log("All data erased.");
                    return true;

                default:
                    Console::log("Wrong FRAM action");
                    return false;
            }

        default:
            Console::log("Wrong action");
            return false;
    }
}

// Process propulsion request
bool PropulsionService::process(DataMessage &command, DataMessage &workingBuffer)
{
    if (command.getService() == PROPULSION_SERVICE)
    {
        workingBuffer.setService(PROPULSION_SERVICE);
        workingBuffer.setMessageType(SERVICE_RESPONSE_REPLY);
        workingBuffer.setPayloadSize(1);
        workingBuffer.getDataPayload()[0] = operatePropulsion(command.getPayload()[1],
                                                          command.getPayload()[2],
                                                          &command.getPayload()[3]) ?
                SERVICE_RESPONSE_REPLY : SERVICE_RESPONSE_ERROR;
        return true;
    }
    return false;
}

// Get task 1
Task* PropulsionService::getTask1()
{
    return task1;
}

// Get task 2
Task* PropulsionService::getTask2()
{
    return task2;
}

// Save data to FRAM now
void PropulsionService::saveData()
{
    if (prop->mr_active == 1)
    {
        prop->task1->notify();
        prop->task2->notify();

        prop->executingTask = true;

        prop->hk.acquireTelemetry(acquireTelemetry);
        PROPTelemetryContainer* tc = prop->hk.getTelemetry();
        tc->setUpTime(++prop->mr_uptime);

        if (prop->mr_active == 1)
        {
            prop->saveThisData(tc);

            prop->executingTask = false;

            if (prop->mr_active == -1)
            {
                prop->mr_active = 0;
                prop->recordStop();
            }
        }
        else // prop->mr_active == -1
        {
            prop->executingTask = false;
            prop->recordStop();
        }
    }
}

// Handle micro-resistojet action
void PropulsionService::mrAction(const MicroResistojetHandler * mr)
{
    prop->reason = mr->getCurrentStatus();
    switch (prop->reason)
    {
        case MicroResistojetHandler::status_t::UNUSABLE:
        case MicroResistojetHandler::status_t::NEVER_STARTED:
            break;

        case MicroResistojetHandler::status_t::ON_HEAT_ONLY:
        case MicroResistojetHandler::status_t::ON_HEAT_AND_VALVE:
            prop->propStart(mr);
            break;

        default: // STOP
            if (prop->executingTask)
            {
                prop->mr_active = -1;
                prop->myGlobalTime = getGlobalTime();
            }
            else
            {
                prop->recordStop();
            }
            break;
    }
}

// Start saving data to FRAM
void PropulsionService::propStart(const MicroResistojetHandler * mr)
{
    const int_fast64_t min_size = 3*sizeof(unsigned char) + 2*sizeof(const uint_fast32_t) +
            sizeof(const char *) + sizeof(const struct MicroResistojetHandler::params_t) +
            sizeof(reason);

    if ((mr_active == 0) && (min_size <= (MEM_SIZE-((int_fast64_t)pos_fram)))) {
        mr_active = 1;
        mr_uptime = 0;
        task1->notify();
        task2->notify();

        last_start_pos_fram = pos_fram;

        saveByte(START_SEQ_BYTE);

        saveGlobalTime();

        const char * name = mr->getName();
        fram.write(pos_fram, (unsigned char *)&name, sizeof(const char *));
        pos_fram += sizeof(const char *);

        fram.write(pos_fram, (unsigned char *)mr->getCurrentParams(),
                   sizeof(const struct MicroResistojetHandler::params_t));
        pos_fram += sizeof(const struct MicroResistojetHandler::params_t);

        ++num_saved_data;
    }
}

// Save stop action to FRAM
uint_fast32_t PropulsionService::recordStop(bool ok)
{
    saveByte(STOP_SEQ_BYTE);

    const uint_fast32_t globalTime = saveGlobalTime(mr_active != -1);
    saveByte(ok ? STOP_OK_BYTE : STOP_NOT_OK_BYTE);

    fram.write(pos_fram, (unsigned char *)&reason, sizeof(reason));
    pos_fram += sizeof(reason);

    mr_active = 0;
    return globalTime;
}

// Save byte to FRAM
void PropulsionService::saveByte(unsigned char byte)
{
    fram.write(pos_fram, (unsigned char *)&byte, sizeof(unsigned char));
    pos_fram += sizeof(unsigned char);
}

// Get 32-bit counter value
uint_fast32_t PropulsionService::getGlobalTime()
{
    volatile uint_fast32_t _num_oflw;
    volatile uint16_t base;

    do {
        _num_oflw = num_oflw;
        base = MAP_Timer_A_getCounterValue(TIMER_COUNTER);
    } while (_num_oflw != num_oflw);

    return (1 << (8*sizeof(uint16_t)))*_num_oflw + ((uint_fast32_t)base);
}

// Save 32-bit counter value to FRAM
uint_fast32_t PropulsionService::saveGlobalTime(bool now)
{
    const uint_fast32_t globalTime = now ? getGlobalTime() : myGlobalTime;
    fram.write(pos_fram, (unsigned char *)&globalTime, sizeof(const uint_fast32_t));
    pos_fram += sizeof(const uint_fast32_t);
    return globalTime;
}

// Send FRAM saved data to user through serial port
void PropulsionService::sendSavedData()
{
    if (mr_active == 0)
    {
        sendSavedDataNow();
    }
    else
    {
        bool unfinished = false;
        executingTask = true;

        if (mr_active == -1)
        {
            recordStop();
            sendSavedDataNow();
        }
        else
        {
            sendSavedDataNow();

            if (mr_active == -1)
            {
                sendStop(recordStop(), reason);
            }
            else if (mr_active == 1)
            {
                unfinished = true;
                sendUnfinished();
            }
        }

        executingTask = false;

        if (mr_active == -1)
        {
            const uint_fast32_t globalTime = recordStop();
            if (!unfinished)
                sendStop(globalTime, reason);
        }
    }
}

// Send count number through serial port
inline void PropulsionService::sendCount(const unsigned int num_saved_data)
{
    Console::log("%d,\"Count\",%d",getGlobalTime(), num_saved_data);
}

// Send count number through serial port
void PropulsionService::sendCount()
{
    sendCount(num_saved_data);
}

// Send start sequence information through serial port
void PropulsionService::sendStart(const uint_fast32_t globalTime, const char * name,
                                  const struct MicroResistojetHandler::params_t * params)
{
    Console::log("%d,\"Start\",\"", globalTime);
    if (name) {
        Console::log(name);
    };

    Console::log("\",%d,%d,%d,%d,%d,%d",params->time_work,params->time_before,params->duty_c_heat,params->timerPeriod,params->time_hold,params->time_spike);
}

// Send stop sequence information through serial port
void PropulsionService::sendStop(const uint_fast32_t globalTime,
                                 const enum MicroResistojetHandler::status_t reason,
                                 bool ok)
{
    Console::log("%d,\"Stop\",\"%s\",%d",globalTime,ok ? "OK" : "FULL",reason);
}

// Send unfinished sequence information through serial port
void PropulsionService::sendUnfinished()
{
    Console::log("%d,\"Unfinished\"",getGlobalTime());
}

// Send FRAM saved data through serial port now
void PropulsionService::sendSavedDataNow()
{
    prop->sendCount();

    if (prop->num_saved_data == 0)
        return;

    unsigned char byteChar;
    uint_fast32_t globalTime;
    const char * name;
    struct MicroResistojetHandler::params_t params;
    enum MicroResistojetHandler::status_t reason;
    PROPTelemetryContainer tc;

    fram.read(0, &byteChar, sizeof(unsigned char));

    for (unsigned int i = sizeof(unsigned char);
            (i < MEM_SIZE) && (byteChar == START_SEQ_BYTE);
            i += sizeof(unsigned char))
    {
        fram.read(i, (unsigned char *)&globalTime, sizeof(const uint_fast32_t));
        i += sizeof(const uint_fast32_t);

        fram.read(i, (unsigned char *)&name, sizeof(const char *));
        i += sizeof(const char *);

        fram.read(i, (unsigned char *)&params, sizeof(const struct MicroResistojetHandler::params_t));
        i += sizeof(const struct MicroResistojetHandler::params_t);

        sendStart(globalTime, name, &params);

        fram.read(i, &byteChar, sizeof(unsigned char));
        while (byteChar == START_FRAME_BYTE)
        {
            i += sizeof(unsigned char);

            fram.read(i, (unsigned char *)&globalTime, sizeof(const uint_fast32_t));
            i += sizeof(const uint_fast32_t);

            fram.read(i, tc.getArray(), tc.size());
            i += tc.size();

            sendThisData(globalTime, &tc);

            fram.read(i, &byteChar, sizeof(unsigned char));
        }

        if (byteChar == STOP_SEQ_BYTE)
        {
            i += sizeof(unsigned char);

            fram.read(i, (unsigned char *)&globalTime, sizeof(const uint_fast32_t));
            i += sizeof(const uint_fast32_t);

            fram.read(i, &byteChar, sizeof(unsigned char));
            i += sizeof(unsigned char);

            fram.read(i, (unsigned char *)&reason, sizeof(reason));
            i += sizeof(reason);

            sendStop(globalTime, reason, (byteChar==STOP_OK_BYTE));

            fram.read(i, &byteChar, sizeof(unsigned char));
        }

        if (i == 0)
            break;
    }
}

// Erase FRAM
void PropulsionService::eraseSavedData()
{
    if (mr_active == 0)
    {
        fram.erase();
        pos_fram = 0;
        num_saved_data = 0;
        last_start_pos_fram = 0;
    }
    else
    {
        executingTask = true;

        unsigned char buff[sizeof(unsigned char) + sizeof(const uint_fast32_t) +
                           sizeof(const char) + sizeof(const struct MicroResistojetHandler::params_t)];

        fram.read(last_start_pos_fram, (unsigned char *)&buff, sizeof(buff));
        fram.erase();

        if (mr_active == 1)
        {
            fram.write(0, (unsigned char *)&buff, sizeof(buff));

            pos_fram = sizeof(buff);
            num_saved_data = 1;
            last_start_pos_fram = 0;

            if (mr_active == -1)
            {
                mr_active = 0;
                eraseSavedData();
            }
        }
        else // mr_active == -1
        {
            pos_fram = 0;
            num_saved_data = 0;
            last_start_pos_fram = 0;
            mr_active = 0;
        }

        executingTask = false;

        if (mr_active == -1)
            recordStop();
    }
}

// Save telemetry data to FRAM
void PropulsionService::saveThisData(PROPTelemetryContainer* tc)
{
    const int_fast64_t size = 3*sizeof(unsigned char) + 2*sizeof(const uint_fast32_t) +
            ((int_fast64_t)(tc->size())) + sizeof(reason);

    if (size > (MEM_SIZE-((int_fast64_t)pos_fram)))
    {
        //throw "Not enough FRAM!";
        Console::log("Not enough FRAM! Not saving data until FRAM erased");
        recordStop(false);
        return;
    }

    saveByte(START_FRAME_BYTE);

    saveGlobalTime();

    fram.write(pos_fram, tc->getArray(), tc->size());
    pos_fram += tc->size();
}

// Send telemetry data through serial port
void PropulsionService::sendThisData(const uint_fast32_t globalTime, PROPTelemetryContainer* tc)
{
    int_fast16_t i;

    Console::log("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                 globalTime,
                 tc->getUpTime(),
                 tc->getBusStatus(),
                 tc->getBusVoltage(),
                 tc->getBusCurrent(),
                 tc->getValveHoldStatus(),
                 tc->getValveHoldVoltage(),
                 tc->getValveHoldCurrent(),
                 tc->getValveSpikeStatus(),
                 tc->getValveSpikeVoltage(),
                 tc->getValveSpikeCurrent(),
                 tc->getHeatersStatus(),
                 tc->getHeatersVoltage(),
                 tc->getHeatersCurrent(),
                 tc->getTmpStatus(),
                 tc->getTemperature());
}
