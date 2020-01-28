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
    Timer_A_UpModeConfig upConfig =
    {
        TIMER_A_CLOCKSOURCE_ACLK,               // ACLK Clock SOurce
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // ACLK
        0,                                      // tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,    // Disable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
    };

    /* Timer_A Compare Configuration Parameter (PWMHold) */
    Timer_A_CompareModeConfig compareConfig_PWMHold =
    {
        TIMER_A_CAPTURECOMPARE_REGISTER_4,          // Use CCR4
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
        TIMER_A_OUTPUTMODE_RESET_SET,               // Toggle output but
        0                                           // Duty Cycle
    };

    /* Timer_A Compare Configuration Parameter  (PWMSpike) */
    Timer_A_CompareModeConfig compareConfig_PWMSpike =
    {
        TIMER_A_CAPTURECOMPARE_REGISTER_3,          // Use CCR3
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
        TIMER_A_OUTPUTMODE_RESET_SET,               // Toggle output but
        0                                           // Duty Cycle
    };

    void stopMR();

 protected:
     const unsigned long MRIPort;
     const unsigned long MRIPinHeat;
     const unsigned long MRIPinSpike;
     const unsigned long MRIPinHold;

     void startValve(uint_fast16_t timerPeriod, uint_fast16_t time_hold, uint_fast16_t time_spike);

 public:
     MicroResistojetHandler( const unsigned long port, const unsigned long pinHeat,
                             const unsigned long pinSpike, const unsigned long pinHold );
     void startMR(uint_fast16_t time_work, uint_fast16_t delayBeforeValve, uint_fast16_t timerPeriod, uint_fast16_t time_hold, uint_fast16_t time_spike);
     static int stopActiveMR(const MicroResistojetHandler *isThis);
     static void init();
};

#endif /* MICRORESISTOJETHANDLER_H_ */
