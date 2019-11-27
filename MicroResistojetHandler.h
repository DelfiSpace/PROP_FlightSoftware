/*
 * MicroResistojetHandler.h
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#ifndef MICRORESISTOJETHANDLER_H_
#define MICRORESISTOJETHANDLER_H_

#include <driverlib.h>
#include "DSerial.h"

class MicroResistojetHandler
{
    /* Timer_A UpDown Configuration Parameter */
    Timer_A_UpDownModeConfig upDownConfig =
    {
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock SOurce
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/1 = 3MHz
        0,                                      // tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,    // Disable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
    };

    /* Timer_A Compare Configuration Parameter  (PWM1) */
    Timer_A_CompareModeConfig compareConfig_PWM1 =
    {
        TIMER_A_CAPTURECOMPARE_REGISTER_1,          // Use CCR1
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
        TIMER_A_OUTPUTMODE_TOGGLE_SET,              // Toggle output but
        0                                           // Duty Cycle
    };

    /* Timer_A Compare Configuration Parameter (PWM2) */
    Timer_A_CompareModeConfig compareConfig_PWM2 =
    {
        TIMER_A_CAPTURECOMPARE_REGISTER_2,          // Use CCR2
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
        TIMER_A_OUTPUTMODE_TOGGLE_SET,              // Toggle output but
        0                                           // Duty Cycle
    };

 protected:
     const unsigned long MRIPort;
     const unsigned long MRIPinHeat;
     const unsigned long MRIPinSpike;
     const unsigned long MRIPinHold;

     void startValve(uint_fast16_t timerPeriod, uint_fast16_t dutyCycle1, uint_fast16_t dutyCycle2);
     void stopValve();

 public:
     MicroResistojetHandler( const unsigned long port, const unsigned long pinHeat,
                             const unsigned long pinSpike, const unsigned long pinHold );
     void startMR(uint_fast16_t timerPeriod, uint_fast16_t dutyCycle1, uint_fast16_t dutyCycle2);
     void stopMR();
};

#endif /* MICRORESISTOJETHANDLER_H_ */
