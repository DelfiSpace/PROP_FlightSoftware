/*
 * MicroResistojetHandler.cpp
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#include "MicroResistojetHandler.h"

#define NUM_ELEM(x) (sizeof(x)/sizeof(x[0]))

volatile MicroResistojetHandler * MicroResistojetHandler::activeMR = nullptr;

const struct MicroResistojetHandler::config_t MicroResistojetHandler::configs[] =
{
    { GPIO_PORT_P2, GPIO_PIN5, GPIO_PORT_P2, GPIO_PIN6, GPIO_PIN7, TIMER_A0_BASE }, // Heat (pin 21), Spike (pin 22), Hold (pin 23)
    { GPIO_PORT_P7, GPIO_PIN6, GPIO_PORT_P7, GPIO_PIN5, GPIO_PIN4, TIMER_A1_BASE }, // Heat (pin 28), Spike (pin 27), Hold (pin 26)
//  { GPIO_PORT_P5, GPIO_PIN7, GPIO_PORT_P6, GPIO_PIN6, GPIO_PIN7, TIMER_A2_BASE }, // Heat (pin 71), Spike (pin 80), Hold (pin 81)
    { GPIO_PORT_P8, GPIO_PIN2, GPIO_PORT_P9, GPIO_PIN2, GPIO_PIN3, TIMER_A3_BASE }  // Heat (pin 46), Spike (pin 74), Hold (pin 75)
};

const unsigned int MicroResistojetHandler::num_configs = NUM_ELEM(MicroResistojetHandler::configs);

const uint32_t availableTimers[] =
{
    TIMER_A0_BASE,
    TIMER_A1_BASE,
//  TIMER_A2_BASE,
    TIMER_A3_BASE
};

inline void smallDelay(const uint32_t microseconds)
{
    const uint32_t cycles_per_it = 48;
    const uint32_t d1 = ((((uint64_t)MAP_CS_getMCLK()) * ((uint64_t)microseconds))/((uint64_t)1000000))/((uint64_t)(cycles_per_it-3));

    for (uint32_t k = 0; k < d1; ++k)
        __delay_cycles(cycles_per_it);
}

MicroResistojetHandler::MicroResistojetHandler( const char * name, const unsigned int configId,
                                                void (*const userFunction)( const MicroResistojetHandler * ) ) :
        MRIName(name),
        MRIConfigId(configId),
        MRIUserFunction(userFunction)
{

    if (configId >= num_configs)
    {
        //throw "Config ID does not exist!";
        return;
    }

    const struct config_t * my_config = &configs[configId];

    MRITimerTime = my_config->timerOutput;
    for (unsigned int i = 0; i < NUM_ELEM(availableTimers); ++i) {
        if (availableTimers[i] != MRITimerTime) {
            MRITimerTime = availableTimers[i];
            break;
        }
    }

    if (MRITimerTime == my_config->timerOutput)
    {
        //throw "Not enough available timers!";
        return;
    }

    setUp(my_config->portHeat, my_config->pinHeat,
          my_config->portValve, my_config->pinSpike, my_config->pinHold,
          my_config->timerOutput);
}

MicroResistojetHandler::MicroResistojetHandler( const char * name, const unsigned int configId, const uint32_t timerTime,
                                                void (*const userFunction)( const MicroResistojetHandler * ) ) :
        MRIName(name),
        MRIConfigId(configId),
        MRITimerTime(timerTime),
        MRIUserFunction(userFunction)
{
    const struct config_t * my_config = &configs[configId];

    if (configId >= num_configs)
    {
        //throw "Config ID does not exist!";
        return;
    }
    else if (my_config->timerOutput == timerTime)
    {
        //throw "Specified timer cannot be equal to config timer!";
        return;
    }
    else
    {
        bool okTimerTime = false;
        for (unsigned int i = 0; i < NUM_ELEM(availableTimers); ++i) {
            if (availableTimers[i] == timerTime) {
                okTimerTime = true;
                break;
            }
        }

        if (!okTimerTime)
        {
            //throw "Specified timer does not exist!";
            return;
        }
    }

    setUp(my_config->portHeat, my_config->pinHeat,
          my_config->portValve, my_config->pinSpike, my_config->pinHold,
          my_config->timerOutput);
}

inline void MicroResistojetHandler::setUp( const unsigned long portHeat, const unsigned long pinHeat,
                                    const unsigned long portValve, const unsigned long pinSpike, const unsigned long pinHold,
                                    const uint32_t timerOutput )
{
    currentStatus = NEVER_STARTED;

    MRIPortHeat = portHeat;
    MRIPinHeat = pinHeat;

    MRIPortValve = portValve;
    MRIPinSpike = pinSpike;
    MRIPinHold = pinHold;

    MRITimerOutput = timerOutput;

    MAP_GPIO_setOutputLowOnPin( MRIPortValve, MRIPinSpike|MRIPinHold );
    MAP_GPIO_setOutputLowOnPin( MRIPortHeat, MRIPinHeat );
    MAP_GPIO_setAsOutputPin( MRIPortValve, MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPortHeat, MRIPinHeat );
}

MicroResistojetHandler::~MicroResistojetHandler()
{
    stopMR(STOP_OBJECT_DESTROYED);
}

const char * MicroResistojetHandler::getName() const
{
    return MRIName;
}

enum MicroResistojetHandler::status_t MicroResistojetHandler::getCurrentStatus() const
{
    return currentStatus;
}

const struct MicroResistojetHandler::params_t * MicroResistojetHandler::getCurrentParams() const
{
    return &currentParams;
}

const MicroResistojetHandler * MicroResistojetHandler::stopActiveMR()
{
    return _stopActiveMR(STOP_MANUAL);
}

const MicroResistojetHandler * MicroResistojetHandler::_stopActiveMR(enum status_t stopReason)
{
    volatile MicroResistojetHandler * activeMRnow = activeMR;

    return ((activeMR != nullptr) && ((MicroResistojetHandler *)activeMRnow)->stopMR(stopReason)) ?
            (const MicroResistojetHandler *)activeMRnow :
            nullptr;
}

void MicroResistojetHandler::_handler_stop_active_MR()
{
    _stopActiveMR(STOP_TIMEOUT);
}

void MicroResistojetHandler::_handler_after_delay()
{
    if (activeMR != nullptr)
    {
        MAP_Timer_A_clearInterruptFlag(activeMR->MRITimerTime);
        MAP_Timer_A_setCompareValue(activeMR->MRITimerTime, TIMER_A_CAPTURECOMPARE_REGISTER_0, activeMR->upConfigCounter.timerPeriod);
        activeMR->compareConfig_PWMSpike.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
        activeMR->compareConfig_PWMHold.compareOutputMode  = TIMER_A_OUTPUTMODE_RESET_SET;
        MAP_Timer_A_initCompare(activeMR->MRITimerOutput, (Timer_A_CompareModeConfig *)&activeMR->compareConfig_PWMSpike);
        MAP_Timer_A_initCompare(activeMR->MRITimerOutput, (Timer_A_CompareModeConfig *)&activeMR->compareConfig_PWMHold);
        MAP_Timer_A_registerInterrupt(activeMR->MRITimerTime, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, _handler_stop_active_MR);
        activeMR->currentStatus = ON_HEAT_AND_VALVE;
        if (activeMR->MRIUserFunction && activeMR->notify)
            activeMR->MRIUserFunction((const MicroResistojetHandler *)activeMR);
    }
}

bool MicroResistojetHandler::startMR(struct params_t c, bool notify)
{
    if (!(  c.time_work   >= 1 &&
            c.time_spike  >= 350 &&
            c.time_spike  <= 3800 &&
            c.time_hold   >= 1 &&
            c.time_hold   >= (c.time_spike/1000) &&
            c.time_hold   <= c.timerPeriod &&
            c.timerPeriod >= ((c.time_spike+1000-1)/1000) &&
            c.timerPeriod <  2000 &&
            c.time_before <  128 &&
            c.time_work   <  128 &&
            c.duty_c_heat <= 100  ))
    {
        return false;
    }

    _stopActiveMR((activeMR == this) ? STOP_RECONFIGURING : STOP_START_DIFFERENT);
    activeMR = this;
    this->notify = notify;

    upConfig.timerPeriod = (uint_fast16_t)((((uint_fast64_t)c.timerPeriod) << 12)/125);
    upConfigCounter.timerPeriod = c.time_before << 9;
    compareConfig_PWMHold.compareValue  = (uint_fast16_t)((((uint_fast64_t)c.time_hold) << 12)/125);
    compareConfig_PWMSpike.compareValue = (uint_fast16_t)((((uint_fast64_t)c.time_spike) << 9)/15625);
    compareConfig_PWMHeat.compareValue  = (uint_fast16_t)((((uint_fast32_t)upConfig.timerPeriod) * ((uint_fast32_t)c.duty_c_heat))/100);

    /* Setting ACLK to REFO at 32Khz */
    MAP_CS_setReferenceOscillatorFrequency(CS_REFO_32KHZ);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Setting MRIPinSpike and MRIPinHold and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPortValve,
        MRIPinSpike|MRIPinHold, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Setting MRIPinHeat and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPortHeat,
        MRIPinHeat, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A0 for UpDown Mode and starting */
    MAP_Timer_A_configureUpMode(MRITimerOutput, &upConfig);

    /* Initialize compare registers to generate PWMHold */
    MAP_Timer_A_initCompare(MRITimerOutput, &compareConfig_PWMHold);

    /* Initialize compare registers to generate PWMSpike */
    MAP_Timer_A_initCompare(MRITimerOutput, &compareConfig_PWMSpike);

    /* Initialize compare registers to generate PWMHeat */
    MAP_Timer_A_initCompare(MRITimerOutput, &compareConfig_PWMHeat);

    /* Configuring Timer_A1 for UpDown Mode and starting */
    MAP_Timer_A_configureUpMode(MRITimerTime, &upConfigCounter);

    const uint_fast16_t delay = upConfigCounter.timerPeriod;
    upConfigCounter.timerPeriod = c.time_work << 9;

    currentParams = c;

    if (delay > 0)
    {
        MAP_Timer_A_clearInterruptFlag(MRITimerTime);
        MAP_Timer_A_registerInterrupt(MRITimerTime, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, _handler_after_delay);
        currentStatus = ON_HEAT_ONLY;

        if (MRIUserFunction && notify)
            MRIUserFunction(this);
    }
    else
    {
        _handler_after_delay();
    }

    /* Starting Timer_A0 */
    MAP_Timer_A_startCounter(MRITimerOutput, TIMER_A_UP_MODE);

    smallDelay(120); // microseconds

    /* Starting Timer_A1 */
    MAP_Timer_A_startCounter(MRITimerTime, TIMER_A_UP_MODE);

    return true;
}

bool MicroResistojetHandler::stopMR(enum status_t stopReason)
{
    volatile MicroResistojetHandler * activeMRnow;

    activeMRnow = activeMR;
    if (activeMRnow != this)
        return false;

    MAP_Timer_A_stopTimer(MRITimerTime);
    MAP_Timer_A_clearInterruptFlag(MRITimerTime);
    MAP_Timer_A_unregisterInterrupt(MRITimerTime, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT);

    activeMRnow = activeMR;
    if (activeMRnow != this)
        return false;

    activeMR = nullptr;

    compareConfig_PWMHold.compareOutputMode  = TIMER_A_OUTPUTMODE_RESET;
    compareConfig_PWMSpike.compareOutputMode = TIMER_A_OUTPUTMODE_RESET;
    compareConfig_PWMHeat.compareOutputMode  = TIMER_A_OUTPUTMODE_RESET;

    MAP_Timer_A_initCompare(MRITimerOutput, &compareConfig_PWMHold);
    MAP_Timer_A_initCompare(MRITimerOutput, &compareConfig_PWMSpike);
    MAP_Timer_A_initCompare(MRITimerOutput, &compareConfig_PWMHeat);

    MAP_Timer_A_clearInterruptFlag(MRITimerOutput);

    while (MAP_Timer_A_getInterruptStatus(MRITimerOutput) == TIMER_A_INTERRUPT_NOT_PENDING);

    MAP_Timer_A_stopTimer(MRITimerOutput);
    MAP_Timer_A_clearInterruptFlag(MRITimerOutput);

    MAP_GPIO_setOutputLowOnPin( MRIPortValve, MRIPinSpike|MRIPinHold );
    MAP_GPIO_setOutputLowOnPin( MRIPortHeat, MRIPinHeat );
    MAP_GPIO_setAsOutputPin( MRIPortValve, MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPortHeat, MRIPinHeat );

    compareConfig_PWMHold.compareOutputMode  = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    compareConfig_PWMSpike.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    compareConfig_PWMHeat.compareOutputMode  = TIMER_A_OUTPUTMODE_RESET_SET;

    currentParams = { };
    currentStatus = stopReason;

    if (MRIUserFunction && notify)
        MRIUserFunction(this);

    return true;
}
