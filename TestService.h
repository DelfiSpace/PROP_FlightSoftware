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

#define TEST_SERVICE            20 // check
#define TEST_ERROR               0
#define TEST_REQUEST             1
#define TEST_RESPONSE            2

class TestService: public Service
{
     bool operatePropulsion(DataFrame &command);
 public:
     virtual bool process( DataMessage &command, DataMessage &workingBbuffer );
};
#endif /* TESTSERVICE_H_ */
