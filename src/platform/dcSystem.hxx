////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef DREAMCLOUD__PLATFORM_SCLIB__DCSYSTEM_HXX
#define DREAMCLOUD__PLATFORM_SCLIB__DCSYSTEM_HXX

////////////////////
//    INCLUDES    //
////////////////////
#include <time.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <ctime>
#include <iostream>

#include "dcConfiguration.hxx"
#include "dcSimuParams.hxx"
#include "noc/nocInterconnect.hxx"
#include "processingElement.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicKhalidDC.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicMinComm.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicRandom.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicStatic.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicStaticSM.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicWeka.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicZigZag.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicZigZagSM.hxx"
#include "simulators-commons/mapping_heuristic/dcMappingHeuristicZigZagThreeCore.hxx"
#include "simulators-commons/mapping_heuristic/uoyHeuristicModuleStatic.hxx"
#include "simulators-commons/parser/AmApplication.h"
#include "simulators-commons/parser/dcAmaltheaParser.h"
#include "simulators-commons/parser/dcApplication.h"

namespace dreamcloud {
namespace platform_sclib {

////////////////////
//      USING     //
////////////////////
using dreamcloud::platform_sclib::noc_ppa::nocInterconnect;
using dreamcloud::platform_sclib::noc_ppa::Packet;
using std::ostringstream;
using std::pair;
using std::vector;
using namespace DCApplication;
using namespace std;

class dcSystem: sc_module {

public:

	sc_in<bool> clock;

	SC_HAS_PROCESS(dcSystem);
	dcSystem(sc_module_name name, dcSimuParams params_) :
			sc_module(name), params(params_), pktsFromPe("pktFromPe",
					params_.getRows()), pktsToPe("pktToPe", params_.getRows()), trReady(
					"trReady", params_.getRows()), newPktFromPe("newPktFrompe",
					params_.getRows()), newRunnable_event("newRunEvent",
					params_.getRows()), nbRunnablesCompleted(0), nbRunnablesMapped(
					0), nbPacketSendInNoc(0), nbpacketReceivedFromNoc(0), noc_(NULL) {

		// Init sc_vectors second dimension
		for (sc_vector<sc_vector<sc_signal<Packet> > >::size_type i = 0;
				i < pktsFromPe.size(); i++) {
			pktsFromPe.at(i).init(params.getCols());
			pktsToPe.at(i).init(params.getCols());
			trReady.at(i).init(params.getCols());
			newPktFromPe.at(i).init(params.getCols());
			newRunnable_event.at(i).init(params.getCols());
		}

		// Open files
		runnablesMappingCsvFile.open(params.getOutputFolder() + "/Mapping.csv");
		runnablesCsvFile.open(
				params.getOutputFolder() + "/OUTPUT_Runnable_Traces.csv");
		runnablesCsvFile << fixed << setprecision(0);
		runnablesCsvFile
				<< "Core_ID,Runnable,Priority,Allocation_Time,Start_Time,Completion_Time,Duration,Deadline,Deadline_Met_Missed"
				<< endl;
		runnablesArffFile.open(
				params.getOutputFolder() + "/OUTPUT_Runnable_Traces.arff");
		runnablesArffFile << "@RELATION runnable" << endl << endl;
		runnablesArffFile << "@ATTRIBUTE coreid           string" << endl;
		runnablesArffFile << "@ATTRIBUTE id               string" << endl;
		runnablesArffFile << "@ATTRIBUTE priority         NUMERIC" << endl;
		runnablesArffFile << "@ATTRIBUTE alloctime        NUMERIC" << endl;
		runnablesArffFile << "@ATTRIBUTE starttime        NUMERIC" << endl;
		runnablesArffFile << "@ATTRIBUTE completiontime   NUMERIC" << endl;
		runnablesArffFile << "@ATTRIBUTE duration         NUMERIC" << endl;
		runnablesArffFile << "@ATTRIBUTE deadline         NUMERIC" << endl;
		runnablesArffFile << "@ATTRIBUTE dealinemetmissed NUMERIC" << endl;
		runnablesArffFile << endl << endl << "@DATA" << endl;
		nocTrafficCsvFile = fopen(
				(params.getOutputFolder() + "/OUTPUT_NoC_Traces.csv").c_str(),
				"w+");
		fputs(
				"Packet ID,Priority,Read Request ID,Source,Destination,Injection Time(NS),Delivery Time(NS),Packet Latency(NS),Latency no Contention(NS)\n",
				nocTrafficCsvFile);
		nocTrafficArffFile.open(
				params.getOutputFolder() + "/OUTPUT_NoC_Traces.arff");
		nocTrafficArffFile << "@RELATION packet" << endl << endl;
		nocTrafficArffFile << "@ATTRIBUTE id            NUMERIC" << endl;
		nocTrafficArffFile << "@ATTRIBUTE priority      NUMERIC" << endl;
		nocTrafficArffFile << "@ATTRIBUTE info          NUMERIC" << endl;
		nocTrafficArffFile << "@ATTRIBUTE source        string" << endl;
		nocTrafficArffFile << "@ATTRIBUTE destination   string" << endl;
		nocTrafficArffFile << "@ATTRIBUTE injectiontime NUMERIC" << endl;
		nocTrafficArffFile << "@ATTRIBUTE deliverytime  NUMERIC" << endl;
		nocTrafficArffFile << "@ATTRIBUTE latency       NUMERIC" << endl;
		nocTrafficArffFile << "@ATTRIBUTE hoplatency    NUMERIC" << endl;
		nocTrafficArffFile << endl << endl << "@DATA" << endl;

		if (params.getGenerateWaveforms()) {
			runnablesVcdFile.open(
					params.getOutputFolder() + "/OUTPUT_CORE_WAVE.vcd");
			runnablesVcdFile << fixed << setprecision(0);
			time_t rawtime;
			struct tm * timeinfo;
			char buffer[180];
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(buffer, 80, "%d-%m-%Y %I:%M:%S", timeinfo);
			std::string str(buffer);
			runnablesVcdFile << "$date " << str << " $end" << endl;
			runnablesVcdFile << "$timescale 1 ns $end" << endl;
			runnableWaveIDs =
					fopen(
							(params.getOutputFolder()
									+ "/OUTPUT_RUNNABLE_IDs.csv").c_str(),
							"w+");
			fputs("ID , Runnable_Name , Task_Name\n", runnableWaveIDs);
		}
		constInstsCsvFile =
				fopen(
						(params.getOutputFolder()
								+ "/Instruction_fixed_Power.txt").c_str(),
						"w+");

		// Get app file or mode file
		string appPath;
		if (params.getAppXml().empty()) {
			parseModeFile(params.getModeFile());
			appPath = modes.front().file;
			simulationEndFromMode = modes.back().time;
		} else {
			appPath = params.getAppXml();
			simulationEndFromCmdLine = params.getSimuEnd();
			//cout << "simu end = " << simulationEndFromCmdLine << endl;
		}

		// Check and get scheduling strategy
		processingElement::SchedulingStrategy sched;
		if (params.getSchedulingStrategy() == "fcfs") {
			sched = processingElement::FCFS;
		} else if (params.getSchedulingStrategy() == "prio") {
			sched = processingElement::PRIO;
		} else {
			cerr << "scheduling strategy " << params.getSchedulingStrategy()
					<< " unknown" << endl;
			exit(-1);
		}

		cout << endl;
		cout
				<< "***************************************************************"
				<< endl;
		cout
				<< "***** Summary of platform (cores + NoC) simulation results ****"
				<< endl;
		cout
				<< "***************************************************************"
				<< endl;
		cout << endl << endl;
		cout << " ##Configuration information##" << endl << endl;
		if (params.getAppXml().empty()) {
			cout << "    Input mode switchig file           : "
					<< params.getModeFile() << endl;
		} else {
			cout << "    Input application                  : "
					<< params.getAppXml() << endl;
		}
		cout << "    Results output folder              : "
				<< params.getOutputFolder() << endl;
		cout << "    Number of cores   in platform      : "
				<< params.getRows() * params.getCols() << endl;
		cout << "    Number of rows    in platform      : " << params.getRows()
				<< endl;
		cout << "    Number of columns in platform      : " << params.getCols()
				<< endl;
		cout << "    Cores operating frequency          : "
				<< getFrequencyString(params.getCoresFrequencyInHz()) << endl;
		cout << "    Mapping heuristic                  : "
				<< params.getMappingHeuristic();
		if (params.getMappingHeuristic() == "Static"
				|| params.getMappingHeuristic() == "StaticSM"
				|| params.getMappingHeuristic() == "StaticModes") {
			cout << " " << params.getMappingFile();
		}
		if (params.getMappingHeuristic() == "Weka") {
			cout << " " << params.getMappingFile() << " "
					<< params.getWekaFile();
		}
		cout << endl;
		cout << "    Runnables scheduling strategy      : "
				<< params.getSchedulingStrategy() << endl;
		cout << "    Syntactic dependency consideration : "
				<< (params.getSeqDep() ? "true" : "false") << endl;
		cout << "    Waveform  generation               : "
				<< (params.getGenerateWaveforms() ? "true" : "false") << endl;
		cout << endl << endl;

		// Use the parser to create an AmApplication object
		int t0 = time(NULL);
		dcAmaltheaParser amaltheaParser;
		AmApplication* amApplication = new AmApplication();
		amaltheaParser.ParseAmaltheaFile(appPath, amApplication);

		// Assert we have the same instructionsPerCyle in
		// all core types in the app because today we are
		// only handling that case
		map<string, dcCoreType*> coreTypes = amApplication->getCoreTypesMap();
		typedef map<string, dcCoreType*>::iterator it_type;
		int instructionsPerCycle = 1;
		for (it_type iterator = coreTypes.begin(); iterator != coreTypes.end();
				iterator++) {
			if (instructionsPerCycle == 1) {
				instructionsPerCycle =
						iterator->second->GetInstructionsPerCycle();
			}
//				else {
//				if (iterator->second->GetInstructionsPerCycle()
//						!= instructionsPerCycle) {
//					cerr
//							<< "different instructionsPerCycle values found in application"
//							<< endl;
//					exit(-1);
//				}
//			}
		}

		// Convert the AmApplication to a dcApplication
		application = new dcApplication();
		taskGraph = application->createGraph("dcTaskGraph");
		application->CreateGraphEntities(taskGraph, amApplication,
				params.getSeqDep());
		tasks = application->GetAllTasks(taskGraph);
		labels = application->GetAllLabels(amApplication);
		runnables = application->GetAllRunnables(taskGraph);
		application->dumpLabelAccesses(taskGraph, params.getOutputFolder());
		application->dumpLabelAccessesPerRunnable(taskGraph,
				params.getOutputFolder());
		application->dumpRunnablesToFiles(taskGraph, params.getOutputFolder());

		// Create mapping heuristic module
		if (params.getMappingHeuristic() == "KhalidDC") {
			mappingHeuristic = new dcMappingHeuristicKahlidDC();
		} else if (params.getMappingHeuristic() == "MinComm") {
			dcMappingHeuristicMinComm *minComm =
					new dcMappingHeuristicMinComm();
			minComm->setRunnables(runnables);
			mappingHeuristic = minComm;
		} else if (params.getMappingHeuristic() == "Static") {
			dcMappingHeuristicStatic *staticMapping =
					new dcMappingHeuristicStatic();
			staticMapping->setFile(params.getMappingFile());
			mappingHeuristic = staticMapping;
		} else if (params.getMappingHeuristic() == "StaticSM") {
			dcMappingHeuristicStaticSM *staticMappingSM =
					new dcMappingHeuristicStaticSM();
			staticMappingSM->setFile(params.getMappingFile());
			mappingHeuristic = staticMappingSM;
		} else if (params.getMappingHeuristic() == "StaticModes") {
			uoyHeuristicModuleStatic *staticMappingSM =
					new uoyHeuristicModuleStatic();
			staticMappingSM->ParseModeListFile(params.getMappingFile());
			staticMappingSM->ParseModeChangeTiming(params.getMappingFile());
			mappingHeuristic = staticMappingSM;
		} else if (params.getMappingHeuristic() == "ZigZag") {
			mappingHeuristic = new dcMappingHeuristicZigZag();
		} else if (params.getMappingHeuristic() == "ZigZagSM") {
			mappingHeuristic = new dcMappingHeuristicZigZagSM();
		} else if (params.getMappingHeuristic() == "3Core") {
			mappingHeuristic = new dcMappingHeuristicZigZagThreeCore();
		} else if (params.getMappingHeuristic() == "Random") {
			mappingHeuristic = new dcMappingHeuristicRandom(
					params.getMappingSeed());
		} else if (params.getMappingHeuristic() == "Weka") {
			dcMappingHeuristicWeka *mappingHeuristicWeka =
					new dcMappingHeuristicWeka(params.getSeed());
			mappingHeuristicWeka->setNocSize(params.getRows(),
					params.getCols());
			mappingHeuristicWeka->setFiles(params.getMappingFile(),
					params.getWekaFile());
			mappingHeuristic = mappingHeuristicWeka;
		} else {
			cerr << "mapping heuristic " << params.getMappingHeuristic()
					<< " unknown" << endl;
			exit(-1);
		}
		mappingHeuristic->setNocSize(params.getRows(), params.getCols());
		mappingHeuristic->setApp(appPath);

		int t1 = time(NULL);
		cout << " ##Application Information Delivered by Parser##" << endl
				<< endl;
		periodicAndSporadicRunnables =
				application->GetPeriodicAndSporadicRunnables(taskGraph,
						params.getSeqDep());
		vector<dcRunnableCall*> independantNonPeriodicRunnables =
				application->GetIndependentNonPeriodicRunnables(taskGraph);
		hyperPeriod = application->GetHyperPeriodWithOffset(taskGraph);
		cout << "    Number of tasks: " << tasks.size() << endl;
		cout << "    Number of runnables: " << runnables.size() << endl;
		cout << "    Number of periodic and sporadic runnables: "
				<< periodicAndSporadicRunnables.size() << " (hyper period = "
				<< hyperPeriod << " ns)" << endl;
		cout << "    Number of labels: " << labels.size() << endl;
//		cout << "    Number of instructions: "
//				<< application->GetAllInstructions(taskGraph).size() << endl;
		cout << "    Parsing time: " << (t1 - t0) << " s" << endl << endl
				<< endl;

		vector<vector<dcRunnableCall *> > tasksToRunnables;
		for (std::vector<dcTask *>::size_type i = 0; i < tasks.size(); i++) {
			tasksToRunnables.push_back(
					application->GetTaskRunnableCalls(taskGraph, tasks.at(i)));
		}

		if (params.getGenerateWaveforms()) {
			int R_Index = 0;
			for (std::vector<vector<dcRunnableCall *>>::size_type waveIndexT = 0;
					waveIndexT < tasksToRunnables.size(); waveIndexT++) {
				for (std::vector<dcRunnableCall *>::size_type waveIndexR = 0;
						waveIndexR < tasksToRunnables.at(waveIndexT).size();
						waveIndexR++) {
					tasksToRunnables.at(waveIndexT).at(waveIndexR)->SetWaveID(
							R_Index);
					string R_Name = tasksToRunnables.at(waveIndexT).at(
							waveIndexR)->GetRunClassName();
					std::string s = std::to_string(R_Index);
					std::string ID_R = s + " , " + R_Name + " , "
							+ tasks.at(waveIndexT)->GetName() + "\n";
					const char* wave_temp_dump = ID_R.c_str();
					fputs(wave_temp_dump, runnableWaveIDs);
					R_Index++;
				}
			}
		}

		// Dump graph files
		application->dumpTaskGraphFile(
				params.getOutputFolder() + "/dcTasksGraphFile.gv", tasks);
		application->dumpTaskAndRunnableGraphFile(
				params.getOutputFolder() + "/dcTasksAndRunsGraphFile.gv", tasks);

		// Initialize some signals
		for (unsigned int row(0); row < params.getRows(); ++row) {
			for (unsigned int col(0); col < params.getCols(); ++col) {
				trReady[row][col] = true;
				newPktFromPe[row][col] = false;
			}
		}

		// Define VCD signals
		for (unsigned int row(0); row < params.getRows(); ++row) {
			for (unsigned int col(0); col < params.getCols(); ++col) {
				if (params.getGenerateWaveforms()) {
					runnablesVcdFile << "$var wire 12 " << VCD_ACTIVE_RUN_ID
							<< row << col << " ACTIVE_RUNNABLE_ID-PE" << row
							<< col << " $end\n";
					runnablesVcdFile << "$var event 1 "
							<< VCD_SUSPENDED_ON_REQUEST << row << col
							<< " SUSPENDED_ON_REQUEST-PE" << row << col
							<< " $end\n";
					runnablesVcdFile << "$var event 1 " << VCD_PREEMPTION << row
							<< col << " PREEMPTION-PE" << row << col
							<< " $end\n";
					runnablesVcdFile << "$var event 1 "
							<< VCD_RUNNABLE_COMPLETED << row << col
							<< " RUNNABLE_COMPLETED-PE" << row << col
							<< " $end\n";
					runnablesVcdFile << "$var event 1 " << VCD_DEADLINE_MISSED
							<< row << col << " DEADLINE_MISSED-PE" << row << col
							<< " $end\n";
				}
			}
		}

		// Init VCD signals
		if (params.getGenerateWaveforms()) {
			runnablesVcdFile << "$upscope $end\n$enddefinitions $end\n";
			for (unsigned int row(0); row < params.getRows(); ++row) {
				for (unsigned int col(0); col < params.getCols(); ++col) {
					runnablesVcdFile << "#0" << " " << VCD_12_UNDEF << " "
							<< VCD_ACTIVE_RUN_ID << row << col << endl;
				}
			}
		}

		// Adjusting dimension of the Processing Element (PE) table
		pes.resize(params.getRows());
		for (unsigned int row(0); row < params.getRows(); ++row) {
			pes[row].resize(params.getCols());
		}

		// Initializing the PE table
		for (unsigned int row(0); row < params.getRows(); ++row) {
			for (unsigned int col(0); col < params.getCols(); ++col) {

				ostringstream name("");
				name << "PE(" << row << "," << col << ")";

				pes[row][col] = new processingElement(name.str().c_str(),
						constInstsCsvFile, nocTrafficCsvFile,
						&nocTrafficArffFile, &runnablesCsvFile,
						&runnablesArffFile, &runnablesVcdFile, sched, params,
						instructionsPerCycle);
				pes[row][col]->x_PE = row;
				pes[row][col]->y_PE = col;
				pes[row][col]->canReceivePkt_signal(trReady[row][col]);
				pes[row][col]->newPacket_signal(newPktFromPe[row][col]);
				//pes[row][col]->clock(clock);
				pes[row][col]->packet_in(pktsToPe[row][col]);
				pes[row][col]->packet_out(pktsFromPe[row][col]);
				pes[row][col]->runnableCompleted_event =
						&runnableCompleted_event;
				pes[row][col]->packetToSend_event = &packetToSend_event;
				pes[row][col]->newRunnable_event = &newRunnable_event[row][col];
			}
		}

		// Creating the NoC interconnect
		noc_ = new nocInterconnect("NoC", params_);
		noc_->packet_in(commIn);
		noc_->packet_out(commOut);

		// Record start time to compute simulation time at the end
		start = std::clock();

		// Create SystemC processes
		SC_CTHREAD(pktFromPeReceiver_cthread, clock.pos());
		SC_METHOD(pktToPeDeliverer_method);
		sensitive << commOut;
		dont_initialize();
		SC_THREAD(runnablesMapper_thread);
		sensitive << runnableReleased_event;
		SC_THREAD(stopSimu_thread);
		sensitive << stopSimu_event;
		dont_initialize();
		SC_METHOD(dependentRunnablesReleaser_method);
		sensitive << runnableCompleted_event;
		dont_initialize();
		SC_THREAD(periodicRunnablesReleaser_thread);
		SC_METHOD(nonPeriodicIndependentRunnablesReleaser_method);
		sensitive << iteration_event;

		// Start mode switching thread if required
		if (simulationEndFromMode != 0) {
			SC_THREAD(modeSwitcher_thread);
		} else {
			SC_METHOD(labelsMapper_method);
		}
	}

	inline ~dcSystem() {
		runnablesCsvFile.close();
		runnablesArffFile.close();
		runnablesMappingCsvFile.close();
		nocTrafficArffFile.close();
		fclose(nocTrafficCsvFile);
		if (params.getGenerateWaveforms()) {
			runnablesVcdFile << "#" << (sc_time_stamp().value() * 1E-3) << endl;
			runnablesVcdFile.close();
		}
		delete mappingHeuristic;
	}

	// Types definitions
	typedef vector<vector<processingElement *> > peTable;
	typedef vector<vector<vector<Packet> > > pktTable;
	typedef pair<int, int> labelLocation;
	typedef pair<labelLocation, string> labelMapping;

private:

	// SystemC processes
	void pktFromPeReceiver_cthread();
	void pktToPeDeliverer_method();
	void runnablesMapper_thread();
	void labelsMapper_method();
	void dependentRunnablesReleaser_method();
	void periodicRunnablesReleaser_thread();
	void nonPeriodicIndependentRunnablesReleaser_method();
	void modeSwitcher_thread();

	// Utility functions
	void dumpNoCLoadGraphFile(unsigned long int time);
	void dumpCoresLoadGraphFile(unsigned long int time);
	void dumpTraces();
	void parseModeFile(string file);
	void updateInstructionsExecTimeBounds(string newAppFile);
	string getFrequencyString(unsigned long int freqInHertz);

	// Function used to release a runnable and wake up the mapper.
	void releaseRunnable(dcRunnableInstance *runnable);
	sc_event runnableReleased_event;

	// The mapping heuristic modules
	dcMappingHeuristicI * mappingHeuristic;

	// Stop the simulation
	void stopSimu_thread();
	sc_event stopSimu_event;

	peTable pes;
	vector<labelMapping> mappingTable;
	dcSimuParams params;

	// Modes management
	typedef struct {
		unsigned long int time;
		string name;
		string file;
	} mode_t;
	vector<mode_t> modes;
	unsigned long int simulationEndFromMode = 0;
	unsigned long int simulationEndFromCmdLine;


	// Communication medium interface
	sc_buffer<Packet> commIn;
	sc_buffer<Packet> commOut;
	sc_vector<sc_vector<sc_signal<Packet> > > pktsFromPe;
	sc_vector<sc_vector<sc_buffer<Packet> > > pktsToPe;
	sc_vector<sc_vector<sc_signal<bool> > > trReady;
	sc_vector<sc_vector<sc_signal<bool> > > newPktFromPe;
	nocInterconnect* noc_;

	// Output files
	FILE* nocTrafficCsvFile;
	ofstream nocTrafficArffFile;
	FILE* constInstsCsvFile;
	FILE* runnableWaveIDs;
	ofstream runnablesCsvFile;
	ofstream runnablesArffFile;
	ofstream runnablesMappingCsvFile;
	ofstream runnablesVcdFile;

	// Amalthea application
	dcTaskGraph* taskGraph;
	dcApplication* application;
	vector<dcTask*> tasks;
	vector<dcLabel*> labels;

	// List of released runnable instances
	vector<dcRunnableInstance*> readyRunnables;

	// List of application runnable calls
	vector<dcRunnableCall*> runnables;

	// Management of periodic runnables
	vector<dcRunnableCall*> periodicAndSporadicRunnables;
	unsigned long hyperPeriod;

	// Runnable Mapper and NoC handshake signals.
	sc_vector<sc_vector<sc_event> > newRunnable_event;
	vector<Packet> packet_local_;

	// Runnables and packets counting
	unsigned int nbRunnablesCompleted;
	unsigned int nbRunnablesMapped;
	unsigned int nbPacketSendInNoc;
	unsigned int nbpacketReceivedFromNoc;

	// Event used by processing elements to notify mapper
	sc_event runnableCompleted_event; // when a runnable is completed
	sc_event packetToSend_event; // when a packet need to be send

	// Event used to notify the nonPeriodicRunnableReleaser method on each new iteration
	sc_event iteration_event;

	// Simulation start time
	clock_t start;
};

}
}

#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
