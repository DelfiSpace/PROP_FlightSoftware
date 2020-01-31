/*
 * MicroResistojetHandler.h
 *
 *  Created on: 21 Nov 2019
 *      Author: guillemrueda
 */

#ifndef MICRORESISTOJETHANDLER_H_
#define MICRORESISTOJETHANDLER_H_

#include <driverlib.h>

class MicroResistojetHandler
{
    public:
        struct params_t {
            uint_fast8_t  time_work;
            uint_fast8_t  time_before;
            uint_fast8_t  duty_c_heat;
            uint_fast16_t timerPeriod;
            uint_fast16_t time_hold;
            uint_fast16_t time_spike;
        };

        struct config_t {
            unsigned long portHeat;
            unsigned long pinHeat;

            unsigned long portValve;
            unsigned long pinSpike;
            unsigned long pinHold;

            uint32_t timerOutput;
        };

        enum status_t {
            UNUSABLE,
            NEVER_STARTED,
            ON_HEAT_ONLY,
            ON_HEAT_AND_VALVE,
            STOP_OBJECT_DESTROYED,
            STOP_TIMEOUT,
            STOP_MANUAL,
            STOP_RECONFIGURING,
            STOP_START_DIFFERENT
        };

        MicroResistojetHandler( const char * name, const unsigned int configId,
                                void (*const userFunction)( const MicroResistojetHandler * ) = nullptr );

        MicroResistojetHandler( const char * name, const unsigned int configId, const uint32_t timerTime,
                                void (*const userFunction)( const MicroResistojetHandler * ) = nullptr );

        ~MicroResistojetHandler();

        const char * getName() const;
        enum status_t getCurrentStatus() const;
        struct params_t getCurrentParams() const;

        bool startMR(struct params_t c);
        static bool stopActiveMR();

        static const struct config_t configs[];
        static const unsigned int num_configs;

    protected:
        const char * MRIName;
        const unsigned int MRIConfigId;

        void (*const MRIUserFunction)( const MicroResistojetHandler * );

    private:
        /* Timer_A UpMode Configuration Parameter */
        Timer_A_UpModeConfig upConfig =
        {
            TIMER_A_CLOCKSOURCE_ACLK,               // ACLK Clock Source
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
            TIMER_A_OUTPUTMODE_OUTBITVALUE,             // Off (then Toggle output but)
            0                                           // Duty Cycle
        };

        /* Timer_A Compare Configuration Parameter  (PWMSpike) */
        Timer_A_CompareModeConfig compareConfig_PWMSpike =
        {
            TIMER_A_CAPTURECOMPARE_REGISTER_3,          // Use CCR3
            TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
            TIMER_A_OUTPUTMODE_OUTBITVALUE,             // Off (then Toggle output but)
            0                                           // Duty Cycle
        };

        /* Timer_A Compare Configuration Parameter  (PWMHeat) */
        Timer_A_CompareModeConfig compareConfig_PWMHeat =
        {
            TIMER_A_CAPTURECOMPARE_REGISTER_2,          // Use CCR2
            TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
            TIMER_A_OUTPUTMODE_RESET_SET,               // Toggle output but
            0                                           // Duty Cycle
        };

        /* Timer_A UpMode Configuration Parameter */
        Timer_A_UpModeConfig upConfigCounter =
        {
            TIMER_A_CLOCKSOURCE_ACLK,               // ACLK Clock Source
            TIMER_A_CLOCKSOURCE_DIVIDER_64,         // ACLK / 64
            0,                                      // tick period
            TIMER_A_TAIE_INTERRUPT_ENABLE,          // Enable Timer interrupt
            TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,    // Disable CCR0 interrupt
            TIMER_A_DO_CLEAR                        // Clear value
        };

        unsigned long MRIPortHeat;
        unsigned long MRIPinHeat;

        unsigned long MRIPortValve;
        unsigned long MRIPinSpike;
        unsigned long MRIPinHold;

        uint32_t MRITimerOutput;
        uint32_t MRITimerTime;

        struct params_t currentParams = { };
        enum status_t currentStatus = UNUSABLE;

        bool stopMR(enum status_t stopReason);

        static volatile MicroResistojetHandler * activeMR;

        static void _handler_stop_active_MR();
        static void _handler_after_delay();

        static bool _stopActiveMR(enum status_t stopReason);

        inline void setUp( const unsigned long portHeat, const unsigned long pinHeat,
                    const unsigned long portValve, const unsigned long pinSpike, const unsigned long pinHold,
                    const uint32_t timerOutput );
};

#endif /* MICRORESISTOJETHANDLER_H_ */
