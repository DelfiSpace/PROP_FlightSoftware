/*
 * TestService.cpp
 *
 *  Created on: 14 Nov 2019
 *      Author: guillemrueda
 */

#include "TestService.h"

bool TestService::process(DataMessage &command, DataMessage &workingBuffer)
{
    if (command.getPayload()[0] == TEST_SERVICE)
    {
        workingBuffer.setSize(2);
        workingBuffer.getPayload()[0] = TEST_SERVICE;
        workingBuffer.getPayload()[1] = SERVICE_RESPONSE_ERROR;
        return true;
    }
    return false;
}


