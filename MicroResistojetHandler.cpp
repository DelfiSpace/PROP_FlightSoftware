/*
 * MicroResistojetHandler.cpp
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#include <MicroResistojetHandler.h>

volatile MicroResistojetHandler * MicroResistojetHandler::activeMR = nullptr;

inline void smallDelay(const uint32_t microseconds)
{
    const uint32_t cycles_per_it = 48;
    const uint32_t d1 = ((((uint64_t)MAP_CS_getMCLK()) * ((uint64_t)microseconds))/((uint64_t)1000000))/((uint64_t)(cycles_per_it-3));

    for (uint32_t k = 0; k < d1; ++k)
        __delay_cycles(cycles_per_it);
}

MicroResistojetHandler::MicroResistojetHandler(const char * name,
                                               const unsigned long port, const unsigned long pinHeat,
                                               const unsigned long pinSpike, const unsigned long pinHold,
                                               const uint32_t timerOutput, const uint32_t timerTime,
                                               void (*userFunction)( bool ) ) :
        MRIName(name),
        MRIPort(port), MRIPinHeat(pinHeat),
        MRIPinSpike(pinSpike), MRIPinHold(pinHold),
        MRITimerOutput(timerOutput), MRITimerTime(timerTime),
        MRIUserFunction(userFunction)
{
    // TODO: Check consistency of arguments

    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
}

MicroResistojetHandler::~MicroResistojetHandler()
{
    stopMR();
}

const char * MicroResistojetHandler::getName()
{
    return MRIName;
}

bool MicroResistojetHandler::stopActiveMR()
{
    volatile MicroResistojetHandler * activeMRnow = activeMR;

    return (activeMR != nullptr) ?
           ((MicroResistojetHandler *)activeMRnow)->stopMR() :
           false;
}

void MicroResistojetHandler::_handler_stop_active_MR()
{
    stopActiveMR();
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
    }
}

bool MicroResistojetHandler::startMR(struct params c)
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

    stopActiveMR();
    activeMR = this;

    upConfig.timerPeriod = (uint_fast16_t)((((uint_fast64_t)c.timerPeriod) << 12)/125);
    upConfigCounter.timerPeriod = c.time_before << 9;
    compareConfig_PWMHold.compareValue  = (uint_fast16_t)((((uint_fast64_t)c.time_hold) << 12)/125);
    compareConfig_PWMSpike.compareValue = (uint_fast16_t)((((uint_fast64_t)c.time_spike) << 9)/15625);
    compareConfig_PWMHeat.compareValue  = (uint_fast16_t)((((uint_fast32_t)upConfig.timerPeriod) * c.duty_c_heat)/100);

    /* Setting ACLK to REFO at 32Khz */
    MAP_CS_setReferenceOscillatorFrequency(CS_REFO_32KHZ);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Setting MRIPinSpike and MRIPinHold and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPort,
        MRIPinHeat|MRIPinSpike|MRIPinHold, GPIO_PRIMARY_MODULE_FUNCTION);

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

    if (delay > 0)
    {
        MAP_Timer_A_clearInterruptFlag(MRITimerTime);
        MAP_Timer_A_registerInterrupt(MRITimerTime, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, _handler_after_delay);
    }
    else
    {
        _handler_after_delay();
    }

    if (MRIUserFunction)
        MRIUserFunction(true);

    /* Starting Timer_A0 */
    MAP_Timer_A_startCounter(MRITimerOutput, TIMER_A_UP_MODE);

    smallDelay(120); // microseconds

    /* Starting Timer_A1 */
    MAP_Timer_A_startCounter(MRITimerTime, TIMER_A_UP_MODE);

    return true;
}

bool MicroResistojetHandler::stopMR()
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

    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );

    compareConfig_PWMHold.compareOutputMode  = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    compareConfig_PWMSpike.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    compareConfig_PWMHeat.compareOutputMode  = TIMER_A_OUTPUTMODE_RESET_SET;

    if (MRIUserFunction)
        MRIUserFunction(false);

    return true;
}
