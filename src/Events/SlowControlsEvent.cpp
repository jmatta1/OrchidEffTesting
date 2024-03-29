/***************************************************************************//**
********************************************************************************
**
** @file SlowControlEvent.cpp
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
** @details Holds the definition of the interface superclass that all events
** being passed to the file IO thread must implement
**
********************************************************************************
*******************************************************************************/

#include"SlowControlsEvent.h"
// includes for C system headers
// includes for C++ system headers
#include<sstream>
#include<iomanip>
// includes from other libraries
// includes from ORCHID
#include"EventCodes.h"

namespace Events
{

SlowControlsEvent::SlowControlsEvent(int numVolChannels, int numTempChannels):
    numVoltageChannels(numVolChannels),
    numTemperatureChannels(numTempChannels), binarySize(0)
{
    this->boardNumber     = new int[numVolChannels];
    this->channelNumber   = new int[numVolChannels];
    this->terminalVoltage = new float[numVolChannels];
    this->senseVoltage    = new float[numVolChannels];
    this->setVoltage      = new float[numVolChannels];
    this->current         = new float[numVolChannels];
    this->rampUpRate      = new float[numVolChannels];
    this->rampDownRate    = new float[numVolChannels];
    this->maxCurrent      = new float[numVolChannels];
    this->maxVoltage      = new float[numVolChannels];
    this->currentTripTime = new int[numVolChannels];
    this->temperature     = new int[numVolChannels];
    this->maxTemperature  = new int[numVolChannels];
    this->outputSwitch    = new bool[numVolChannels];
    this->channelStatus   = new SlowControls::ChannelStatus[numVolChannels];
    //calculate the size of an event
    //size of the event, event ident, num voltage channels, num temp channels, and the current time
    int overhead = 2*sizeof(ChanSizeType) + 2*sizeof(short) + 2*sizeof(unsigned long long int);
    //2 shorts + 3 floats + 1 int + 1 unsigned int per vol channel
    int sizeOfSingleVoltageChannel = 2*sizeof(short) + 3*sizeof(float) +
                                     1*sizeof(int) + sizeof(unsigned int);
    //there are numVoltageChannels worth of this to store, plus the crate status
    //which fits into an int
    int sizeOfVoltage = sizeof(unsigned int) +
                        (this->numVoltageChannels*sizeOfSingleVoltageChannel);
    //no temperature data at the moment
    int sizeOfSingleTemperatureChannel = 0;
    int sizeOfTemperature = this->numTemperatureChannels*sizeOfSingleTemperatureChannel;
    binarySize = (overhead + sizeOfVoltage + sizeOfTemperature);
}

SlowControlsEvent::~SlowControlsEvent()
{
    delete[] this->terminalVoltage;
    delete[] this->senseVoltage;
    delete[] this->setVoltage;
    delete[] this->current;
    delete[] this->rampUpRate;
    delete[] this->rampDownRate;
    delete[] this->maxCurrent;
    delete[] this->maxVoltage;
    delete[] this->currentTripTime;
    delete[] this->temperature;
    delete[] this->maxTemperature;
    delete[] this->outputSwitch;
    delete[] this->channelStatus;
}


int SlowControlsEvent::getSizeOfBinaryRepresentation()
{
    return binarySize;
    
}

void SlowControlsEvent::getBinaryRepresentation(char* buff)
{
    //here we write, in binary the data in the event to the given buffer
    //lots of pointer tricks to come
    size_t index = 0;
    //first write the size of the event
    *(reinterpret_cast<short*>(&(buff[index]))) = this->binarySize;
    index += sizeof(short);
    //write the event type code
    *(reinterpret_cast<short*>(&(buff[index]))) = Codes::SlowControlsEventCode;
    index += sizeof(short);
    //write the time of reading start
    *(reinterpret_cast<short*>(&(buff[index]))) = this->eventStartTime;
    index += sizeof(unsigned long long int);
    //write the time of reading stop
    *(reinterpret_cast<short*>(&(buff[index]))) = this->eventStopTime;
    index += sizeof(unsigned long long int);
    //write the number of voltage channels
    *(reinterpret_cast<int*>(&(buff[index]))) = this->numVoltageChannels;
    index += sizeof(int);
    //write the number of temperature channels
    *(reinterpret_cast<int*>(&(buff[index]))) = this->numTemperatureChannels;
    index += sizeof(int);
    //now write the power crate information
    *(reinterpret_cast<unsigned int*>(&(buff[index]))) = this->crateStatus.giveIntRepresentation();
    index += sizeof(unsigned int);
    //now write the rest of the power information channel by channel
    for(int i =0; i<numVoltageChannels; ++i)
    {
        //board number
        *(reinterpret_cast<short*>(&(buff[index]))) = static_cast<short>(this->boardNumber[i]);
        index += sizeof(short);
        //channel number
        *(reinterpret_cast<short*>(&(buff[index]))) = static_cast<short>(this->channelNumber[i]);
        index += sizeof(short);
        //channel terminal voltage
        *(reinterpret_cast<float*>(&(buff[index]))) = this->terminalVoltage[i];
        index += sizeof(float);
        //channel sense voltage
        *(reinterpret_cast<float*>(&(buff[index]))) = this->senseVoltage[i];
        index += sizeof(float);
        //channel currents
        *(reinterpret_cast<float*>(&(buff[index]))) = this->current[i];
        index += sizeof(float);
        //channel temperatures
        *(reinterpret_cast<int*>(&(buff[index])))   = this->temperature[i];
        index += sizeof(int);
        //channel status
        *(reinterpret_cast<unsigned int*>(&(buff[index]))) = this->channelStatus[i].giveIntRepresentation();
        index += sizeof(unsigned int);
    }
    //now write the the temperature information
}


void SlowControlsEvent::ReadVoltageData(const SlowControls::VoltageData& data)
{
    //set the time data
    this->eventStartTime = data.beginRead;
    this->eventStopTime = data.finishRead;
    //set up the loop for transfering per channel information
    std::size_t i=0, j=0;
    std::size_t dataSize = data.terminalVoltage.size();
    //loop and transfer per channel information
    while((i < dataSize) && (j < numVoltageChannels))
    {
        if(data.outputSwitch[i])
        {
            this->boardNumber[j]        = data.boardNumber[i];
            this->channelNumber[j]      = data.channelNumber[i];
            this->terminalVoltage[j]    = data.terminalVoltage[i];
            this->senseVoltage[j]       = data.senseVoltage[i];
            this->setVoltage[j]         = data.setVoltage[i];
            this->current[j]            = data.current[i];
            this->rampUpRate[j]         = data.rampUpRate[i];
            this->rampDownRate[j]       = data.rampDownRate[i];
            this->maxCurrent[j]         = data.maxCurrent[i];
            this->maxVoltage[j]         = data.maxVoltage[i];
            this->currentTripTime[j]    = data.currentTripTime[i];
            this->temperature[j]        = data.temperature[i];
            this->maxTemperature[j]     = data.maxTemperature[i];
            this->outputSwitch[j]       = data.outputSwitch[i];
            this->channelStatus[j]      = data.channelStatus[i];
            ++j;
        }
        ++i;
    }
    //now transfer crate information
    this->crateStatus = data.crateStatus;
}

//void SlowControlsEvent::ReadTemperatureData(const SlowControls::TemperatureData& tempData)
}
