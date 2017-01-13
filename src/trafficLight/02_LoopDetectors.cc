/****************************************************************************/
/// @file    LoopDetectors.cc
/// @author  Mani Amoozadeh <maniam@ucdavis.edu>
/// @author
/// @date    April 2015
///
/****************************************************************************/
// VENTOS, Vehicular Network Open Simulator; see http:?
// Copyright (C) 2013-2015
/****************************************************************************/
//
// This file is part of VENTOS.
// VENTOS is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <iomanip>

#include "trafficLight/02_LoopDetectors.h"

namespace VENTOS {

Define_Module(VENTOS::LoopDetectors);


LoopDetectors::~LoopDetectors()
{

}


void LoopDetectors::initialize(int stage)
{
    super::initialize(stage);

    if(stage == 0)
    {
        collectInductionLoopData = par("collectInductionLoopData").boolValue();

        Vec_loopDetectors.clear();
    }
}


void LoopDetectors::finish()
{
    super::finish();

    if(collectInductionLoopData)
        saveLDsData();
}


void LoopDetectors::handleMessage(omnetpp::cMessage *msg)
{
    super::handleMessage(msg);
}


void LoopDetectors::initialize_withTraCI()
{
    super::initialize_withTraCI();

    if(collectInductionLoopData)
        AllLDs = TraCI->LDGetIDList();   // get all loop detectors
}


void LoopDetectors::executeEachTimeStep()
{
    super::executeEachTimeStep();

    if(collectInductionLoopData)
        collectLDsData();    // collecting induction loop data in each timeStep
}


void LoopDetectors::collectLDsData()
{
    // for each loop detector
    for (auto &it : AllLDs)
    {
        std::vector<std::string>  st = TraCI->LDGetLastStepVehicleData(it);

        // only if this loop detector detected a vehicle
        if( st.size() > 0 )
        {
            // laneID of loop detector
            std::string lane = TraCI->LDGetLaneID(it);

            // get vehicle information
            std::string vehicleName = st.at(0);
            double entryT = atof( st.at(2).c_str() );
            double leaveT = atof( st.at(3).c_str() );
            double speed = TraCI->LDGetLastStepMeanVehicleSpeed(it);  // vehicle speed at current moment

            const LoopDetectorData *searchFor = new LoopDetectorData( it.c_str(), "", vehicleName.c_str() );
            auto counter = std::find(Vec_loopDetectors.begin(), Vec_loopDetectors.end(), *searchFor);

            // its a new entry, so we add it
            if(counter == Vec_loopDetectors.end())
            {
                LoopDetectorData *tmp = new LoopDetectorData( it.c_str(), lane.c_str(), vehicleName.c_str(), entryT, leaveT, speed, speed );
                Vec_loopDetectors.push_back(*tmp);
            }
            // if found, just update leaveTime and leaveSpeed
            else
            {
                counter->leaveTime = leaveT;
                counter->leaveSpeed = speed;
            }
        }
    }
}


void LoopDetectors::saveLDsData()
{
    if(Vec_loopDetectors.empty())
        return;

    int currentRun = omnetpp::getEnvir()->getConfigEx()->getActiveRunNumber();

    std::ostringstream fileName;
    fileName << boost::format("%03d_loopDetector.txt") % currentRun;

    boost::filesystem::path filePath ("results");
    filePath /= fileName.str();

    FILE *filePtr = fopen (filePath.c_str(), "w");
    if (!filePtr)
        throw omnetpp::cRuntimeError("Cannot create file '%s'", filePath.c_str());

    // write simulation parameters at the beginning of the file
    {
        // get the current config name
        std::string configName = omnetpp::getEnvir()->getConfigEx()->getVariable("configname");

        std::string iniFile = omnetpp::getEnvir()->getConfigEx()->getVariable("inifile");

        // PID of the simulation process
        std::string processid = omnetpp::getEnvir()->getConfigEx()->getVariable("processid");

        // globally unique identifier for the run, produced by
        // concatenating the configuration name, run number, date/time, etc.
        std::string runID = omnetpp::getEnvir()->getConfigEx()->getVariable("runid");

        // get number of total runs in this config
        int totalRun = omnetpp::getEnvir()->getConfigEx()->getNumRunsInConfig(configName.c_str());

        // get the current run number
        int currentRun = omnetpp::getEnvir()->getConfigEx()->getActiveRunNumber();

        // get all iteration variables
        std::vector<std::string> iterVar = omnetpp::getEnvir()->getConfigEx()->unrollConfig(configName.c_str(), false);

        // write to file
        fprintf (filePtr, "configName      %s\n", configName.c_str());
        fprintf (filePtr, "iniFile         %s\n", iniFile.c_str());
        fprintf (filePtr, "processID       %s\n", processid.c_str());
        fprintf (filePtr, "runID           %s\n", runID.c_str());
        fprintf (filePtr, "totalRun        %d\n", totalRun);
        fprintf (filePtr, "currentRun      %d\n", currentRun);
        fprintf (filePtr, "currentConfig   %s\n", iterVar[currentRun].c_str());
        fprintf (filePtr, "startDateTime   %s\n", TraCI->simulationGetStartTime().c_str());
        fprintf (filePtr, "endDateTime     %s\n", TraCI->simulationGetEndTime().c_str());
        fprintf (filePtr, "duration        %s\n\n\n", TraCI->simulationGetDuration().c_str());
    }

    // write header
    fprintf (filePtr, "%-30s","loopDetector");
    fprintf (filePtr, "%-20s","lane");
    fprintf (filePtr, "%-20s","vehicleName");
    fprintf (filePtr, "%-20s","vehicleEntryTime");
    fprintf (filePtr, "%-20s","vehicleLeaveTime");
    fprintf (filePtr, "%-22s","vehicleEntrySpeed");
    fprintf (filePtr, "%-22s\n\n","vehicleLeaveSpeed");

    // write body
    for(auto& y : Vec_loopDetectors)
    {
        fprintf (filePtr, "%-30s", y.detectorName.c_str());
        fprintf (filePtr, "%-20s", y.lane.c_str());
        fprintf (filePtr, "%-20s", y.vehicleName.c_str());
        fprintf (filePtr, "%-20.2f", y.entryTime);
        fprintf (filePtr, "%-20.2f", y.leaveTime);
        fprintf (filePtr, "%-20.2f", y.entrySpeed);
        fprintf (filePtr, "%-20.2f\n", y.leaveSpeed);
    }

    fclose(filePtr);
}

}
