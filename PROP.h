/*
 * PROP.h
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#ifndef PROP_H_
#define PROP_H_

#include <driverlib.h>
#include "msp.h"
#include "DelfiPQcore.h"
#include "PQ9Bus.h"
#include "PQ9Frame.h"
#include "DWire.h"
#include "INA226.h"
#include "DSerial.h"
#include "CommandHandler.h"
#include "PingService.h"
#include "ResetService.h"
#include "Task.h"
#include "PeriodicTask.h"
#include "TaskManager.h"
#include "HousekeepingService.h"
#include "PROPTelemetryContainer.h"
#include "TMP100.h"
#include "Service.h"
#include "SoftwareUpdateService.h"
#include "TestService.h"
#include "MicroResistojetHandler.h"
#include "PeriodicTaskNotifier.h"

#define FCLOCK 48000000

#define PROP_ADDRESS     6

// callback functions
void acquireTelemetry(PROPTelemetryContainer *tc);
void periodicTask();

#endif /* PROP_H_ */
