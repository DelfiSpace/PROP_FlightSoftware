/*
 * TestService.h
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#ifndef TESTSERVICE_H_
#define TESTSERVICE_H_

#include "Service.h"
#include "DSerial.h"
#include <driverlib.h>

class TestService: public Service
{
 public:
     virtual bool process( DataFrame &command, DataBus &interface, DataFrame &workingBbuffer );
};
#endif /* TESTSERVICE_H_ */
