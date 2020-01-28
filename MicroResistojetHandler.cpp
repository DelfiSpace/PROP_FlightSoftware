/*
 * MicroResistojetHandler.cpp
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#include <MicroResistojetHandler.h>

extern DSerial serial;
static MicroResistojetHandler * activeMR = nullptr;

MicroResistojetHandler::MicroResistojetHandler(const unsigned long MRport, const unsigned long MRpinHeat,
                                               const unsigned long MRpinSpike, const unsigned long MRpinHold) :
        MRIPort(MRport), MRIPinHeat(MRpinHeat),
        MRIPinSpike(MRpinSpike), MRIPinHold(MRpinHold)  {
    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
}

void MicroResistojetHandler::init()
{
    activeMR = nullptr;
}

void MicroResistojetHandler::startMR(uint_fast16_t time_work, uint_fast16_t delayBeforeValve,
    uint_fast16_t timerPeriod, uint_fast16_t time_hold, uint_fast16_t time_spike)
{
    stopActiveMR(nullptr);

    activeMR = this;

    MAP_GPIO_setOutputHighOnPin( MRIPort, MRIPinHeat );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat );

    /*delayBeforeValve = (((uint_fast64_t)delayBeforeValve)*((uint_fast64_t)MAP_CS_getMCLK()))/48000000;

    for (uint_fast16_t i = 0; i < delayBeforeValve; ++i)
        __delay_cycles((48000 - 4)/2);*/

    startValve(timerPeriod, time_hold, time_spike);
}

void MicroResistojetHandler::stopMR()
{
    TA0CCTL4 = TIMER_A_OUTPUTMODE_RESET;
    TA0CCTL3 = TIMER_A_OUTPUTMODE_RESET;
    TA0CCTL2 = TIMER_A_OUTPUTMODE_RESET;
    MAP_Timer_A_clearInterruptFlag(TIMER_A0_BASE);

    while(!(TA0CTL & 1));

    MAP_GPIO_setOutputLowOnPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );
    MAP_GPIO_setAsOutputPin( MRIPort, MRIPinHeat|MRIPinSpike|MRIPinHold );

    activeMR = nullptr;
}

int MicroResistojetHandler::stopActiveMR(const MicroResistojetHandler *isThis)
{
    if (activeMR == nullptr)
        return -1;

    int ret = (activeMR == isThis);
    activeMR->stopMR();
    return ret;
}

void MicroResistojetHandler::startValve(uint_fast16_t timerPeriod,
    uint_fast16_t time_hold, uint_fast16_t time_spike)
{
    upConfig.timerPeriod = (uint_fast16_t)((((uint_fast64_t)timerPeriod) << 12)/125);
    compareConfig_PWMHold.compareValue = (uint_fast16_t)((((uint_fast64_t)time_hold) << 12)/125);
    compareConfig_PWMSpike.compareValue = (uint_fast16_t)((((uint_fast64_t)time_spike) << 9)/15625);

    /* Setting ACLK to REFO at 32Khz */
    MAP_CS_setReferenceOscillatorFrequency(CS_REFO_32KHZ);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    /* Setting MRIPinSpike and MRIPinHold and peripheral outputs for CCR */
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(MRIPort,
        MRIPinSpike|MRIPinHold, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring Timer_A1 for UpDown Mode and starting */
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &upConfig);

    /* Initialize compare registers to generate PWMHold */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMHold);

    /* Initialize compare registers to generate PWMSpike */
    MAP_Timer_A_initCompare(TIMER_A0_BASE, &compareConfig_PWMSpike);

    /* Starting Timer_A0 */
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}
