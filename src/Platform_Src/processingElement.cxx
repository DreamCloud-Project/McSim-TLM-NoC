////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include "processingElement.hxx"
#include "NoC/nocInterconnect.hxx"
#include <math.h> 
#include <bitset>

////////////////////
//      USING     //
////////////////////
using namespace std;

namespace dreamcloud {
namespace platform_sclib {

/**
 * Add the given runnable execution element to th ready list.
 * This function preserve ready runnables list order according
 * to the priority of the runnable execution element.
 */
void processingElement::addReadyRunnable(runnableExecElement runnableExecElem) {
	readyRunnables.insert(readyRunnables.begin(), runnableExecElem);
	std::sort(readyRunnables.begin(), readyRunnables.end(),
			[](const runnableExecElement &left, const runnableExecElement &right)
			{
				return left.priority < right.priority;
			});
}

/**
 * SystemC method sensitive to the writeAccessToNoCTraces signal that
 * dump all the NoC traffic strings recorded during simulation into a file.
 * This signal is raised to true by dcSystem at the end of the simu.
 * TODO: do the writing in the output file during simulation as for other files
 */
void processingElement::dumpNoCTraces() {
	for (vector<char*>::size_type i = 0; i < nocTrafficStrings.size(); i++) {
		fputs(nocTrafficStrings.at(i), nocTrafficCsvFile);
	}
}

/**
 * Execute the given constant number of instructions by
 * waiting. Also log information for energy model.
 */
void processingElement::executeInstructionsConstant(dcInstruction *inst,
		dcRunnableInstance *run, int instructionId) {
	dcExecutionCyclesConstantInstruction* einst =
			static_cast<dcExecutionCyclesConstantInstruction*>(inst);
	double waitTimeInNano = 1E9 * einst->GetValue()
			* type.getNbCyclesPerInstructions() / type.getFrequencyInHz();
	wait(waitTimeInNano, SC_NS);
	string exeInstS = run->getRunCall()->GetRunClassName() + " ,"
			+ std::to_string(instructionId) + " ,"
			+ std::to_string(einst->GetValue()) + "\n";
	computationTime += waitTimeInNano;
	const char* exeInstS_const = exeInstS.c_str();
	fputs(exeInstS_const, instsCsvFile);
}

/**
 * Execute the given deviation number of instructions by
 * waiting. Also log information for energy model.
 */
void processingElement::executeInstructionsDeviation(dcInstruction *inst,
		dcRunnableInstance *run, int instructionId) {
	dcExecutionCyclesDeviationInstruction* einst =
			static_cast<dcExecutionCyclesDeviationInstruction*>(inst);
	double UpperBound = 0.0;
	double LowerBound = 0.0;
	if (einst->GetUpperBoundValid()) {
		UpperBound = einst->GetUpperBound();
	}
	if (einst->GetLowerBoundValid()) {
		LowerBound = einst->GetLowerBound();
	}
	std::uniform_int_distribution<> distr(LowerBound, UpperBound);
	int compute_duration = distr(gen);
	double waitTimeInNano = 1E9 * compute_duration
			* type.getNbCyclesPerInstructions() / type.getFrequencyInHz();
	wait(waitTimeInNano, SC_NS);
	computationTime += waitTimeInNano;
	string exeInstS = run->getRunCall()->GetRunClassName() + " ,"
			+ std::to_string(instructionId) + " ,"
			+ std::to_string(compute_duration) + "\n";
	const char* exeInstS_const = exeInstS.c_str();
	fputs(exeInstS_const, instsCsvFile);
}

void processingElement::executeRemoteLabelRead(dcRemoteAccessInstruction *rinst,
		dcRunnableInstance *run, int x, int y, int instructionId,
		int priority) {

	// Create packet representing read request
	pktQElement temp_Packet;
	temp_Packet.priority = priority;
	temp_Packet.readRequestId = nextReadRequestId++; // ID used to identified when we'll receive answer packtes to wich requests they correspond
	temp_Packet.source = make_pair(x_PE, y_PE);
	temp_Packet.destination = make_pair(x, y);
	temp_Packet.write = false;
	temp_Packet.readResponse = false;
	temp_Packet.requestedSize = (int) (ceil(
			double(rinst->GetLabel()->GetSize())
					/ double(8 * PACKET_SIZE_IN_BYTES))); // Packets ;
	temp_Packet.write_rq_ID = 0;
	temp_Packet.write_rq_size = 0;
	temp_Packet.write_request_ID = 0;
	nbRemRds++;
	bytesRemRds += rinst->GetLabel()->GetSize();

	// Put the runnable in the blocked list
	// and remove it from the ready list
	runnableBlockedOnRemoteRead blockedOnRemoteRead;
	runnableExecElement execElem;
	execElem.instructionIdx = instructionId + 1;
	execElem.runInstance = run;
	execElem.priority = priority;
	blockedOnRemoteRead.nbPktReceived = 0;
	blockedOnRemoteRead.nbPktToBeReceived = temp_Packet.requestedSize;
	blockedOnRemoteRead.readRequestId = temp_Packet.readRequestId;
	blockedOnRemoteRead.execElem = execElem;
	blockedOnRemoteRead.xReplier = x;
	blockedOnRemoteRead.yReplier = y;
	runnablesBlockedOnRemoteRead.push_back(blockedOnRemoteRead);
	removeReadyRunnable(run);

	// Notify packet_creater()
	localPktsOut.push_back(temp_Packet);
	sendPacket_event.notify();

	// Sending a packet has a cost of 32 cycles for now:
	// 1 clock cycle for each flit of the packet
	wait(PACKET_SIZE_IN_BYTES, SC_NS);

	// Log runnable block in VCD file
	if (params.getGenerateWaveforms()) {
		unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
		*runnablesVcdFile << "#" << nowInNano << endl;
		*runnablesVcdFile << VCD_12_UNDEF << " " << VCD_ACTIVE_RUN_ID << x_PE
				<< y_PE << endl;
		*runnablesVcdFile << "1" << VCD_SUSPENDED_ON_REQUEST << x_PE << y_PE
				<< endl;
	}
}

void processingElement::executeRemoteLabelWrite(
		dcRemoteAccessInstruction *rinst, dcRunnableInstance *run,
		int writeRequestId, int x, int y) {
	int number_of_Packets = (int) (ceil(
			double(rinst->GetLabel()->GetSize())
					/ double(8 * PACKET_SIZE_IN_BYTES)));
	nbRemWrs++;
	bytesRemWrs += rinst->GetLabel()->GetSize();
	for (int pkts = 0; pkts < number_of_Packets; pkts++) {
		pktQElement temp_Packet;
		temp_Packet.priority = run->getRunCall()->GetPriority();
		temp_Packet.source = make_pair(x_PE, y_PE);
		temp_Packet.destination = make_pair(x, y);
		temp_Packet.write = true;
		temp_Packet.readResponse = false;
		temp_Packet.requestedSize = 0;
		temp_Packet.write_rq_ID = pkts;
		if (pkts == 0) {
			temp_Packet.write_rq_size = number_of_Packets;
		} else {
			temp_Packet.write_rq_size = 0;
		}
		temp_Packet.write_request_ID = writeRequestId;
		localPktsOut.push_back(temp_Packet);
		wait(PACKET_SIZE_IN_BYTES, SC_NS);
	}

	// Notify packet_creater()
	sendPacket_event.notify();
}

/**
 * SC_METHOD sensitive to the packet_in port which
 * is connected to an sc_buffer. It handles the
 * received packets according to 3 situations:
 *
 *   1 Remote write from other PE
 *   2 Remote read from other PE
 *   3 Answer to a remote read originated from this PE
 *
 *   In the situation number 2, some packets need to be sent back
 *   to the requester.
 *
 */
void processingElement::pktReceiver_method() {

	Packet p = packet_in.read();
	p.set_delivery_time();

	// Assert the packet is for me
	if (p.get_destination().first != x_PE
			|| p.get_destination().second != y_PE) {
		cerr << "PE" << x_PE << y_PE << " received a packet targeted at PE"
				<< p.get_destination().first << p.get_destination().second
				<< endl;
		exit(-1);
	}

	// We receive a request for a write to this PE from a remote PE
	// NOTHING TO DO
	if (p.isWrite()) {
	}

	// We receive a read from a remote PE
	// we must send back the response
	else if (!p.isWrite() && !p.isReadResponse()) {
		int number_of_Packets = p.get_requestedSize();
		for (int pkts = 0; pkts < number_of_Packets; pkts++) {
			pktQElement pktElem;
			pktElem.priority = p.get_priority();
			pktElem.readRequestId = p.get_read_request_id();
			pktElem.source = p.get_destination();
			pktElem.destination = p.get_source();
			pktElem.write = false;
			pktElem.requestedSize = 0;
			pktElem.readResponse = true;
			localPktsOut.push_back(pktElem);
		}
		// Notify packet_creater()
		sendPacket_event.notify();
	}

// We receive a response from a previous remote read originated from this PE
// We must move the runnable concerned by this read from the "blocked on remote read"
// runnables list to the "ready" runnables one.
	else if (!p.isWrite() && p.isReadResponse()) {

		vector<runnableBlockedOnRemoteRead>::iterator it;
		it = std::find_if(runnablesBlockedOnRemoteRead.begin(),
				runnablesBlockedOnRemoteRead.end(),
				[&p](const runnableBlockedOnRemoteRead& elem)
				{
					return (elem.xReplier == p.get_source().first &&
							elem.yReplier == p.get_source().second &&
							elem.readRequestId == p.get_read_request_id());
				});

		// If this PE doesn't have a runnable blocked on the remote read
		if (it == runnablesBlockedOnRemoteRead.end()) {
			cerr << "PE" << x_PE << y_PE << " received a read response from PE"
					<< p.get_source().first << p.get_source().second
					<< " that he didn't initiated with request ID = "
					<< p.get_read_request_id() << endl;
			exit(-1);
		}

		// We receive a new packet for the remote read
		// If all the packets have been received, then we can
		// unblock the runnable.
		(it->nbPktReceived)++;
		if (it->nbPktReceived == it->nbPktToBeReceived) {
			addReadyRunnable(it->execElem);
			runnablesBlockedOnRemoteRead.erase(it);
			newRunnable_event->notify();
		}
	}

	// Save noc string to be dump later
	string sPacket = p.to_str();
	const char *cstr = sPacket.c_str();
	char* sPacketBuff = new char[(sPacket.length()) + 1];
	strcpy(sPacketBuff, cstr);
	nocTrafficStrings.push_back(sPacketBuff);

	// Dump into Noc Arff file for Weka
	*nocTrafficArffFile << p.get_id() << ",";
	*nocTrafficArffFile << p.get_priority() << ",";
	*nocTrafficArffFile << p.get_read_request_id() << ",";
	*nocTrafficArffFile << "(" << p.get_source().first << " "
			<< p.get_source().second << "),";
	*nocTrafficArffFile << "(" << p.get_destination().first << " "
			<< p.get_destination().second << "),";
	*nocTrafficArffFile << fixed << p.get_injection_time() << ",";
	*nocTrafficArffFile << fixed << p.get_delivery_time() << ",";
	*nocTrafficArffFile << fixed << p.get_latency() << ",";
	*nocTrafficArffFile << fixed << p.get_latency_no_contention() << endl;
}

/**
 * This SC_THREAD sends packet for remote read
 * and remote write request. It's sensitive
 * to the sendPacket_event event which is
 * notified immediately by the runnableExecuter
 * in 3 cases:
 *
 * 1- This core executes a remote read
 * 2- This core executes a remote write
 * 3- This core answers to a remote read from another core
 */
void processingElement::pktSender_thread() {

	while (true) {

		// We can have several packets to send
		// when we reply to a remote read request
		// or when we reply to a remote request AND
		// this core is making a remote request
		// So we send all of them
		while (!localPktsOut.empty()) {

			// Convert the local packet to a one for the NoC
			// TODO: why do we need another packet class ?
			pktQElement localPkt = localPktsOut.front();
			Packet nocPkt;
			nocPkt.set_priority(localPkt.priority);
			nocPkt.set_read_request_id(localPkt.readRequestId);
			nocPkt.set_source(localPkt.source);
			nocPkt.set_destination(localPkt.destination);
			nocPkt.set_rd_wr(localPkt.write);
			nocPkt.set_requestedSize(localPkt.requestedSize);
			nocPkt.set_req_resp(localPkt.readResponse);
			nocPkt.set_writeSize(localPkt.writeSize);
			nocPkt.set_write_rq_ID(localPkt.write_rq_ID);
			nocPkt.set_write_rq_size(localPkt.write_rq_size);
			nocPkt.set_write_request_ID(localPkt.write_request_ID);
			nocPkt.set_injection_time();

			// Wait for dcSystem to be able to receive a packet
			while (!canReceivePkt_signal.read()) {
				wait(params.getCoresPeriodInNano(), SC_NS);
			}

			// Send the packet to dcSystem
			packet_out.write(nocPkt);
			newPacket_signal.write(true);
			packetToSend_event->notify();

			// Wait for dcSystem ack
			while (canReceivePkt_signal.read()) {
				wait(params.getCoresPeriodInNano(), SC_NS);
			}

			// The packet has been sent
			newPacket_signal.write(false);
			localPktsOut.erase(localPktsOut.begin());
		}
		wait(); // wait next event
	}
}

/**
 * SystemC thread that execute an instruction of the current
 * runnable and then check for preemption.
 */
void processingElement::runnableExecuter_thread() {

	int writeRequestId = 0;

	while (true) {

		// Wait for runnables from dcSystem
		if (readyRunnables.empty() && !newRunnableSignal) {
			wait(*newRunnable_event);
		}

		// If a new runnable has been sent during the execution of the last instruction
		// Put it in the ready list
		// ONE clock cycle
		if (newRunnableSignal) {
			int prio;
			if (sched == PRIO) {
				prio = newRunnable->getRunCall()->GetPriority();
			} else if (sched == FCFS) {
				prio = newRunnable->GetMappingTime();
			}
			runnableExecElement execElement;
			execElement.instructionIdx = 0;
			execElement.runInstance = newRunnable;
			execElement.priority = prio;
			newRunnable->SetCoreReceiveTime(sc_time_stamp().value());
			addReadyRunnable(execElement);

			// Log runnable preemption in VCD file
			unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
			if (params.getGenerateWaveforms()) {
				if (readyRunnables.size() > 1
						&& readyRunnables.front().runInstance->GetUniqueID()
								== newRunnable->GetUniqueID()) {
					*runnablesVcdFile << "#" << nowInNano << endl;
					*runnablesVcdFile << "1" << VCD_PREEMPTION << x_PE << y_PE
							<< endl;
				}
			}

			newRunnableSignal = false;
			wait(1, SC_NS);
		}

		// Choose the runnable with the highest priority among ready ones
		// Operation in ZERO clock cycle
		runnableExecElement execElem = readyRunnables.front();
		dcRunnableInstance *currentRunnable = execElem.runInstance;
		unsigned int instructionId = execElem.instructionIdx;
		vector<dcInstruction *> instructions =
				currentRunnable->getRunCall()->GetAllInstructions();
		unsigned int nbInstructions = instructions.size();
		bool blockedOnRemoteRead = false;

		// Move to next instruction for next time this runnable will be executed
		readyRunnables.front().instructionIdx++;

		// If the next instruction to execute exists.
		// This checks is required because when we block a runnable
		// on a remote read and its the last instruction, when we unblock it
		// we are above the last instruction
		if (instructionId < nbInstructions) {
			dcInstruction* inst = instructions.at(instructionId);
			string instrName = inst->GetName();
			unsigned long int nowInPico = sc_time_stamp().value();
			if (instructionId == 0) {
				currentRunnable->SetStartTime(nowInPico);
			}

			// Log runnable activation in VCD file
			if (params.getGenerateWaveforms()) {
				std::bitset<12> binId(
						currentRunnable->getRunCall()->GetWaveID());
				*runnablesVcdFile << "#" << (nowInPico * 1E-3) << " b" << binId
						<< " " << VCD_ACTIVE_RUN_ID << x_PE << y_PE << endl;
			}

			// Label accesses
			if (instrName == "sw:LabelAccess") {

				// Search where is the label
				dcRemoteAccessInstruction *rinst =
						static_cast<dcRemoteAccessInstruction*>(inst);
				string labelName = rinst->GetLabel()->GetName();
				vector<std::pair<std::pair<int, int>, string> >::iterator it;
				it =
						std::find_if(labelsMappingTable.begin(),
								labelsMappingTable.end(),
								[&labelName](const std::pair< std::pair<int, int>, std::string >& pair)
								{
									return pair.second == labelName;
								});
				unsigned int destX = it->first.first;
				unsigned int destY = it->first.second;

				// Local read and write accesses
				// ONE clock cycle per byte
				if ((x_PE == destX) && (y_PE == destY)) {
					int localAccessSize = (int) (ceil(
							double(rinst->GetLabel()->GetSize()) / double(8)));
					wait(localAccessSize, SC_NS);
					if (rinst->GetWrite()) {
						nbLocWrs++;
						bytesLocWrs += rinst->GetLabel()->GetSize();
					} else {
						nbLocRds++;
						bytesLocRds += rinst->GetLabel()->GetSize();
					}
				}

				// Remote write: sent packets according to the size of the label
				// nbPackets * PACKET_SIZE clock cycles
				else if (rinst->GetWrite()) {
					executeRemoteLabelWrite(rinst, currentRunnable,
							writeRequestId, destX, destY);
					writeRequestId++;
				}

				// Remote read: send one packet including information allowing
				// receiver to send response back
				// PACKET_SIZE clock cycles
				else if (!rinst->GetWrite()) {
					executeRemoteLabelRead(rinst, currentRunnable, destX, destY,
							instructionId, execElem.priority);
					blockedOnRemoteRead = true;
				}
			} // End label access

			// Instruction constant
			else if (instrName == "sw:InstructionsConstant") {
				executeInstructionsConstant(inst, currentRunnable,
						instructionId);
			} else if (instrName == "sw:InstructionsDeviation") {
				executeInstructionsDeviation(inst, currentRunnable,
						instructionId);
			}
		} // End if instructionID < nbInstructions

		// Check if runnable is completed
		if (!blockedOnRemoteRead
				&& (instructionId >= nbInstructions - 1 || nbInstructions == 0)) {

			// Log runnable info in CSV file and arff
			currentRunnable->SetCompletionTime(sc_time_stamp().value());
			unsigned long int runnableExecutionTime =
					currentRunnable->GetCompletionTime()
							- currentRunnable->GetCoreReceiveTime();
			(*runnablesCsvFile) << "PE" << x_PE << y_PE << ",";
			(*runnablesCsvFile)
					<< currentRunnable->getRunCall()->GetRunClassName() << ",";
			(*runnablesCsvFile) << currentRunnable->getRunCall()->GetPriority()
					<< ",";
			(*runnablesCsvFile) << (currentRunnable->GetMappingTime() / 1E3)
					<< ",";
			(*runnablesCsvFile) << (currentRunnable->GetStartTime() / 1E3)
					<< ",";
			(*runnablesCsvFile) << (currentRunnable->GetCompletionTime() / 1E3)
					<< ",";
			(*runnablesCsvFile) << (runnableExecutionTime / 1E3) << ",";
			(*runnablesCsvFile)
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano())
					<< ",";
			(*runnablesCsvFile)
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano()
							- (runnableExecutionTime / 1E3)) << endl;
			(*runnablesArffFile) << "PE" << x_PE << y_PE << ",";
			(*runnablesArffFile) << fixed
					<< currentRunnable->getRunCall()->GetRunClassName() << ",";
			(*runnablesArffFile) << fixed
					<< currentRunnable->getRunCall()->GetPriority() << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->GetMappingTime() / 1E3) << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->GetStartTime() / 1E3) << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->GetCompletionTime() / 1E3) << ",";
			(*runnablesArffFile) << fixed << (runnableExecutionTime / 1E3)
					<< ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano()
							/ 1E3) << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano()
							- (runnableExecutionTime / 1E3)) << endl;

			// Log runnable complete in VCD file
			unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
			if (params.getGenerateWaveforms()) {
				*runnablesVcdFile << "#" << nowInNano << endl;
				*runnablesVcdFile << VCD_12_UNDEF << " " << VCD_ACTIVE_RUN_ID
						<< x_PE << y_PE << endl;
				*runnablesVcdFile << "1" << VCD_RUNNABLE_COMPLETED << x_PE
						<< y_PE << endl;
			}

			// Check deadline miss
			if (runnableExecutionTime * 1E-3
					> currentRunnable->getRunCall()->GetDeadlineValueInNano()) {
				deadlinesMissed++;
				if (params.getGenerateWaveforms()) {
					*runnablesVcdFile << "1" << VCD_DEADLINE_MISSED << x_PE
							<< y_PE << endl;
				}
			}

			// Add the completed runnable to completed queue and remove it from ready one
			nbRunnablesCompleted++;
			completedRunInstances.push_back(currentRunnable);
			(*runnableCompleted_event).notify(SC_ZERO_TIME);
			removeReadyRunnable(currentRunnable);
		}
	}
}

void processingElement::removeReadyRunnable(dcRunnableInstance *runnable) {
	vector<runnableExecElement>::iterator toRemove = std::find_if(
			readyRunnables.begin(), readyRunnables.end(),
			[&runnable](runnableExecElement& elem)
			{
				return elem.runInstance == runnable;
			});
	if (toRemove == readyRunnables.end()) {
		cerr << "ERORRRR " << runnable->getRunCall()->GetRunClassName()
				<< " not found" << endl;
		exit(-1);
	}
	readyRunnables.erase(toRemove);
}

}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
