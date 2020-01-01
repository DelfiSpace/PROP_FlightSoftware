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
     virtual bool process( PQ9Frame &command, PQ9Sender &interface, PQ9Frame &workingBbuffer );
};
#endif /* TESTSERVICE_H_ */
