/*
 * MicroResistojetHandler.cpp
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#include <MicroResistojetHandler.h>

extern DSerial serial;

MicroResistojetHandler::MicroResistojetHandler(const unsigned long MRport, const unsigned long MRpinHeat,
                                               const unsigned long MRpinSpike, const unsigned long MRpinHold) :
        MRIPort(MRport), MRIPinHeat(MRpinHeat),
        MRIPinSpike(MRpinSpike), MRIPinHold(MRpinHold)  {
    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
}

void MicroResistojetHandler::startMR(uint_fast16_t timerPeriod,
        uint_fast16_t dutyCycle1, uint_fast16_t dutyCycle2)
{
    MAP_GPIO_setOutputHighOnPin( MRIPort, MRIPinHeat );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat );
    startValve(timerPeriod, dutyCycle1, dutyCycle2);
}

void MicroResistojetHandler::stopMR()
{
    stopValve();
    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat );
}

void MicroResistojetHandler::startValve(uint_fast16_t timerPeriod,
        uint_fast16_t dutyCycle1, uint_fast16_t dutyCycle2)
{
    upConfig.timerPeriod = (uint_fast16_t)((((uint_fast32_t)timerPeriod) << 9)/15625);
    compareConfig_PWMSpike.compareValue = (uint_fast16_t)((((uint_fast32_t)dutyCycle1) << 9)/15625);
    compareConfig_PWMHold.compareValue = (uint_fast16_t)((((uint_fast32_t)dutyCycle2) << 9)/15625);

    /* Setting ACLK to REFO at 32Khz */
    MAP_CS_setReferenceOscillatorFrequency(CS_REFO_32KHZ);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Setting MRIPinSpike and MRIPinHold and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPort,
        MRIPinSpike|MRIPinHold, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A1 for UpDown Mode and starting */
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);

    /* Initialize compare registers to generate PWMSpike */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMSpike);

    /* Initialize compare registers to generate PWMHold */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMHold);

    /* Starting Timer_A1 */
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}

void MicroResistojetHandler::stopValve()
{
    MAP_Timer_A_stopTimer(TIMER_A0_BASE);
    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinSpike|MRIPinHold );
}
