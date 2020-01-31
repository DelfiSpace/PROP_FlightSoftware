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
#include "MicroResistojetHandler.h"

#include <driverlib.h>

#define PROPULSION_SERVICE            23 // check

class PropulsionService: public Service
{
    private:
        static PropulsionService * prop;

        unsigned int num_mr = 0;
        MicroResistojetHandler * mr[10];

        bool mr_active = false;
        uint_fast32_t mr_uptime = 0;

        HousekeepingService<PROPTelemetryContainer> hk;

        Task * task1, * task2;

        static void mrAction(const MicroResistojetHandler * mr);

        static void sendData();
        void sendThisData(PROPTelemetryContainer* tc, const uint_fast32_t globalTime,
                          bool binary = false);

        bool operatePropulsion(DataMessage &command);

    public:
        PropulsionService( const char * mrName[], const unsigned int mrConfig[], unsigned int count );
        ~PropulsionService();

        virtual bool process( DataMessage &command, DataMessage &workingBbuffer );

        Task * getTask1();
        Task * getTask2();
};
#endif /* PROPULSIONSERVICE_H_ */
