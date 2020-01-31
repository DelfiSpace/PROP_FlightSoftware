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

#define TEST_SERVICE            0 // check

class TestService: public Service
{
 public:
     virtual bool process( DataMessage &command, DataMessage &workingBbuffer );
};
#endif /* TESTSERVICE_H_ */
