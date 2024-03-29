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
** @details Holds the definition of the slow controls class used to poll the
** voltage and temperature controls
**
********************************************************************************
*******************************************************************************/
#include"SlowControlsThread.h"
// includes for C system headers
// includes for C++ system headers
// includes from other libraries
#include<boost/thread.hpp>
// includes from ORCHID
#include"Events/SlowControlsEvent.h"

namespace Threads
{
SlowControlsThread::SlowControlsThread(SlowControls::MpodReader* mRead,
                                       InterThread::SlowData* slDat,
                                       InterThread::SlowControlsThreadControl* sctCtrl,
                                       int refreshRate, InterThread::OutputControl* outCtrl,
                                       InterThread::FileData* fileDat,
                                       int thrdNum, Utility::LoggerType& log,
                                       const std::string& baseOutputDirectory):
    sctControl(sctCtrl), notTerminated(true), mpodReader(mRead), slowData(slDat),
    scEvent(slDat->numVoltageChannels, slDat->numTemperatureChannels), 
    outputCtrl(outCtrl), outputFile(fileDat, log, thrdNum, baseOutputDirectory),
    lg(log)
{
    long long int refreshNanoseconds = (1000000000/refreshRate);
    this->refreshPeriod = boost::chrono::nanoseconds(refreshNanoseconds);
    this->eventSize = scEvent.getSizeOfBinaryRepresentation();
    this->eventBuffer = new char[this->eventSize];
    
}

void SlowControlsThread::operator ()()
{
    while(notTerminated)
    {
        switch(this->sctControl->getState())
        {
        case InterThread::SlowControlsThreadState::Terminate:
            BOOST_LOG_SEV(lg, Information) << "SC Thread: Terminating";
            //stop looping
            this->notTerminated = false;
            break;
        case InterThread::SlowControlsThreadState::Stopped:
            //if we are stopped, then wait to not be stopped
            //there is no need to acknowledge stop, that happens in wait for new state
            //this->sctControl->acknowledgeStop();
            BOOST_LOG_SEV(lg, Information) << "SC Thread: Stopped";
            this->sctControl->waitForNewState();
            break;
        case InterThread::SlowControlsThreadState::Polling:
            this->sctControl->acknowledgePolling();
            this->doPollingLoop();
            break;
        case InterThread::SlowControlsThreadState::Writing:
            //call the writing loop
            this->sctControl->acknowledgeWriting();
            this->doWritingLoop();
            break;
        }
    }
    //just before we exit, tell the controller that we have exited
    this->sctControl->acknowledgeTerminate();
}

void SlowControlsThread::doPollingLoop()
{
    //here we poll the mpod but do not send any voltage events to disk
    BOOST_LOG_SEV(lg, Information) << "SC Thread: Started Polling";
    //first time we enter into a mode, read *everything*
    //poll the mpod
    this->mpodReader->readAll();
    //publish the data for internal use
    this->publishInternalData();
    //do not write the data to disk
    //make a simple variable to count the number of times we have looped
    unsigned long long loopCount = 0;
    //set up the loop to run until we change state
    while(this->sctControl->getState() == InterThread::SlowControlsThreadState::Writing)
    {
        //pause the specified length of time using a timed wait
        this->sctControl->slowControlsThreadSleep(this->refreshPeriod);
        if((loopCount % 300)==0)
        {
            BOOST_LOG_SEV(lg, Information) << "SC Thread: Still Polling (# Loops: "<<loopCount<<")";
        }
        //only read things that actually change for normal polling
        this->mpodReader->readActive();
        //publish the data for internal use
        this->publishInternalData();
        //get the current state prior to going to the top of the loop again
        ++loopCount;
    }
}

void SlowControlsThread::doWritingLoop()
{
    BOOST_LOG_SEV(lg, Information) << "SC Thread: Started Writing";
    
    //start the slow controls file
    int runNumber;
    std::string runTitle;
    outputCtrl->getRunTitle(runTitle);
    runNumber = outputCtrl->getRunNumber();
    //now pass those parameters to the internal file
    outputFile.setNewRunParameters(runTitle, runNumber);
    
    //poll the mpod
    //first time we enter into write mode, read *everything*
    this->mpodReader->readAll();
    //write the event to file
    this->writeScEventToFile();
    //publish the data for the UI Thread
    this->publishInternalData();
    //make a simple variable to count the number of times we have looped
    unsigned long long loopCount = 0;
    //set up the loop to run until we change state
    while(this->sctControl->getState() == InterThread::SlowControlsThreadState::Writing)
    {
        //pause the specified length of time using a timed wait on a condition var
        this->sctControl->slowControlsThreadSleep(this->refreshPeriod);
        if((loopCount % 300)==0)
        {
            BOOST_LOG_SEV(lg, Information) << "SC Thread: Still Writing (# Loops: "<<loopCount<<")";
        }
        //only read the things that actually changed
        //poll the mpod
        this->mpodReader->readActive();
        //write the event to file
        this->writeScEventToFile();
        //publish the data for the UI Thread
        this->publishInternalData();
        ++loopCount;
    }
    //close the internal file
    outputFile.endRun();
}

}
