/*
 * PropulsionService.h
 *
 *  Created on: 31 Jan 2020
 *      Author: guillemrueda
 */

#ifndef PROPULSIONSERVICE_H_
#define PROPULSIONSERVICE_H_

#include "PROP.h"
#include "Service.h"
#include "Task.h"
#include "PropTelemetryContainer.h"
#include "MicroResistojetHandler.h"

#include <driverlib.h>

#define PROPULSION_SERVICE            23 // check

class PropulsionService: public Service
{
    private:
        static PropulsionService * prop;

        unsigned int num_mr = 0;
        unsigned int pos_fram = 0, last_start_pos_fram = 0;
        MicroResistojetHandler * mr[10];

        volatile int mr_active = 0;
        volatile uint_fast32_t mr_uptime = 0;
        volatile bool executingTask = false;

        volatile enum MicroResistojetHandler::status_t reason;

        volatile uint_fast32_t myGlobalTime;

        unsigned int num_saved_data = 0;

        HousekeepingService<PROPTelemetryContainer> hk;

        Task * task1, * task2;

        static void mrAction(const MicroResistojetHandler * mr);

        static void saveData();

        static void sendThisData(const uint_fast32_t globalTime, PROPTelemetryContainer* tc);

        static void sendSavedDataNow();

        inline static void sendCount(const unsigned int num_saved_data);

        static void sendStart(const uint_fast32_t globalTime, const char * name,
                              const struct MicroResistojetHandler::params_t * params);

        static void sendStop(const uint_fast32_t globalTime,
                             const enum MicroResistojetHandler::status_t reason,
                             bool ok = true);

        static void sendUnfinished();

        static unsigned int countSavedData();

        static uint_fast32_t getGlobalTime();

        static bool startMR(MicroResistojetHandler * myMR, const unsigned char * config);

        void sendCount();

        void propStart(const MicroResistojetHandler * mr);

        uint_fast32_t recordStop(bool ok = true);

        void saveByte(unsigned char byte);

        uint_fast32_t saveGlobalTime(bool now = true);

        void saveThisData(PROPTelemetryContainer* tc);

        void sendSavedData();

        void eraseSavedData();

        bool operatePropulsion(const unsigned char request, const unsigned char action,
                               const unsigned char * payload);

    public:
        PropulsionService( const char * mrName[], const unsigned int mrConfig[], unsigned int count );
        ~PropulsionService();

        virtual bool process( DataMessage &command, DataMessage &workingBbuffer );

        Task * getTask1();
        Task * getTask2();
};
#endif /* PROPULSIONSERVICE_H_ */
