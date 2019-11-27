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
    upDownConfig.timerPeriod = timerPeriod;
    compareConfig_PWM1.compareValue = dutyCycle1;
    compareConfig_PWM2.compareValue = dutyCycle2;

    /* Setting MRIPinSpike and MRIPinHold and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPort,
        MRIPinSpike|MRIPinHold, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A1 for UpDown Mode and starting */
    MAP_Timer_A_configureUpDownMode(TIMER_A1_BASE, &upDownConfig);

    /* Initialize compare registers to generate PWM1 */
    MAP_Timer_A_initCompare(TIMER_A1_BASE, &compareConfig_PWM1);

    /* Initialize compare registers to generate PWM2 */
    MAP_Timer_A_initCompare(TIMER_A1_BASE, &compareConfig_PWM2);

    /* Starting Timer_A1 */
    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UPDOWN_MODE);
}

void MicroResistojetHandler::stopValve()
{
    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinSpike|MRIPinHold );
}
