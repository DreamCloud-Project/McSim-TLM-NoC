////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef DREAMCLOUD__PLATFORM_SCLIB__PROCESSINGELEMENT_HXX
#define DREAMCLOUD__PLATFORM_SCLIB__PROCESSINGELEMENT_HXX

////////////////////
//    INCLUDES    //
////////////////////
#include <systemc.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include "noc/packet.hxx"
#include "dcConfiguration.hxx"
#include "dcSimuParams.hxx"
#include "processingElementType.hxx"
#include "commons/parser/dcAmaltheaParser.h"
#include "commons/parser/dcApplication.h"
#include "commons/parser/dcRunnableInstance.h"

namespace dreamcloud {
namespace platform_sclib {

////////////////////
//      USING     //
////////////////////
using dreamcloud::platform_sclib::noc_ppa::Packet;
using namespace DCApplication;

// Constants for VCD file generation
const string VCD_ACTIVE_RUN_ID("!");
const string VCD_SUSPENDED_ON_REQUEST("{");
const string VCD_PREEMPTION("[");
const string VCD_RUNNABLE_COMPLETED("*");
const string VCD_DEADLINE_MISSED("(");
const string VCD_12_UNDEF("bxxxxxxxxxxxx");

class processingElement: sc_module {
public:

	// Interface for sending/receiving packets
	sc_in<Packet> packet_in;
	sc_out<Packet> packet_out;
	sc_in<bool> canReceivePkt_signal;
	sc_out<bool> newPacket_signal;

	// Interface to receive runnables from mapper
	dcRunnableInstance* newRunnable;
	bool newRunnableSignal = false;
	sc_event *newRunnable_event;

	sc_event *runnableCompleted_event;
	sc_event *packetToSend_event;

	unsigned int x_PE, y_PE;

	int nextReadRequestId = 1;
	vector<dcRunnableInstance *> completedRunInstances;

	// Global labels mapping table
	vector<std::pair<std::pair<int, int>, string> > labelsMappingTable;

	// Scheduling strategy
	enum SchedulingStrategy {
		FCFS, PRIO
	};
	const SchedulingStrategy sched;

	// Statistics about what happened on the core
	unsigned int nbLocRds;
	unsigned int nbLocWrs;
	unsigned int nbRemRds;
	unsigned int nbRemWrs;
	unsigned long int bytesLocRds;
	unsigned long int bytesLocWrs;
	unsigned long int bytesRemRds;
	unsigned long int bytesRemWrs;
	unsigned long int computationTime;
	int deadlinesMissed = 0;
	int nbRunnablesCompleted = 0;

	SC_HAS_PROCESS(processingElement);
	processingElement(sc_module_name name, FILE *constInstsCsvFile_,
			FILE *nocTrafficCsvFile_, ofstream *nocTrafficArffFile_,
			ofstream *runnablesCsvFile_, ofstream *runnablesArffFile_,
			ofstream *runnablesVcdFile_, SchedulingStrategy sched_,
			dcSimuParams params_, unsigned long int nbCyclesPerInstructions) :
			sc_module(name), sched(sched_), nbLocRds(0), nbLocWrs(0), nbRemRds(
					0), nbRemWrs(0), bytesLocRds(0), bytesLocWrs(0), bytesRemRds(
					0), bytesRemWrs(0), computationTime(0), instsCsvFile(
					constInstsCsvFile_), nocTrafficCsvFile(nocTrafficCsvFile_), nocTrafficArffFile(
					nocTrafficArffFile_), runnablesCsvFile(runnablesCsvFile_), runnablesArffFile(
					runnablesArffFile_), runnablesVcdFile(runnablesVcdFile_), params(
					params_), type(
					processingElementType(params_.getCoresFrequencyInHz(),
							nbCyclesPerInstructions)) {

		// Inits SystemC processes
		SC_THREAD(pktSender_thread);
		set_stack_size(TH_STACK_SIZE);
		sensitive << sendPacket_event;
		SC_METHOD(pktReceiver_method);
		sensitive << packet_in;
		dont_initialize();
		SC_THREAD(runnableExecuter_thread);
		set_stack_size(TH_STACK_SIZE);

		// Inits the generator for deviation instructions
		int seed;
		if (params.getRandomNonDet()) {
			seed = time(NULL);
		} else {
			seed = 1242;
		}
		gen = std::mt19937(seed);
	}

	inline ~processingElement() {
	}
	void dumpNoCTraces();

//	// Type definitions for managing runnables preemption
//	typedef pair<unsigned int, dcRunnableInstance*> runnableExecStatus; // instruction index, runnable
//	typedef pair<int, runnableExecStatus> runnableExecElement; // prio, instruction index, runnable
//
//	// Type definitions for managing runnables blocked on read
//	typedef pair<int, runnableExecStatus> blockedRunnableExecElement; // read request id, instruction index, runnable
//	typedef pair<pair<int, int>, int> responsePackets; // sender, nb of packets rcvd
//	typedef pair<responsePackets, pair<int, blockedRunnableExecElement> > runnableBlockedOnRemoteRead; // sender, nb of packets rcvd, preemption element, read request id, instruction index, runnable

	// Type definitions for managing runnables preemption
	struct runnableExecElement {
		int instructionIdx;
		dcRunnableInstance *runInstance;
		int priority;
	};
	struct runnableBlockedOnRemoteRead {
		 unsigned int readRequestId;
		 runnableExecElement execElem;
		 unsigned int xReplier;
		 unsigned int yReplier;
		 int nbPktReceived;
		 int nbPktToBeReceived;
	};

private:
	// SystemC processes
	void pktReceiver_method();
	void pktSender_thread();
	void runnableExecuter_thread();

	// Functions to execute runnable items
	void executeInstructionsConstant(dcInstruction *inst,
			dcRunnableInstance *run, int instructionId);
	void executeInstructionsDeviation(dcInstruction *inst,
			dcRunnableInstance *run, int instructionId);
	void executeRemoteLabelWrite(dcRemoteAccessInstruction *rinst,
			dcRunnableInstance *run, int writeRequestId, int x, int y);
	void executeRemoteLabelRead(dcRemoteAccessInstruction *rinst,
			dcRunnableInstance *run, int x, int y, int instructionId, int priority);

	// Internal functions
	void addReadyRunnable(runnableExecElement runnableExecElem);
	void removeReadyRunnable(dcRunnableInstance *runnable);

	typedef struct pktQElement {
		int priority;
		std::pair<int, int> source;
		std::pair<int, int> destination;

		// The two following fields identify the packet among:
		//   - A read  request
		//   - A read  response
		//   - A write request
		bool write; // if true, the packet belongs to a write else it belongs to a read
		bool readResponse; // for packets belonging to read, this boolean says if it's the request or the response
		int readRequestId; // this field is used to correlate read requests with read responses

		int requestedSize;
		int writeSize;
		int write_rq_ID; // Different ID for each packet of same write instruction
		int write_rq_size;
		int write_request_ID; //same ID for whole write instruction
	} pktQElement;

	// Used by the core to notify the "router" (i.e. packetSender SC_THREAD)
	sc_event sendPacket_event;

	// Output files
	FILE *instsCsvFile;
	FILE *nocTrafficCsvFile;
	ofstream *nocTrafficArffFile;
	ofstream *runnablesCsvFile;
	ofstream *runnablesArffFile;
	ofstream *runnablesVcdFile;

	// Simulation parameters
	dcSimuParams params;

	// Random number generator for deviation instructions
	std::mt19937 gen;

	// List of noc traffic strings to be dump at the end of the simu a file
	vector<char*> nocTrafficStrings;

	// Local packets list where the core write pkts to be sent and where the "router" read them
	vector<pktQElement> localPktsOut;

	// Management of the execution of runnables on this PE
	vector<runnableExecElement> readyRunnables;
	vector<runnableBlockedOnRemoteRead> runnablesBlockedOnRemoteRead;

	// PE's type
	processingElementType type;
};

}
}

#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
