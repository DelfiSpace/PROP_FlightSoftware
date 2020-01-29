/*
 * MicroResistojetHandler.cpp
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#include <MicroResistojetHandler.h>

extern DSerial serial;

MicroResistojetHandler * MicroResistojetHandler::activeMR = nullptr;

MicroResistojetHandler::MicroResistojetHandler(const unsigned long MRport, const unsigned long MRpinHeat,
                                               const unsigned long MRpinSpike, const unsigned long MRpinHold) :
        MRIPort(MRport), MRIPinHeat(MRpinHeat),
        MRIPinSpike(MRpinSpike), MRIPinHold(MRpinHold)  {
    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
}

void MicroResistojetHandler::stopActiveMR()
{
    if (activeMR) {
        activeMR->stopMR();
        activeMR = nullptr;
    }
}

void MicroResistojetHandler::_handler_after_delay()
{
    MAP_Timer_A_clearInterruptFlag(TIMER_A1_BASE);
    MAP_Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0, activeMR->upConfigCounter.timerPeriod);
    TA0CCTL3 = TIMER_A_OUTPUTMODE_RESET_SET;
    TA0CCTL4 = TIMER_A_OUTPUTMODE_RESET_SET;
    MAP_Timer_A_registerInterrupt(TIMER_A1_BASE, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, stopActiveMR);
}

void MicroResistojetHandler::startMR(uint_fast16_t time_work, uint_fast16_t duty_cycle_heat,
    uint_fast16_t delayBeforeValve, uint_fast16_t timerPeriod, uint_fast16_t time_hold,
    uint_fast16_t time_spike)
{
    stopActiveMR();
    activeMR = this;

    upConfig.timerPeriod = (uint_fast16_t)((((uint_fast64_t)timerPeriod) << 12)/125);
    upConfigCounter.timerPeriod = delayBeforeValve << 9;
    compareConfig_PWMHold.compareValue = (uint_fast16_t)((((uint_fast64_t)time_hold) << 12)/125);
    compareConfig_PWMSpike.compareValue = (uint_fast16_t)((((uint_fast64_t)time_spike) << 9)/15625);
    compareConfig_PWMHeat.compareValue = (uint_fast16_t)((((uint_fast32_t)upConfig.timerPeriod) * duty_cycle_heat)/100);

    /* Setting ACLK to REFO at 32Khz */
    MAP_CS_setReferenceOscillatorFrequency(CS_REFO_32KHZ);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Setting MRIPinSpike and MRIPinHold and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPort,
        MRIPinHeat|MRIPinSpike|MRIPinHold, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A0 for UpDown Mode and starting */
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);

    /* Initialize compare registers to generate PWMHold */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMHold);

    /* Initialize compare registers to generate PWMSpike */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMSpike);

    /* Initialize compare registers to generate PWMHeat */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMHeat);

    /* Configuring Timer_A1 for UpDown Mode and starting */
    MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &upConfigCounter);

    const uint_fast16_t delay = upConfigCounter.timerPeriod;
    upConfigCounter.timerPeriod = time_work << 9;

    if (delay > 0)
    {
        MAP_Timer_A_clearInterruptFlag(TIMER_A1_BASE);
        MAP_Timer_A_registerInterrupt(TIMER_A1_BASE, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, _handler_after_delay);
    }
    else
    {
        _handler_after_delay();
    }


    /* Starting Timer_A0 */
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    /* Starting Timer_A1 */
    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
}

void MicroResistojetHandler::stopMR()
{
    TA0CCTL4 = TIMER_A_OUTPUTMODE_RESET;
    TA0CCTL3 = TIMER_A_OUTPUTMODE_RESET;
    TA0CCTL2 = TIMER_A_OUTPUTMODE_RESET;
    MAP_Timer_A_clearInterruptFlag(TIMER_A0_BASE);

    if (TA0CTL & 0x18)
        while(!(TA0CTL & 1));

    MAP_Timer_A_stopTimer(TIMER_A0_BASE);
    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
    MAP_Timer_A_clearInterruptFlag(TIMER_A1_BASE);
    MAP_Timer_A_unregisterInterrupt(TIMER_A1_BASE, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT);

    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
}
