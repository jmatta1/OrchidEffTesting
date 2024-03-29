/***************************************************************************//**
********************************************************************************
**
** @file SlowControlsThread.h
** @author James Till Matta
** @date 01 June, 2016
** @brief
**
** @copyright Copyright (C) 2016 James Till Matta
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** @details Holds the definition of the slow controls thread class used to poll
** the slow controls like voltage and temperature
**
********************************************************************************
*******************************************************************************/
#ifndef ORCHID_SRC_THREADS_SLOWCONTROLSTHREAD_H
#define ORCHID_SRC_THREADS_SLOWCONTROLSTHREAD_H

// includes for C system headers
// includes for C++ system headers
#include<string>
// includes from other libraries
#include<boost/chrono.hpp>
// includes from ORCHID
#include"InterThreadComm/Control/SlowControlsThreadControl.h"
#include"Hardware/HVLib/MpodReader.h"
#include"InterThreadComm/Data/FileData.h"
#include"InterThreadComm/Control/OutputControl.h"
#include"InterThreadComm/Data/SlowData.h"
#include"Utility/CommonTypeDefs.h"
#include"Events/SlowControlsEvent.h"
#include"IO/SecantFileWriter.h"

namespace Threads
{

class SlowControlsThread
{
public:
    //construction and destruction
    SlowControlsThread(SlowControls::MpodReader* mRead, InterThread::SlowData* slDat,
                       InterThread::SlowControlsThreadControl* sctCtrl,
                       int refreshRate, InterThread::OutputControl* outCtrl, 
                       InterThread::FileData* fileDat, int thrdNum,
                       Utility::LoggerType& log, const std::string& baseOutputDirectory);
    ~SlowControlsThread(){delete[] eventBuffer;}
    
    // the function that makes this object callable which actually executes the thread
    void operator() ();
private:
    void publishInternalData()
    {
        slowData->readVoltageData(this->mpodReader->voltageData);
    }

    void writeScEventToFile()
    {
        //put the data into the event
        scEvent.ReadVoltageData(this->mpodReader->voltageData);
        //put the event into a write buffer
        scEvent.getBinaryRepresentation(this->eventBuffer);
        //put the event buffer into the file
        outputFile.writeEvent(this->eventBuffer, this->eventSize);
    }
    
    void doWritingLoop();
    void doPollingLoop();
    
    //the controller for this thread
    InterThread::SlowControlsThreadControl* sctControl;
    //to maintain the event
    bool notTerminated;
    //time to pause the loop between reads
    boost::chrono::nanoseconds refreshPeriod;
    
    //the slow controls data
    SlowControls::MpodReader* mpodReader;
    InterThread::SlowData* slowData;
    
    //stuff for writing to file
    Events::SlowControlsEvent scEvent;
    char* eventBuffer;
    int eventSize;
    InterThread::OutputControl* outputCtrl;
    
    //the actual output file
    IO::SecantFileWriter outputFile;
    
    //logger
    Utility::LoggerType& lg;
};

}

#endif //ORCHID_SRC_THREADS_SLOWCONTROLSTHREAD_H
