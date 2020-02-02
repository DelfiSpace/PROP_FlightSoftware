#include "PROP.h"

#define NUM_ELEM(x) (sizeof(x)/sizeof(x[0]))

// I2C busses
DWire I2Cinternal(0);

// SPI bus
DSPI spi(3);
MB85RS fram(spi, GPIO_PORT_P1, GPIO_PIN0);

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
HousekeepingService<PROPTelemetryContainer> hk;
SoftwareUpdateService SWUpdate;
TestService test;
const char * names[] = {"LPM", "VLM"};
const unsigned int configs[] = {0, 1};
PropulsionService prop(names, configs, NUM_ELEM(names));
Service* services[] = { &ping, &reset, &hk, &SWUpdate, &test, &prop };

// PROP board tasks
CommandHandler<PQ9Frame> cmdHandler(pq9bus, services, NUM_ELEM(services));
Task timerTask(periodicTask);
Task* periodicTasks[] = {&timerTask};
PeriodicTaskNotifier periodicNotifier = PeriodicTaskNotifier(FCLOCK, periodicTasks, NUM_ELEM(periodicTasks));
Task* tasks[] = { &cmdHandler, &timerTask, prop.getTask1(), prop.getTask2() };

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

    // Initialize SPI master
    spi.initMaster(DSPI::MODE0, DSPI::MSBFirst, 1000000);
    fram.init();
    fram.erase();

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

    TaskManager::start(tasks, NUM_ELEM(tasks));
}
