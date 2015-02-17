
#include "AddVehicle.h"
#include "Router.h"
#include <algorithm>

namespace VENTOS {

Define_Module(VENTOS::AddVehicle);

AddVehicle::~AddVehicle()
{

}


void AddVehicle::initialize(int stage)
{
    if(stage ==0)
    {
        // get the ptr of the current module
        nodePtr = FindModule<>::findHost(this);
        if(nodePtr == NULL)
            error("can not get a pointer to the module.");

        // get a pointer to the TraCI module
        cModule *module = simulation.getSystemModule()->getSubmodule("TraCI");
        TraCI = static_cast<TraCI_Extend *>(module);

        Signal_executeFirstTS = registerSignal("executeFirstTS");
        simulation.getSystemModule()->subscribe("executeFirstTS", this);

        on = par("on").boolValue();
        mode = par("mode").longValue();
        totalVehicles = par("totalVehicles").longValue();
        lambda = par("lambda").longValue();
        plnSize = par("plnSize").longValue();
        plnSpace = par("plnSpace").doubleValue();
    }
}


void AddVehicle::finish()
{


}


void AddVehicle::handleMessage(cMessage *msg)
{

}


void AddVehicle::receiveSignal(cComponent *source, simsignal_t signalID, long i)
{
    Enter_Method_Silent();

    if(signalID == Signal_executeFirstTS)
    {
        AddVehicle::Add();
    }
}


void AddVehicle::Add()
{
    // if dynamic adding is off, return
    if (!on)
        return;

    // incident detection
    if(mode == 1)
    {
        Scenario1();
    }
    // all ACC
    else if(mode == 2)
    {
        Scenario2();
    }
    // all CACC
    else if(mode == 3)
    {
        Scenario3();
    }
    else if(mode == 4)
    {
        Scenario4();
    }
    else if(mode == 5)
    {
        Scenario5();
    }
    // bi-directional control
    else if(mode == 6)
    {
        Scenario6();
    }
    else if(mode == 7)
    {
        Scenario7();
    }
    else if(mode == 8)
    {
        Scenario8();
    }
    else if(mode == 9)
    {
        Scenario9();
    }
    else
    {
        error("not a valid mode!");
    }
}


void AddVehicle::Scenario1()
{
    int depart = 0;

    for(int i=1; i<=totalVehicles; i++)
    {
        char vehicleName[10];
        sprintf(vehicleName, "Veh%d", i);
        depart = depart + 9000;

        uint8_t lane = intrand(3);  // random number in [0,3)

        TraCI->commandAddVehicle(vehicleName, "TypeManual", "route1", depart, 0, 0, lane);
        TraCI->commandSetLaneChangeMode(vehicleName, 0b1000010101 /*0b1000100101*/);
    }

    // now we add a vehicle as obstacle
    TraCI->commandAddVehicle("obstacle", "TypeObstacle", "route1", 50, 3200, 0, 1);

    // make it stop on the lane!
    TraCI->commandChangeVehicleSpeed("obstacle", 0.);
    TraCI->commandSetLaneChangeMode("obstacle", 0);

    // change the color to red
    TraCIColor newColor = TraCIColor::fromTkColor("red");
    TraCI->commandChangeVehicleColor("obstacle", newColor);
}


void AddVehicle::Scenario2()
{
    int depart = 0;

    for(int i=1; i<=totalVehicles; i++)
    {
        char vehicleName[10];
        sprintf(vehicleName, "ACC%d", i);
        depart = depart + 10000;

        TraCI->commandAddVehicle(vehicleName, "TypeACC", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);
    }
}


void AddVehicle::Scenario3()
{
    int depart = 0;

     for(int i=1; i<=totalVehicles; i++)
     {
         char vehicleName[10];
         sprintf(vehicleName, "CACC%d", i);
         depart = depart + 10000;

         TraCI->commandAddVehicle(vehicleName, "TypeCACC", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);
     }
}

bool fexists(const char *filename)
{
    ifstream ifile(filename);
    return ifile;
}

vector<string> getEdgeNames(string netName)
{
  vector<string> edgeNames;

  file <> xmlFile(netName.c_str());
  xml_document<> doc;
  xml_node<> *node;
  doc.parse<0>(xmlFile.data());
  for(node = doc.first_node()->first_node("edge"); node; node = node->next_sibling("edge"))
    edgeNames.push_back(node->first_attribute()->value());
  return edgeNames;
}

vector<string> getNodeNames(string netName)
{
  vector<string> nodeNames;
  file <> xmlFile(netName.c_str());
  xml_document<> doc;
  xml_node<> *node;
  doc.parse<0>(xmlFile.data());
  for(node = doc.first_node()->first_node("junction"); node; node = node->next_sibling("junction"))
    nodeNames.push_back(node->first_attribute()->value());
  return nodeNames;
}

double curve(double x)  //Input will linearly increase from 0 to 1, from first to last vehicle.
{                       //Output should be between 0 and 1, scaled by some function
    return x;
}


void generateVehicles(string dir, Router* r)
{
  string netName = dir + "/hello.net.xml";
  string vName = dir + "/Vehicles" + SSTR(r->totalVehicleCount) + ".xml";
  ifstream netFile(netName.c_str());

  srand(time(NULL));
  vector<string> edgeNames = getEdgeNames(netName);
  vector<string> nodeNames = getNodeNames(netName);

  ofstream vFile(vName.c_str());
  vFile << "<vehicles>" << endl;
  for(int i = 1; i <= r->totalVehicleCount; i++)
  {
    string edge = edgeNames[rand() % edgeNames.size()];
    string node = nodeNames[rand() % nodeNames.size()];
    //vFile << "   <vehicle id=\"v" << i << "\" type=\"TypeManual\" origin=\"" << edge << "\" destination=\"" << node << "\" depart=\"" << i * r->createTime / r->totalVehicleCount << "\" />" << endl;

    vFile << "   <vehicle id=\"v" << i << "\" type=\"TypeManual\" origin=\"" << edge << "\" destination=\""
          << node << "\" depart=\"" << curve((double)i/r->totalVehicleCount) * r->createTime << "\" />" << endl;
  }
  vFile << "</vehicles>" << endl;
  vFile.close();
}

void AddVehicle::Scenario4()
{
    cModule *module = simulation.getSystemModule()->getSubmodule("router");
    Router *r = static_cast< Router* >(module);

    boost::filesystem::path VENTOS_FullPath = cSimulation::getActiveSimulation()->getEnvir()->getConfig()->getConfigEntry("network").getBaseDirectory();
    boost::filesystem::path SUMO_Path = simulation.getSystemModule()->par("SUMODirectory").stringValue();
    boost::filesystem::path SUMO_FullPath = VENTOS_FullPath / SUMO_Path;
    // check if this directory is valid?
    if( !exists( SUMO_FullPath ) )
    {
        error("SUMO directory is not valid! Check it again.");
    }

    string vehFile = ("/Vehicles" + SSTR(r->totalVehicleCount) + ".xml");
    string xmlFileName = SUMO_FullPath.string();
    xmlFileName += vehFile;

    if(!fexists(xmlFileName.c_str()))
    {
        generateVehicles(SUMO_FullPath.string(), r);
    }

    file<> xmlFile( xmlFileName.c_str() );     // Convert our file to a rapid-xml readable object
    xml_document<> doc;                        // Build a rapidxml doc
    doc.parse<0>(xmlFile.data());              // Fill it with data from our file
    xml_node<> *node = doc.first_node("vehicles"); // Parse up to the "nodes" declaration

    string id, type, origin, destination;
    double depart;
    for(node = node->first_node("vehicle"); node; node = node->next_sibling()) // For each vehicle
    {
        int readCount = 0;
        for(xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute())//For each attribute
        {
            switch(readCount)   //Read that attribute to the right variable
            {
            case 0:
                id = attr->value();
                break;
            case 1:
                type = attr->value();
                break;
            case 2:
                origin = attr->value();
                break;
            case 3:
                destination = attr->value();
                break;
            case 4:
                depart = atof(attr->value());
                break;
            }
            readCount++;
        }
        if(readCount < 5)
        {
            error("XML formatted wrong! Not enough elements given for some vehicle.");
        }

        r->net->vehicles[id] = new Vehicle(id, type, origin, destination, depart);

        list<string> routeList = TraCI->commandGetRouteIds();   //Get all the routes so far
        bool foundRoute = 0;
        for(list<string>::iterator it = routeList.begin(); it != routeList.end(); it++)   //Loop through them
        {
            //cout << "Found route " << *it << endl;
            if(*it == origin)   //If we find the route named after this vehicle's starting edge, do nothing
            {
                foundRoute = 1;
            }
        }
        if(!foundRoute) //Otherwise, build a new route
        {
            //cout << "Made route " << origin << endl;
            list<string> startRoute;
            startRoute.push_back(origin);   //With just the starting edge
            TraCI->commandAddRoute(origin, startRoute);   //And add it to the simulation
        }

        //cout << "Routes" << endl;
        //for(list<string>::iterator it = routeList.begin(); it != routeList.end(); it++)
        //    cout << *it << endl;

        //commandAddVehicleRouter wants string id, string type, string (edge) origin, string (node) destination, double (time) depart, and string routename
        TraCI->commandAddVehicle(id, type, origin, 1000 * depart, 0, 0, 0);  //Send a TraCI add call -- might not need to be *1000.
    }
}


void AddVehicle::Scenario5()
{
    // change from 'veh/h' to 'veh/s'
    lambda = lambda / 3600;

    // 1 vehicle per 'interval' milliseconds
    double interval = (1 / lambda) * 1000;

    int depart = 0;

    for(int i=0; i<totalVehicles; i++)
    {
        char vehicleName[10];
        sprintf(vehicleName, "CACC%d", i+1);
        depart = depart + interval;

        TraCI->commandAddVehicle(vehicleName, "TypeCACC", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);
    }
}


void AddVehicle::Scenario6()
{
    int depart = 0;

     for(int i=1; i<=totalVehicles; i++)
     {
         char vehicleName[10];
         sprintf(vehicleName, "CACC%d", i);
         depart = depart + 10000;

         TraCI->commandAddVehicle(vehicleName, "TypeBiDi", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);
     }
}


void AddVehicle::Scenario7()
{
    int depart = 0;

     for(int i=1; i<=10; i++)
     {
         char vehicleName[10];
         sprintf(vehicleName, "CACC%d", i);
         depart = depart + 1000;

         TraCI->commandAddVehicle(vehicleName, "TypeCACC", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);
     }

     for(int i=11; i<=100; i++)
     {
         char vehicleName[10];
         sprintf(vehicleName, "CACC%d", i);
         depart = depart + 10000;

         TraCI->commandAddVehicle(vehicleName, "TypeCACC", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);
     }
}


void AddVehicle::Scenario8()
{
    // change from 'veh/h' to 'veh/s'
    lambda = lambda / 3600;

    // 1 vehicle per 'interval' milliseconds
    //double interval = (1 / lambda) * 1000;

    double interval = 5000;

    int depart = 0;

    TraCI->commandSetLaneVmax("1to2_0", 400.);
    TraCI->commandSetMaxSpeed("TypeCACC", 400.);
    TraCI->commandSetVint("TypeCACC", 400.);
    TraCI->commandSetComfAccel("TypeCACC", 400.);

    for(int i=0; i<totalVehicles; i++)
    {
        char vehicleName[10];
        sprintf(vehicleName, "CACC%d", i+1);
        depart = depart + interval;

        TraCI->commandAddVehicle(vehicleName, "TypeCACC", "route1", depart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);

        if(i == 0)
        {
            TraCI->commandChangeVehicleSpeed(vehicleName, 20.);
        }
        else
        {
            TraCI->commandChangeVehicleSpeed(vehicleName, 400.);
            TraCI->commandSetVehicleMaxAccel(vehicleName, 400.);
        }

        if(i % plnSize == 0)
        {
            TraCI->commandSetVehicleTg(vehicleName, plnSpace);
        }
    }
}


void AddVehicle::Scenario9()
{
    int vehicleDepart = 0;
    int bicycleDepart = 0;
    int pedestrianDepart = 0;

     for(int i=1; i<=totalVehicles; i++)
     {
         char vehicleName[10];
         sprintf(vehicleName, "Veh%d", i);
         vehicleDepart = vehicleDepart + 10000;
         TraCI->commandAddVehicle(vehicleName, "TypeManual", "route4", vehicleDepart, 0 /*pos*/, 0 /*speed*/, 0 /*lane*/);

         char bicycleName[10];
         sprintf(bicycleName, "Bike%d", i);
         bicycleDepart = bicycleDepart + 10000;
         TraCI->commandAddVehicle(bicycleName, "TypeBicycle", "route1", bicycleDepart, 350 /*pos*/, 0 /*speed*/, 1 /*lane*/);

         char pedestrianName[10];
         sprintf(pedestrianName, "Ped%d", i);
         pedestrianDepart = pedestrianDepart + 10000;
         TraCI->commandAddVehicle(pedestrianName, "TypePedestrian", "route1", pedestrianDepart, 350 /*pos*/, 0 /*speed*/, 2 /*lane*/);
     }
}

}