/***************************************************************************//**
********************************************************************************
**
** @file Digitizer.h
** @author James Till Matta
** @date 27 June, 2016
** @brief
**
** @copyright Copyright (C) 2016 James Till Matta
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** @details Definition file for the Digitizer class
**
********************************************************************************
*******************************************************************************/
#ifndef ORCHID_SRC_HARDWARE_DIGITIZER_DIGITIZER_H
#define ORCHID_SRC_HARDWARE_DIGITIZER_DIGITIZER_H
// includes for C system headers
// includes for C++ system headers
// includes from other libraries
#include<CAENComm.h>
// includes from ORCHID
#include"InputLib/Blocks/DigitizerModuleData.h"
#include"InputLib/Blocks/DigitizerChannelData.h"
#include"Utility/OrchidLogger.h"
#include"Vx1730DigitizerRegisters.h"

namespace Digitizer
{
//Todo: add some mechanism to reduce the number of individual channel reads
//  while acquiring data, maybe use the number of aggregates to trigger an IRQ
//  as a bare minimum to wait for?
class Vx1730Digitizer
{
public:
    Vx1730Digitizer(int modNum, InputParser::DigitizerModuleData* modData,
                    InputParser::DigitizerChannelData* chanData);
    ~Vx1730Digitizer();
    
    //this takes a little while as the system has to not only set all the
    //parameters, it then needs to wait a little while, run the calibration,
    //and then wait a little more time
    void setupDigitizer();
    
    //puts the digitizer in running mode
    void startAcquisition();
    //takes the digitizer out of running mode
    void stopAcquisition();
    
    //checks if there is an event ready, if yes it grabs the event, if false
    //it waits on an interrupt from the digitizer
    unsigned int getData(unsigned int* buffer);
    
    //is used after a forcible data flush at the end of acquisition
    unsigned int performFinalReadout(unsigned int* buffer);
    
    //give the max possible size of a buffer in bytes so that they can be pre-
    //allocated for the queueing system
    int getSizeOfReadBufferIn32BitInts(){return maxSizeOfBoardAggregateBlock;}
    
    //return the channel 0 index
    int getModuleStartChannel(){return channelStartInd;}
    //return the channel 0 index
    int getModuleNumber(){return moduleNumber;}
private:
    void writeErrorAndThrow(CAENComm_ErrorCode errVal);

    //waits for the digitizer to raise and interrupt about data full.
    //once an interrupt is raised, it reads the event size value to find out
    //how much it needs to read, then it iteratively reads data that data into
    //the given buffer
    unsigned int waitForInterruptToReadData(unsigned int* buffer);
    unsigned int readImpromptuDataAvailable(unsigned int* buffer);
    unsigned int readInterruptDataAvailable(unsigned int* buffer);
    
    unsigned int readEvent(unsigned int* buffer);
    bool isEventReady()
    {
        using LowLvl::Vx1730ReadRegisters;
        using LowLvl::Vx1730CommonReadRegistersAddr;
        CAENComm_ErrorCode readError;
        unsigned int readValue = 0;
        readError = CAENComm_Read32(digitizerHandle, Vx1730CommonReadRegistersAddr<Vx1730ReadRegisters::ReadoutStatus>::value, &readValue);
        if(readError < 0)
        {
            BOOST_LOG_SEV(lg, Error) << "ACQ Thread: Error Reading Event Ready Status From Digitizer #" << moduleNumber;
            this->writeErrorAndThrow(readError);
        }
        return ((readValue & 0x00000001)!=0);
    }
    
    void forceDataFlush()
    {
        using LowLvl::Vx1730WriteRegisters;
        using LowLvl::Vx1730IbcastWriteRegistersAddr;
        //hit the software force data flush for a data flush
        CAENComm_ErrorCode readError;
        readError = CAENComm_Write32(digitizerHandle, Vx1730IbcastWriteRegistersAddr<Vx1730WriteRegisters::ForcedDataFlush>::value, 0x00000001);
        if(readError < 0)
        {
            BOOST_LOG_SEV(lg, Error) << "ACQ Thread: Error Forcing data flush for digitizer #" << moduleNumber;
            this->writeErrorAndThrow(readError);
        }
    }
    
    void writeCommonRegisterData();
    void writeGroupRegisterData();
    void writeIndividualRegisterData();
    void calculateMaximumSizes();
    
    //wait for SPI flags to clear
    void openDigitizer();
    void clearResetAndSync();
    void calibrateDigitizer();
    void waitForChanSpiIdle(unsigned chNum);
    void waitForChanCal(unsigned chNum);
    
    //functions to calculate register values for common registers
    unsigned int calculateBoardConfigRegVal();
    void calculateAcqCtrlRegBase();
    unsigned int calculateGlblTrigMaskRegVal();
    unsigned int calculateTrigOutEnableMaskRegVal();
    unsigned int calculateChanEnMaskRegVal();
    unsigned int calculateLocalTrgManagementRegVal(int ind);
    unsigned int calculateTriggerValidationMask(int ind);
    
    //functions to calculate register values for individual registers
    unsigned int calculateCfdRegSettings(int i);
    unsigned int calculateDppAlgCtrlRegVal(int i);
    
    int moduleNumber;
    int channelStartInd;
    int numChannel;
    int digitizerHandle;
    int eventsPerInterrupt;
    bool acqRunning;
    bool digitizerOpen;
    InputParser::DigitizerModuleData* moduleData;
    InputParser::DigitizerChannelData* channelData;
    //arrays to handle multireads and multi writes
    unsigned int* addrArray;
    unsigned int* dataArray;
    unsigned int* rdbkArray;
    CAENComm_ErrorCode* cycleErrsArray;
    int arraySize;
    //variables to hold sizes of parts of the readout (in 32 bit ints
    int sizePerEvent[8];
    int sizePerChanPairAggregate[8];
    int maxSizeOfBoardAggregate;
    int maxSizeOfBoardAggregateBlock;
    int maxBufferFillForAnotherRead;
    //variables to hold persistent values for use later
    unsigned int acquisitionCtrlRegBase;
    
    boost::log::sources::severity_logger_mt<LogSeverity>& lg;
    
};

}

#endif //ORCHID_SRC_HARDWARE_DIGITIZER_DIGITIZER_H
