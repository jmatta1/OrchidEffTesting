/***************************************************************************//**
********************************************************************************
**
** @file SnmpUtilCommands.h
** @author James Till Matta
** @date 02 May, 2016
** @brief
**
** @copyright Copyright (C) 2016 James Till Matta
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
** @details Lookup table to convert a command enum into a command string
**
********************************************************************************
*******************************************************************************/

#ifndef ORCHID_SRC_HVLIB_SNMPUTILCOMMANDS_H
#define ORCHID_SRC_HVLIB_SNMPUTILCOMMANDS_H

// includes for C system headers
// includes for C++ system headers
#include<map>
#include<string>
// includes from other libraries
#include<boost/assign/list_of.hpp>
using boost::assign::map_list_of;
// includes from ORCHID

//this is a hackish solution to generate the appropriate strings for enums
//it is crap to maintain, but it will work for now
enum class MpodGlobalGetParam: char{SysMainSwitch, SysStatus};


enum class MpodChannelGetParam: char{OutputStatus, OutputSwitch, SenseVoltage, TerminalVoltage,
                                     Temperature, SetVoltage, Current, RampUp,
                                     RampDown, MaxTerminalVoltage, MaxCurrent,
                                     SetCurrent, MaxCurrentTripTime,};

enum class MpodGlobalSetParam: char{SysMainSwitch};


enum class MpodChannelSetParam: char{OutputSwitch, SetVoltage, RampUp, RampDown,
                                     MaxTerminalVoltage, MaxCurrent, MaxCurrentTripTime};

namespace CmdLookup
{

static const std::map<MpodGlobalGetParam, std::string> GLOBAL_GET_COMMANDS =
    map_list_of (MpodGlobalGetParam::SysMainSwitch, "sysMainSwitch.0")
                (MpodGlobalGetParam::SysStatus    , "sysStatus.0");

static const std::map<MpodChannelGetParam, std::string> CHANNEL_GET_COMMANDS =
    map_list_of (MpodChannelGetParam::OutputStatus      , "outputStatus")
                (MpodChannelGetParam::OutputSwitch      , "outputSwitch")
                (MpodChannelGetParam::SenseVoltage      , "outputMeasurementSenseVoltage")
                (MpodChannelGetParam::TerminalVoltage   , "outputMeasurementTerminalVoltage")
                (MpodChannelGetParam::Current           , "outputMeasurementCurrent")
                (MpodChannelGetParam::Temperature       , "outputMeasurementTemperature")
                (MpodChannelGetParam::SetVoltage        , "outputVoltage")
                (MpodChannelGetParam::SetCurrent        , "outputCurrent")
                (MpodChannelGetParam::RampUp            , "outputVoltageRiseRate")
                (MpodChannelGetParam::RampDown          , "outputVoltageFallRate")
                (MpodChannelGetParam::MaxTerminalVoltage, "outputSupervisionMaxTerminalVoltage")
                (MpodChannelGetParam::MaxCurrent        , "outputSupervisionMaxCurrent")
                (MpodChannelGetParam::MaxCurrentTripTime, "outputTripTimeMaxCurrent");

static const std::map<MpodGlobalSetParam, std::string> GLOBAL_SET_COMMANDS =
    map_list_of (MpodGlobalSetParam::SysMainSwitch, "sysMainSwitch.0");

static const std::map<MpodChannelSetParam, std::string> CHANNEL_SET_COMMANDS =
    map_list_of (MpodChannelSetParam::OutputSwitch      , "outputSwitch")
                (MpodChannelSetParam::SetVoltage        , "outputVoltage")
                (MpodChannelSetParam::RampUp            , "outputVoltageRiseRate")
                (MpodChannelSetParam::RampDown          , "outputVoltageFallRate")
                (MpodChannelSetParam::MaxTerminalVoltage, "outputSupervisionMaxTerminalVoltage")
                (MpodChannelSetParam::MaxCurrent        , "outputSupervisionMaxCurrent")
                (MpodChannelSetParam::MaxCurrentTripTime, "outputTripTimeMaxCurrent");

}
#endif //ORCHID_SRC_HVLIB_SNMPUTILCOMMANDS_H
