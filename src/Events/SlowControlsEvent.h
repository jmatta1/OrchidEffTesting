/***************************************************************************//**
********************************************************************************
**
** @file SlowControlEvent.h
** @author James Till Matta
** @date 07 June, 2016
** @brief
**
** @copyright Copyright (C) 2016 James Till Matta
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** @details Holds the definition of the class that implements events from slow
** controls
**
********************************************************************************
*******************************************************************************/
#ifndef ORCHID_SRC_EVENTS_SLOWCONTROLEVENT_H
#define ORCHID_SRC_EVENTS_SLOWCONTROLEVENT_H
// includes for C system headers
// includes for C++ system headers
// includes from other libraries
// includes from ORCHID
#include"EventInterface.h"
#include"Hardware/HVLib/VoltageData.h"

namespace Events
{
typedef short ChanSizeType;

class SlowControlsEvent
{
public:
    SlowControlsEvent(int numVolChannels, int numTempChannels);
    ~SlowControlsEvent();
    
    //functions to retrieve the binary representation
    int getSizeOfBinaryRepresentation();
    //the buffer provided to this function will be at *least* the size
    //returned by getSizeOfBinaryRepresentation
    void getBinaryRepresentation(char* buff);
    
    void ReadVoltageData(const SlowControls::VoltageData& data);
    //void ReadTemperatureData()
private:
    
    //MPOD Information
    //Per Channel information
    int*    boardNumber;
    int*    channelNumber;
    float*  terminalVoltage;
    float*  senseVoltage;
    float*  setVoltage;
    float*  current;
    float*  rampUpRate;
    float*  rampDownRate;
    float*  maxCurrent;
    float*  maxVoltage;
    int*    currentTripTime;
    int*    temperature;
    int*    maxTemperature;
    bool*   outputSwitch;
    SlowControls::ChannelStatus*   channelStatus;
    SlowControls::CrateStatus      crateStatus;

    //Temperature Sensor Information

    
    //event timing
    unsigned long long int eventStartTime;
    unsigned long long int eventStopTime;
    
    //general information these do not need to be atomic as they are written
    //once at object creation and never again
    ChanSizeType numVoltageChannels;
    ChanSizeType numTemperatureChannels;
    short binarySize;
};

}

#endif //ORCHID_SRC_EVENTS_SLOWCONTROLEVENT_H
