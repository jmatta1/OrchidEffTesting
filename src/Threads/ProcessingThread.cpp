/***************************************************************************//**
********************************************************************************
**
** @file ProcessingThread.cpp
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
** @details Definition file for the ProcessingThread callable
**
********************************************************************************
*******************************************************************************/
#include"ProcessingThread.h"
// includes for C system headers
// includes for C++ system headers
#include<iomanip>
// includes from other libraries
// includes from ORCHID

namespace Threads
{

void ProcessingThread::operator()()
{
    //run the event loop
    while(this->notTerminated)
    {
        switch(this->controller->getCurrentState())
        {
        case InterThread::ProcessingThreadState::Terminate:
            BOOST_LOG_SEV(lg, Information) << "PR Thread " << threadNumber << ": Terminating";
            this->notTerminated = false;
            break;
        case InterThread::ProcessingThreadState::Stopped:
            BOOST_LOG_SEV(lg, Information) << "PR Thread " << threadNumber << ": Stopped and waiting for instructions";
            //There is no need to acknowledge stop, it happens inside wait for change
            //this->controller->acknowledgeStop();
            this->controller->waitForChange();
            break;
        case InterThread::ProcessingThreadState::Running:
            BOOST_LOG_SEV(lg, Information) << "PR Thread " << threadNumber << ": Starting Processing";
            //acknowledge that we are starting
            this->controller->acknowledgeStart();
            //create a couple intermediates to hold the runTitle and runNumber
            std::string runTitle;
            int runNumber;
            //here we grab the run parameters
            outputCtrl->getRunTitle(runTitle);
            runNumber = outputCtrl->getRunNumber();
            //now pass those parameters to the internal file
            outputFile.setNewRunParameters(runTitle, runNumber);
            this->doProcessingLoop();
            BOOST_LOG_SEV(lg, Information) << "PR Thread " << threadNumber << ": Clearing buffer queue";
            this->emptyProcessingBuffer();
            outputFile.endRun();
            break;
        }
    }
    //if we are here then we are exiting
    this->controller->acknowledgeTerminate();
}

void ProcessingThread::doProcessingLoop()
{
    Utility::ToProcessingBuffer* dataBuffer = nullptr;
    while(this->controller->getCurrentState() == InterThread::ProcessingThreadState::Running)
    {
        //first pull a buffer from the file thread queue, this may cause a wait
        if(dataInputQueue->consumerPop(dataBuffer))
        {//if we got the buffer, proceed, if not make sure we are not being
            //asked to terminate
            processDataBuffer(dataBuffer); //when this finishes the data buffer
            //is completely handled and can be put back on the return queue
            this->dataInputQueue->consumerPush(dataBuffer);
            dataBuffer = nullptr;
        }
    }
    if(dataBuffer != nullptr)
    {
        this->dataInputQueue->consumerPush(dataBuffer);
    }
}

void ProcessingThread::emptyProcessingBuffer()
{//essentially loop until we cannot pull more buffers
    Utility::ToProcessingBuffer* dataBuffer = nullptr;
    while(dataInputQueue->tryConsumerPop(dataBuffer))
    {
        //asked to terminate
        processDataBuffer(dataBuffer); //when this finishes the data buffer
        //is completely handled and can be put back on the return queue
        this->dataInputQueue->consumerPush(dataBuffer);
        dataBuffer = nullptr;
    }
    if(dataBuffer != nullptr)
    {
        this->dataInputQueue->consumerPush(dataBuffer);
    }
}

void ProcessingThread::processDataBuffer(Utility::ToProcessingBuffer* buffer)
{
    //grab a couple useful things in the buffer
    unsigned int* rawBuffer = buffer->dataBuffer;
    int dataSize = buffer->sizeOfData;
    int board = buffer->info.boardNumber;
    int offset = 0;
    acqData->addData(board, 4*dataSize);
    //now loop through the board aggregates
    int loopCount = 0;
    while(offset < dataSize)
    {
        //first makes sure the board aggregate leads with the right info
        if(((rawBuffer[offset] & 0xF0000000UL)!=0xA0000000UL) && ((rawBuffer[offset] & 0x0FFFFFFFUL) <= (dataSize - offset)))
        {
            BOOST_LOG_SEV(lg, Warning) << "PR Thread " << threadNumber << ": Found corrupted board aggregate";
            BOOST_LOG_SEV(lg, Warning) << "PR Thread " << threadNumber << ": BA Header is: 0x" << std::hex << rawBuffer[offset] << " 0x" << rawBuffer[offset+1] << " 0x" << rawBuffer[offset+2] << " 0x" << rawBuffer[offset+3];
            BOOST_LOG_SEV(lg, Warning) << "PR Thread " << threadNumber << ": Brd #: " << std::dec << board << " Size: " << dataSize << " Offset: " << offset;
            //now skip the rest of the buffer
            break;
        }
        //if we have the right header, process the board aggregate
        offset += processBoardAggregate(buffer, offset);
        ++loopCount;
    }
}

int ProcessingThread::processBoardAggregate(Utility::ToProcessingBuffer* buffer, int startOffset)
{
    //grab a couple useful things in the buffer
    unsigned int* rawBuffer = buffer->dataBuffer;
    int startChan = buffer->info.startChannel;
    int offset = startOffset;
    int stopOffset = startOffset + (rawBuffer[offset] & 0x0FFFFFFFUL);
    ++offset;
    short chanMask = (rawBuffer[offset] & 0x000000FFUL);
    offset += 3;
    //now loop through the board aggregates
    int loopCount = 0;
    while(offset < stopOffset)
    {
        if(((chanMask >> loopCount) & 0x01) == 1)
        {
            offset += processChannelAggregate(buffer, offset, (2*loopCount+startChan));
        }
        //offset now points at the last channel aggregate event, increment it by
        //1 to point it at the start of the next channel aggregate
        //++offset;
        ++loopCount;
    }
    return (offset - startOffset);
}

int ProcessingThread::processChannelAggregate(Utility::ToProcessingBuffer* buffer, int startOffset, int baseChan)
{
    //grab a couple useful things in the buffer
    unsigned int* rawBuffer = buffer->dataBuffer;
    int board = buffer->info.boardNumber;
    int offset = startOffset;
    int stopOffset = startOffset + (rawBuffer[offset] & 0x00007FFFUL);
    ++offset;
    short sampleSlotsPerEvent = 8*(rawBuffer[offset] & 0x00000FFFUL);
    bool extraEnabled = ((rawBuffer[offset] & 0x10000000UL) != 0);
    bool waveformEnabled = ((rawBuffer[offset] & 0x08000000UL) != 0);
    short extraFormat = ((rawBuffer[offset] & 0x07000000UL) >> 24);
    ++offset;
    if(extraEnabled)
    {
        int skip = 0;
        if(waveformEnabled)
        {
            skip = (sampleSlotsPerEvent/2);
        }
        switch(extraFormat)
        {
        case 0x0:
            offset += processEventsWithExtras1(rawBuffer, offset, stopOffset, baseChan, board, skip);
            break;
        case 0x1:
            offset += processEventsWithExtras2(rawBuffer, offset, stopOffset, baseChan, board, skip);
            break;
        case 0x2:
            offset += processEventsWithExtras3(rawBuffer, offset, stopOffset, baseChan, board, skip);
            break;
            //ignore case 0x5, extras word, it is useless
        default:
            offset += processEventsWithoutExtras(rawBuffer, offset, stopOffset, baseChan, board, skip+1);
            break;
        }
    }
    else
    {
        int skip = 0;
        if(waveformEnabled)
        {
            skip = (sampleSlotsPerEvent/2);
        }
        offset += processEventsWithoutExtras(rawBuffer, offset, stopOffset, baseChan, board, skip);
    }
    return (offset-startOffset);
}

int ProcessingThread::processEventsWithExtras1(unsigned int* rawBuffer, int startOffset, int stopOffset, int baseChan, int boardNum, int skip)
{
    int offset = startOffset;
    int scaledStopOffset = stopOffset - (skip + 2);
    int skipSize = skip + 1;
    while(offset < scaledStopOffset)
    {
        //offset points to the start of an event (the trigger time tag and channel bit)
        evBuff.setBoard(boardNum);
        //put the data into the event
        if(rawBuffer[offset] & 0x80000000)
        {
            evBuff.setChannel(baseChan + 1);
            this->acqData->incrTrigs(baseChan + 1);
            ++(counters[baseChan+1]);
        }
        else
        {
            evBuff.setChannel(baseChan);
            this->acqData->incrTrigs(baseChan);
            ++(counters[baseChan]);
        }

        evBuff.setTimeStamp(rawBuffer[offset] & 0x7FFFFFFF);
        offset += skipSize; //now pointing to the EXTRAS word
        evBuff.setExtraTimeStamp((rawBuffer[offset] & 0xFFFF0000) >> 16);
        ++offset; //now pointing to integrals word
        evBuff.setLongGate((rawBuffer[offset] & 0xFFFF0000) >> 16);
        evBuff.setShortGate(rawBuffer[offset] & 0x00007FFF);
        //the only flag available is the pileup flag
        evBuff.setFlags((rawBuffer[offset] & 0x00008000) >> 15);
        ++offset;
        //now write the event to file
        outputFile.writeEvent(evBuff.getBuffer(), 15);
    }
    return (offset - startOffset);
}

int ProcessingThread::processEventsWithExtras2(unsigned int* rawBuffer, int startOffset, int stopOffset, int baseChan, int boardNum, int skip)
{
    int offset = startOffset;
    int scaledStopOffset = stopOffset - (skip + 2);
    int skipSize = skip + 1;
    while(offset < scaledStopOffset)
    {
        //offset points to the start of an event (the trigger time tag and channel bit)
        evBuff.setBoard(boardNum);
        //put the data into the event
        if(rawBuffer[offset] & 0x80000000)
        {
            evBuff.setChannel(baseChan + 1);
            this->acqData->incrTrigs(baseChan + 1);
            ++(counters[baseChan+1]);
        }
        else
        {
            evBuff.setChannel(baseChan);
            this->acqData->incrTrigs(baseChan);
            ++(counters[baseChan]);
        }
        evBuff.setTimeStamp(rawBuffer[offset] & 0x7FFFFFFF);
        offset += skipSize; //now pointing to the EXTRAS word
        evBuff.setExtraTimeStamp((rawBuffer[offset] & 0xFFFF0000) >> 16);
        unsigned short baseFlags = ((rawBuffer[offset] & 0x0000FFFF) >> 13);
        ++offset; //now pointing to integrals word
        //get the pileup flag
        baseFlags |= ((rawBuffer[offset] & 0x00008000) >> 15);
        evBuff.setFlags(baseFlags);
        evBuff.setLongGate((rawBuffer[offset] & 0xFFFF0000) >> 16);
        evBuff.setShortGate(rawBuffer[offset] & 0x00007FFF);
        ++offset; //now pointing to the start of the next event or aggregate
        //now write the event to the file
        outputFile.writeEvent(evBuff.getBuffer(), 15);
    }
    return (offset - startOffset);
}

int ProcessingThread::processEventsWithExtras3(unsigned int* rawBuffer, int startOffset, int stopOffset, int baseChan, int boardNum, int skip)
{
    int offset = startOffset;
    int scaledStopOffset = stopOffset - (skip + 2);
    int skipSize = skip + 1;
    while(offset < scaledStopOffset)
    {
        //offset points to the start of an event (the trigger time tag and channel bit)
        evBuff.setBoard(boardNum);
        if(rawBuffer[offset] & 0x80000000)
        {
            evBuff.setChannel(baseChan + 1);
            this->acqData->incrTrigs(baseChan + 1);
            ++(counters[baseChan+1]);
        }
        else
        {
            evBuff.setChannel(baseChan);
            this->acqData->incrTrigs(baseChan);
            ++(counters[baseChan]);
        }
        evBuff.setTimeStamp(rawBuffer[offset] & 0x7FFFFFFF);
        offset += skipSize; //now pointing to the EXTRAS word
        evBuff.setExtraTimeStamp((rawBuffer[offset] & 0xFFFF0000) >> 16);
        unsigned short baseFlags = ((rawBuffer[offset] & 0x0000FC00) >> 13);
        ++offset; //now pointing to integrals word
        baseFlags |= ((rawBuffer[offset] & 0x00008000) >> 15);
        //get the pileup flag
        evBuff.setFlags(baseFlags);
        evBuff.setLongGate((rawBuffer[offset] & 0xFFFF0000) >> 16);
        evBuff.setShortGate(rawBuffer[offset] & 0x00007FFF);
        ++offset; //now pointing to the start of the next event or aggregate
        //now write the event to the file
        outputFile.writeEvent(evBuff.getBuffer(), 15);
    }
    return (offset - startOffset);
}

int ProcessingThread::processEventsWithoutExtras(unsigned int* rawBuffer, int startOffset, int stopOffset, int baseChan, int boardNum, int skip)
{
    int offset = startOffset;
    int scaledStopOffset = stopOffset - (skip + 1);
    int skipSize = skip + 1;
    while(offset < scaledStopOffset)
    {
        //offset points to the start of an event (the trigger time tag and channel bit)
        evBuff.setBoard(boardNum);
        if(rawBuffer[offset] & 0x80000000)
        {
            evBuff.setChannel(baseChan + 1);
            this->acqData->incrTrigs(baseChan + 1);
            ++(counters[baseChan+1]);
        }
        else
        {
            evBuff.setChannel(baseChan + 1);
            this->acqData->incrTrigs(baseChan);
            ++(counters[baseChan]);
        }
        evBuff.setTimeStamp(rawBuffer[offset] & 0x7FFFFFFF);
        offset += skipSize; //now pointing to integrals word
        evBuff.setExtraTimeStamp(0);
        //the only flag available is the pileup flag
        evBuff.setFlags((rawBuffer[offset] & 0x00008000) >> 15);
        evBuff.setLongGate((rawBuffer[offset] & 0xFFFF0000) >> 16);
        evBuff.setShortGate(rawBuffer[offset] & 0x00007FFF);
        ++offset; //now pointing to the start of the next event or aggregate
        //now write the event to the file
        outputFile.writeEvent(evBuff.getBuffer(), 15);
    }
    return (offset - startOffset);
}

}
