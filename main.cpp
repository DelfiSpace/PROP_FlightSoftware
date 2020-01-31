
#include "PROP.h"

// I2C busses
DWire I2Cinternal(0);

// Micro-resistojet handlers
MicroResistojetHandler mr[] =
{
     { "LPM", 0, TIMER_A1_BASE, mrAction },
     { "VLM", 1, TIMER_A0_BASE, mrAction }
};

unsigned int num_mr = sizeof(mr)/sizeof(MicroResistojetHandler);

// voltage / current sensors
INA226 powerBus(I2Cinternal, 0x40);
INA226 valveHold(I2Cinternal, 0x41);
INA226 valveSpike(I2Cinternal, 0x42);
INA226 heaters(I2Cinternal, 0x43);

// temperature sensor
TMP100 temp(I2Cinternal, 0x48);

// CDHS bus handler
PQ9Bus pq9bus(3, GPIO_PORT_P9, GPIO_PIN0);

// debug console handler
DSerial serial;

// services running in the system
PingService ping;
ResetService reset( GPIO_PORT_P4, GPIO_PIN0 );
HousekeepingService<PROPTelemetryContainer> hk, hk2;
SoftwareUpdateService SWUpdate;
TestService test;
Service* services[] = { &ping, &reset, &hk, &SWUpdate, &test };

// PROP board tasks
CommandHandler<PQ9Frame> cmdHandler(pq9bus, services, 5);
Task timerTask(periodicTask);
Task* periodicTasks[] = {&timerTask};
PeriodicTaskNotifier periodicNotifier = PeriodicTaskNotifier(FCLOCK, periodicTasks, 1);
Task sendSensorsData(sendData);
Task notifySendSensorsData(notifySendData);
Task* tasks[] = { &cmdHandler, &timerTask, &sendSensorsData, &notifySendSensorsData };

bool mr_active = false;
uint_fast32_t mr_uptime = 0;

// system uptime
unsigned long uptime = 0;

// TODO: remove when bug in CCS has been solved
void kickWatchdog(DataFrame &newFrame)
{
    cmdHandler.received(newFrame);
}

void validCmd(void)
{
    reset.kickInternalWatchDog();
}

void mrAction(const MicroResistojetHandler * mr)
{
    switch (mr->getCurrentStatus()) {
        case MicroResistojetHandler::status_t::UNUSABLE:
        case MicroResistojetHandler::status_t::NEVER_STARTED:
            break;

        case MicroResistojetHandler::status_t::ON_HEAT_ONLY:
        case MicroResistojetHandler::status_t::ON_HEAT_AND_VALVE:
            if (!mr_active) {
                mr_active = true;
                sendSensorsData.notify();
            }
            break;

        default: // STOP
            mr_active = false;
            mr_uptime = 0;
            break;
    }
}

void sendThisData(PROPTelemetryContainer* tc, const uint_fast32_t globalTime, bool binary = false)
{
    if (binary)
    {
        serial.print(((unsigned char *)&globalTime)[3]);
        serial.print(((unsigned char *)&globalTime)[2]);
        serial.print(((unsigned char *)&globalTime)[1]);
        serial.print(((unsigned char *)&globalTime)[0]);

        unsigned char * telemetry = tc->getArray();

        for (int i = 0; i < tc->size(); ++i, ++telemetry)
        {
            serial.print(*telemetry);
        }

        return;
    }

    int_fast16_t i;

    serial.print(tc->getUpTime(), DEC); serial.print(',');

    serial.print(globalTime, DEC); serial.print(',');

    serial.print(tc->getBusStatus(),  DEC); serial.print(',');
    serial.print(tc->getBusVoltage(), DEC); serial.print(',');
    i = tc->getBusCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getValveHoldStatus(),  DEC); serial.print(',');
    serial.print(tc->getValveHoldVoltage(), DEC); serial.print(',');
    i = tc->getValveHoldCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getValveSpikeStatus(),  DEC); serial.print(',');
    serial.print(tc->getValveSpikeVoltage(), DEC); serial.print(',');
    i = tc->getValveSpikeCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getHeatersStatus(),  DEC); serial.print(',');
    serial.print(tc->getHeatersVoltage(), DEC); serial.print(',');
    i = tc->getHeatersCurrent();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.print(i, DEC); serial.print(',');

    serial.print(tc->getTmpStatus(), DEC); serial.print(',');
    i = tc->getTemperature();
    if (i < 0)
    {
        serial.print('-');
        i = -i;
    }
    serial.println(i, DEC);
}

void sendData()
{
    if (mr_active)
    {
        notifySendSensorsData.notify();
        hk2.acquireTelemetry(acquireTelemetry);
        PROPTelemetryContainer* tc = hk2.getTelemetry();
        tc->setUpTime(++mr_uptime);
        sendThisData(tc, 0); // TODO: Send global time
    }
}

void notifySendData()
{
    if (mr_active)
        sendSensorsData.notify();
}

void periodicTask()
{
    // increase the timer, this happens every second
    uptime++;

    // collect telemetry
    hk.acquireTelemetry(acquireTelemetry);

    // refresh the watch-dog configuration to make sure that, even in case of internal
    // registers corruption, the watch-dog is capable of recovering from an error
    reset.refreshConfiguration();

    // kick hardware watch-dog after every telemetry collection happens
    reset.kickExternalWatchDog();
}

void acquireTelemetry(PROPTelemetryContainer *tc)
{
    unsigned short v;
    signed short i, t;

    // set uptime in telemetry
    tc->setUpTime(uptime);

    // measure the board
    tc->setBusStatus((!powerBus.getVoltage(v)) & (!powerBus.getCurrent(i)));
    tc->setBusVoltage(v);
    tc->setBusCurrent(i);

    // measure the valve's hold
    tc->setValveHoldStatus((!valveHold.getVoltage(v)) & (!valveHold.getCurrent(i)));
    tc->setValveHoldVoltage(v);
    tc->setValveHoldCurrent(i);

    // measure the valve's spike
    tc->setValveSpikeStatus((!valveSpike.getVoltage(v)) & (!valveSpike.getCurrent(i)));
    tc->setValveSpikeVoltage(v);
    tc->setValveSpikeCurrent(i);

    // measure the heaters
    tc->setHeatersStatus((!heaters.getVoltage(v)) & (!heaters.getCurrent(i)));
    tc->setHeatersVoltage(v);
    tc->setHeatersCurrent(i);

    // acquire board temperature
    tc->setTmpStatus(!temp.getTemperature(t));
    tc->setTemperature(t);
}

/**
 * main.c
 */
void main(void)
{
    // initialize the MCU:
    // - clock source
    // - clock tree
    DelfiPQcore::initMCU();

    // Initialize I2C master
    I2Cinternal.setFastMode();
    I2Cinternal.begin();

    // initialize the shunt resistor
    powerBus.setShuntResistor(40);
    valveHold.setShuntResistor(40);
    valveSpike.setShuntResistor(40);
    heaters.setShuntResistor(40);

    temp.init();

    serial.begin( );                        // baud rate: 9600 bps
    pq9bus.begin(115200, PROP_ADDRESS);     // baud rate: 115200 bps
                                            // address PROP (6)

    // initialize the reset handler:
    // - prepare the watch-dog
    // - initialize the pins for the hardware watch-dog
    // - prepare the pin for power cycling the system
    reset.init();

    // link the command handler to the PQ9 bus:
    // every time a new command is received, it will be forwarded to the command handler
    // TODO: put back the lambda function after bug in CCS has been fixed
    //pq9bus.setReceiveHandler([](PQ9Frame &newFrame){ cmdHandler.received(newFrame); });
    pq9bus.setReceiveHandler(&kickWatchdog);

    // every time a command is correctly processed, call the watch-dog
    // TODO: put back the lambda function after bug in CCS has been fixed
    //cmdHandler.onValidCommand([]{ reset.kickInternalWatchDog(); });
    cmdHandler.onValidCommand(&validCmd);

    serial.println("PROP booting...");

    TaskManager::start(tasks, 4);
}
