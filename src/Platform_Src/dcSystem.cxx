////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include "../Platform_Src/dcSystem.hxx"

#include <fstream>
#include <utility>

#include "../Platform_Src/simulators-commons/utils/Math.hxx"

namespace dreamcloud {
namespace platform_sclib {

/**
 * SystemC method used to get completed runnables from PEs and to enable
 * runnables dependent on the completed ones.
 */
void dcSystem::dependentRunnablesReleaser_method() {

	// Copy all completed dcRunnableInstance ids from PEs into dcSystem and erase them in PE_XY
	vector<dcRunnableInstance *> completedRunInstances;
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			processingElement *pe = pes[x][y];
			for (std::vector<dcRunnableInstance *>::const_iterator i =
					pe->completedRunInstances.begin();
					i != pe->completedRunInstances.end(); ++i) {
				completedRunInstances.push_back(*i);
				nbRunnablesCompleted++;
			}
		}
	}

	// When we don't handle periodics stop the simulation when all
	// the runnables (periodic and non periodic) have been executed once
	// or start a new iteration if needed
	if (simulationEndFromMode == 0
			&& (params.dontHandlePeriodic() || hyperPeriod == 0)) {
		if (nbRunnablesCompleted / runnables.size() >= params.getIterations()) {
			stopSimu_event.notify();
			return;
		}
		if ((nbRunnablesCompleted % runnables.size()) == 0) {
			iteration_event.notify(0, SC_NS);
		}
	}

	// Loop over all the completed runnables
	for (vector<dcRunnableInstance *>::size_type i = 0;
			i < completedRunInstances.size(); ++i) {
		// Loop over all runnableCalls that depend on the completed one
		dcRunnableInstance *runInst = completedRunInstances.at(i);
		dcRunnableCall* runCall = runInst->getRunCall();
		vector<dcRunnableCall*> enabledToExecute = runCall->GetListOfEnables();
		for (std::vector<dcRunnableCall*>::iterator it =
				enabledToExecute.begin(); it != enabledToExecute.end(); ++it) {
			// If all the previous runnables of the current dependent are completed, release it (the dependent one)
			if ((*it)->GetEnabledBy() == (*it)->GetListOfEnablers().size()) {
				dcRunnableInstance * runInst = new dcRunnableInstance(*it);
				releaseRunnable(runInst);
			}
			// Else only indicate that one dependency has been satisfied
			else {
				(*it)->SetEnabledBy((*it)->GetEnabledBy() + 1);
			}
		}
	}

	// Delete all completed runnable instances
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			processingElement *pe = pes[x][y];
			for (std::vector<dcRunnableInstance *>::const_iterator i =
					pe->completedRunInstances.begin();
					i != pe->completedRunInstances.end(); ++i) {
				delete (*i);
			}
			pe->completedRunInstances.clear();
		}
	}
}

void dcSystem::dumpCoresLoadGraphFile(unsigned long int time) {
	ofstream f(params.getOutputFolder() + "/coresLoad.gv");
	f << "digraph CoresLoad {" << endl;
	f << "graph [splines=ortho]" << endl;
	ofstream f1(params.getOutputFolder() + "/coresDeadMissed.gv");
	f1 << "digraph CoresDeadMissed {" << endl;
	f1 << "graph [splines=ortho]" << endl;
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			int yS = -y;
			double utilization = (pes[x][y]->computationTime * 100.0) / time;
			f << x << y
					<< "[style=filled, shape=box, style=\"rounded, filled\", fillcolor=gray, label=\"("
					<< x << "," << y << ")\\n" << ((int) utilization)
					<< "%\", pos=\"" << 1.5 * x << "," << 1.5 * yS << "!\"];"
					<< endl;
			double percentMiss = (pes[x][y]->deadlinesMissed * 100.0)
					/ pes[x][y]->nbRunnablesCompleted;
			f1 << x << y
					<< "[style=filled, shape=box, style=\"rounded, filled\", fillcolor=gray, label=\"("
					<< x << "," << y << ")\\n" << pes[x][y]->deadlinesMissed
					<< "/" << pes[x][y]->nbRunnablesCompleted << "\\n"
					<< setprecision(2) << percentMiss << "%\", pos=\""
					<< 1.5 * x << "," << 1.5 * yS << "!\"];" << endl;
		}
	}
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			// Right edge
			if (x < params.getRows() - 1) {
				f << x << y << "->" << (x + 1) << y << endl;
				f1 << x << y << "->" << (x + 1) << y << endl;
			}
			// Left edge
			if (x >= 1) {
				f << x << y << "->" << (x - 1) << y << endl;
				f1 << x << y << "->" << (x - 1) << y << endl;
			}
			// Up edge
			if (y >= 1) {
				f << x << y << "->" << x << (y - 1) << endl;
				f1 << x << y << "->" << x << (y - 1) << endl;
			}
			// Bottom edge
			if (y < params.getRows() - 1) {
				f << x << y << "->" << x << (y + 1) << endl;
				f1 << x << y << "->" << x << (y + 1) << endl;
			}
		}
	}
	f << "}";
	f.close();
	f1 << "}";
	f1.close();
}

void dcSystem::dumpNoCLoadGraphFile(unsigned long int time) {
	ofstream f(params.getOutputFolder() + "/nocLoadPcktsCount.gv");
	ofstream f1(params.getOutputFolder() + "/nocLoadDistrib.gv");
	f << "digraph NoCLoad {" << endl;
	f << "graph [splines=ortho]" << endl;
	f1 << "digraph NoCLoad {" << endl;
	f1 << "graph [splines=ortho]" << endl;
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			int yS = -y;
			f << x << y
					<< "[style=filled, shape=box, style=\"rounded, filled\", fillcolor=gray, label=\"("
					<< x << "," << y << ")\", pos=\"" << 1.5 * x << ","
					<< 1.5 * yS << "!\"];" << endl;
			f1 << x << y
					<< "[style=filled, shape=box, style=\"rounded, filled\", fillcolor=gray, label=\"("
					<< x << "," << y << ")\", pos=\"" << 1.5 * x << ","
					<< 1.5 * yS << "!\"];" << endl;
		}
	}
	double maxPckCount = 0;
	typedef map<int, unsigned int>::const_iterator it_type;
	for (it_type iterator = noc_->routeToNbPkts.begin();
			iterator != noc_->routeToNbPkts.end(); iterator++) {
		if (iterator->second > maxPckCount) {
			maxPckCount = iterator->second;
		}
	}
	int maxFlitCount = maxPckCount
			* (PACKET_SIZE_IN_BYTES / FLIT_SIZE_IN_BYTES);
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			// Right edge
			if (x < params.getRows() - 1) {
				int route = pairing_function(pairing_function(y, x),
						pairing_function(y, x + 1));
				double load;
				if (noc_->routeToNbPkts.find(route)
						== noc_->routeToNbPkts.end()) {
					load = 0;
				} else {
					load = noc_->routeToNbPkts[route]
							* (PACKET_SIZE_IN_BYTES / FLIT_SIZE_IN_BYTES);
				}
				f << x << y << "->" << (x + 1) << y << "[label=\"" << load
						<< "\"];" << endl;
				f1 << x << y << "->" << (x + 1) << y << "[label=\""
						<< setprecision(2) << load / maxFlitCount << "\"];"
						<< endl;
			}
			// Left edge
			if (x >= 1) {
				int route = pairing_function(pairing_function(y, x),
						pairing_function(y, x - 1));
				double load;
				if (noc_->routeToNbPkts.find(route)
						== noc_->routeToNbPkts.end()) {
					load = 0;
				} else {
					load = noc_->routeToNbPkts[route]
							* (PACKET_SIZE_IN_BYTES / FLIT_SIZE_IN_BYTES);
				}
				f << x << y << "->" << (x - 1) << y << "[label=\"" << load
						<< "\"];" << endl;
				f1 << x << y << "->" << (x - 1) << y << "[label=\""
						<< setprecision(2) << load / maxFlitCount << "\"];"
						<< endl;
			}
			// Up edge
			if (y >= 1) {
				int route = pairing_function(pairing_function(y, x),
						pairing_function(y - 1, x));
				double load;
				if (noc_->routeToNbPkts.find(route)
						== noc_->routeToNbPkts.end()) {
					load = 0;
				} else {
					load = noc_->routeToNbPkts[route]
							* (PACKET_SIZE_IN_BYTES / FLIT_SIZE_IN_BYTES);
				}
				f << x << y << "->" << x << (y - 1) << "[label=\"" << load
						<< "\"];" << endl;
				f1 << x << y << "->" << x << (y - 1) << "[label=\""
						<< setprecision(2) << load / maxFlitCount << "\"];"
						<< endl;
			}
			// Bottom edge
			if (y < params.getRows() - 1) {
				int route = pairing_function(pairing_function(y, x),
						pairing_function(y + 1, x));
				double load;
				if (noc_->routeToNbPkts.find(route)
						== noc_->routeToNbPkts.end()) {
					load = 0;
				} else {
					load = noc_->routeToNbPkts[route]
							* (PACKET_SIZE_IN_BYTES / FLIT_SIZE_IN_BYTES);
				}
				f << x << y << "->" << x << (y + 1) << "[label=\"" << load
						<< "\"];" << endl;
				f1 << x << y << "->" << x << (y + 1) << "[label=\""
						<< setprecision(2) << load / maxFlitCount << "\"];"
						<< endl;
			}
		}
	}
	f << "}";
	f.close();
	f1 << "}";
	f1.close();
}

/**
 * This SC_METHOD is called each time a mode switch occur
 * using dynamic sensitivity
 */
void dcSystem::modeSwitcher_thread() {
	vector<mode_t>::size_type modIdx = 0;
	while (true) {
		unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
		if (modes.at(modIdx).name.compare("end")) {
			mappingHeuristic->switchMode(nowInNano, modes.at(modIdx).file,
					modes.at(modIdx).name);
			labelsMapper_method();
			updateInstructionsExecTimeBounds(modes.at(modIdx).file);
		}
		assert(
				modes.at(modIdx).time == nowInNano
						&& "Invalid time in modeSwitcher_thread");
		if (modIdx + 1 < modes.size()) {
			unsigned long int waitTime = modes.at(modIdx + 1).time - nowInNano;
			modIdx++;
			wait(waitTime, SC_NS);
		} else {
			stopSimu_event.notify();
			break;
		}
	}
}

/**
 * SC_METHOD that release all independent runnables.
 * This method is called first at time zero and then on each
 * iteration_event notification.
 */
void dcSystem::nonPeriodicIndependentRunnablesReleaser_method() {
	vector<dcRunnableCall *> runs;
	if (params.dontHandlePeriodic()) {
		runs = application->GetIndependentRunnables(taskGraph);
	} else {
		runs = application->GetIndependentNonPeriodicRunnables(taskGraph);
	}
	if (params.dontHandlePeriodic() && runs.size() == 0) {
		cerr << params.getAppXml()
				<< " doesn't contain any independent runnable to start, please check it !"
				<< endl;
		exit(-1);
	}
	for (std::vector<dcRunnableCall *>::iterator it = runs.begin();
			it != runs.end(); ++it) {
		dcRunnableInstance * runInst = new dcRunnableInstance(*it);
		releaseRunnable(runInst);
	}
}

/**
 * SystemC thread used to release periodic runnables.
 * This thread is triggered at initialization time and then dynamically
 * computes its next occurrence.
 */
void dcSystem::periodicRunnablesReleaser_thread() {

	// Only used in periodic handling case
	if (params.dontHandlePeriodic()) {
		return;
	}

	vector<int>::size_type nbPeriodicRunnables =
			periodicAndSporadicRunnables.size();
	vector<unsigned long int> remainingTimeBeforeRelease(nbPeriodicRunnables);
	for (vector<int>::size_type i = 0; i < nbPeriodicRunnables; ++i) {
		dcRunnableCall *runnable = periodicAndSporadicRunnables.at(i);
		remainingTimeBeforeRelease[i] = runnable->GetOffsetInNano();
	}

	while (true) {

		// Stop the simu if hyper period reached and we are not using mode switch
		if (simulationEndFromMode == 0 && !params.dontHandlePeriodic()
				&& hyperPeriod > 0
				&& (sc_time_stamp().value() / 1E3) >= hyperPeriod) {
			stopSimu_event.notify();
			return;
		}

		// Stop the simu if specified simu time is done
		if (simulationEndFromCmdLine != 0
				&& (sc_time_stamp().value() / 1E3)
						>= simulationEndFromCmdLine) {
			stopSimu_event.notify();
			return;
		}

		unsigned long int waitTime = ULONG_MAX;
		for (vector<int>::size_type i = 0; i < nbPeriodicRunnables; ++i) {
			if (remainingTimeBeforeRelease[i] == 0) {
				dcRunnableCall *runnable = periodicAndSporadicRunnables.at(i);
				dcRunnableInstance * runInst = new dcRunnableInstance(runnable);
				releaseRunnable(runInst);
				remainingTimeBeforeRelease[i] = runnable->GetPeriodInNano();
			}
			if (remainingTimeBeforeRelease[i] < waitTime) {
				waitTime = remainingTimeBeforeRelease[i];
			}
		}
		for (vector<int>::size_type i = 0; i < nbPeriodicRunnables; ++i) {
			remainingTimeBeforeRelease[i] = remainingTimeBeforeRelease[i]
					- waitTime;
		}
		wait(waitTime, SC_NS);
	}
}

/**
 * Return a human readable string of the given frequency.
 */
string dcSystem::getFrequencyString(unsigned long int freqInHertz) {
	if (freqInHertz < 1E3) {
		return std::to_string(freqInHertz) + " Hz";
	} else if (freqInHertz >= 1E3 && freqInHertz < 1E6) {
		return std::to_string((unsigned long int) (freqInHertz / 1E3)) + " KHz";
	} else if (freqInHertz >= 1E6 && freqInHertz < 1E9) {
		return std::to_string((unsigned long int) (freqInHertz / 1E6)) + " MHz";
	} else if (freqInHertz >= 1E9) {
		return std::to_string((unsigned long int) (freqInHertz / 1E9)) + " GHz";
	} else {
		cerr << "Invalid Frequency in hertz" << freqInHertz << endl;
		exit(-1);
	}
}

/**
 * SystemC method called once only during initialization.
 */
void dcSystem::labelsMapper_method() {
	for (vector<dcLabel*>::size_type i = 0; i < labels.size(); i++) {
		dcMappingHeuristicI::dcMappingLocation loc = mappingHeuristic->mapLabel(
				labels.at(i)->GetID(), sc_time_stamp().value() * 1E-3,
				labels.at(i)->GetName());
		// All the PEs have a local copy of the labels mapping table
		for (unsigned int row(0); row < params.getRows(); ++row) {
			for (unsigned int col(0); col < params.getCols(); ++col) {
				pes[row][col]->labelsMappingTable.push_back(
						std::make_pair(loc, labels.at(i)->GetName()));
			}
		}
	}
}

void dcSystem::runnablesMapper_thread() {

	int appIterations = 0;
	int remainingNbRunnablesToMap = runnables.size();
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			pes[row][col]->newRunnableSignal = false;
		}
	}

	// Map runnables until application is "finished"
	while (true) {

		// Wait until a runnable can be executed or we reached hyper period
		if (readyRunnables.empty()) {
			wait();
			continue;
		}

		// Ask the mapping heuristic where to map the current ready runnable
		dcRunnableCall * runCall = readyRunnables.front()->getRunCall();
		dcMappingHeuristicI::dcMappingLocation pe =
				mappingHeuristic->mapRunnable(sc_time_stamp().value(),
						runCall->GetRunClassId(), runCall->GetRunClassName(),
						runCall->GetTask()->GetName(),
						runCall->GetTask()->GetID(), runCall->GetIdInTask(),
						readyRunnables.front()->GetPeriodId());
		int x = pe.first;
		int y = pe.second;

		// If the PE has completed receiving the previous runnable,
		// map the new one on it
		// ONE clock cycle
		if (!pes[x][y]->newRunnableSignal) {
			mappingHeuristic->runnableMapped(sc_time_stamp().value(),
					runCall->GetRunClassId());
			runnablesMappingCsvFile
					<< readyRunnables.front()->getRunCall()->GetRunClassId()
					<< ","
					<< readyRunnables.front()->getRunCall()->GetRunClassName()
					<< "," << x << y << endl;
			nbRunnablesMapped++;
			readyRunnables.front()->SetMappingTime(sc_time_stamp().value());
			pes[x][y]->newRunnable = readyRunnables.front();
			pes[x][y]->newRunnableSignal = true;
			newRunnable_event[x][y].notify();
			readyRunnables.erase(readyRunnables.begin());
			remainingNbRunnablesToMap--;
			if (remainingNbRunnablesToMap == 0) {
				appIterations++;
				remainingNbRunnablesToMap = runnables.size();
			}
			wait(1, SC_NS);
		}

		// Else move the current runnable to the end of the independent queue
		// ONE clock cycle
		else {
			readyRunnables.push_back(readyRunnables.front());
			readyRunnables.erase(readyRunnables.begin());
			wait(1, SC_NS);
		}
	}
}

void dcSystem::parseModeFile(string file) {
	std::ifstream infile(file);
	std::string line;
	while (std::getline(infile, line)) {
		istringstream is(line);
		string timeString;
		string modeName;
		string modeFile;
		getline(is, timeString, ';');
		getline(is, modeName, ';');
		getline(is, modeFile, ';');
		string::size_type sz;
		unsigned long int timeInNano = stod(timeString, &sz) * 1E9;
		mode_t mode { timeInNano, modeName, modeFile };
		modes.push_back(mode);
	}
}

/**
 * SystemC thread that receive packets from the processing elements
 * and forward them to the NoC.
 * TODO: remove the dcSystem layer between cores and NoC
 */
void dcSystem::pktFromPeReceiver_cthread() {

	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			trReady[row][col].write(false);
		}
	}
	wait();

	while (true) {

		// Copy packets from PEs in a local vector
		// 1 clock cycle
		vector<Packet> intermediateQ;
		for (unsigned int x(0); x < params.getRows(); ++x) {
			for (unsigned int y(0); y < params.getCols(); ++y) {
				if (newPktFromPe[x][y].read()) {
					Packet pkt = pktsFromPe[x][y].read();
					intermediateQ.push_back(pkt);
					trReady[x][y].write(false); // indicates the PE that dcSystem has received the pkt
				}
			}
		}
		wait(1);

		// Control injection rate.
		while ((nbPacketSendInNoc - nbpacketReceivedFromNoc)
				> (params.getRows() * params.getCols() / 2)) {
			wait();
		}

		// Send the packets into the NoC
		// 1 clock cycle
		for (vector<Packet>::size_type i = 0; i < intermediateQ.size(); i++) {
			commIn.write(intermediateQ.at(i));
			nbPacketSendInNoc++;
			wait(SC_ZERO_TIME);
		}
		wait();

		// Say to PEs that we can receive again packets
		// 1 clock cycle
		for (unsigned int row(0); row < params.getRows(); ++row) {
			for (unsigned int col(0); col < params.getCols(); ++col) {
				trReady[row][col].write(true);
			}
		}
		wait();

		// Wait for new packets to be sent
		bool w = true;
		for (unsigned int x(0); x < params.getRows(); ++x) {
			for (unsigned int y(0); y < params.getCols(); ++y) {
				if (newPktFromPe[x][y].read()) {
					w = false;
					break;
				}
			}
			if (!w) {
				break;
			}
		}
		if (w) {
			wait(packetToSend_event);
		}
	}
}

/**
 * Receive packets from NoC and forward them to cores
 * TODO: remove the dcSystem layer between cores and NoC
 */
void dcSystem::pktToPeDeliverer_method() {
	Packet temp = commOut;
	nbpacketReceivedFromNoc++;
	packet_local_.push_back(temp);
	while (!packet_local_.empty()) {
		pair<unsigned int, unsigned int> delivered_destination =
				packet_local_.front().get_destination();
		pktsToPe[delivered_destination.first][delivered_destination.second] =
				packet_local_.front();
		packet_local_.erase(packet_local_.begin());
	}
}

void dcSystem::releaseRunnable(dcRunnableInstance *runnable) {
	runnable->SetReleaseTime(sc_time_stamp().value());
	readyRunnables.push_back(runnable);
	runnableReleased_event.notify();
}

void dcSystem::stopSimu_thread() {

	// In the case of periodic runnables
	// handling,  wait for all mapped ones to be
	// completed
	if (!params.dontHandlePeriodic()) {
		while (nbRunnablesCompleted < nbRunnablesMapped) {
			wait(runnableCompleted_event);
		}
	}

	// Wait for all the packets sent to the NoC to be delivered
	while (nbPacketSendInNoc > nbpacketReceivedFromNoc) {
		wait(commOut.value_changed_event());
	}

	// Get end time
	clock_t end = std::clock();

	// Prints label access results
	ofstream f(params.getOutputFolder() + "/labels.csv");
	f << "PE,";
	f << "Nb bytes for local reads,";
	f << "Nb of local reads,";
	f << "Nb bytes for local writes,";
	f << "Nb of local writes,";
	f << "Nb bytes for remote reads,";
	f << "Nb of remote reads,";
	f << "Nb bytes for remote writes,";
	f << "Nb of remote write,";
	f << "Total computation time" << endl;
	unsigned int nbLocRds = 0;
	unsigned int nbLocWrs = 0;
	unsigned int nbRemRds = 0;
	unsigned int nbRemWrs = 0;
	unsigned long int bytesLocRds = 0;
	unsigned long int bytesLocWrs = 0;
	unsigned long int bytesRemRds = 0;
	unsigned long int bytesRemWrs = 0;
	unsigned long int computationTime = 0;
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			f << "PE" << row << col << ",";
			f << pes[row][col]->bytesLocRds << ",";
			f << pes[row][col]->nbLocRds << ",";
			f << pes[row][col]->bytesLocWrs << ",";
			f << pes[row][col]->nbLocWrs << ",";
			f << pes[row][col]->bytesRemRds << ",";
			f << pes[row][col]->nbRemRds << ",";
			f << pes[row][col]->bytesRemWrs << ",";
			f << pes[row][col]->nbRemWrs << ",";
			f << pes[row][col]->computationTime << endl;
			bytesLocRds += pes[row][col]->bytesLocRds;
			bytesLocWrs += pes[row][col]->bytesLocWrs;
			bytesRemRds += pes[row][col]->bytesRemRds;
			bytesRemWrs += pes[row][col]->bytesRemWrs;
			nbLocRds += pes[row][col]->nbLocRds;
			nbLocWrs += pes[row][col]->nbLocWrs;
			nbRemRds += pes[row][col]->nbRemRds;
			nbRemWrs += pes[row][col]->nbRemWrs;
			computationTime += pes[row][col]->computationTime;
		}
	}
	f << "Total Nb bytes for local reads = " << bytesLocRds << endl;
	f << "Total Nb of local reads = " << nbLocRds << endl;
	f << "Total Nb bytes for local writes = " << bytesLocWrs << endl;
	f << "Total Nb of local writes = " << nbLocWrs << endl;
	f << "Total Nb bytes for remote reads = " << bytesRemRds << endl;
	f << "Total Nb of remote reads = " << nbRemRds << endl;
	f << "Total Nb bytes for remote writes = " << bytesRemWrs << endl;
	f << "Total Nb of remote writes = " << nbRemWrs << endl;
	f.close();

	// Print some results in a file for energy estimation
	FILE *Parameters = fopen(
			(params.getOutputFolder() + "/Parameters.txt").c_str(), "w+");
	unsigned long int endTimeInNano = sc_time_stamp().value() / 1E3;
	string timeString = static_cast<ostringstream*>(&(ostringstream()
			<< endTimeInNano))->str();
	float systemFreq = 1.0 / params.getCoresPeriodInNano();
	std::ostringstream freqVal;
	freqVal << systemFreq;
	string freqString = freqVal.str();
	string parameter_row = static_cast<ostringstream*>(&(ostringstream()
			<< (params.getRows())))->str();
	string parameter_col = static_cast<ostringstream*>(&(ostringstream()
			<< (params.getCols())))->str();
	string s_temp = "Execution time of the application(ns): " + timeString
			+ "\nClock frequency(GHz) : " + freqString + "\nROWS : "
			+ parameter_row + "\nCOLUMNS : " + parameter_col;
	fputs(s_temp.c_str(), Parameters);

	// Print some results on stdout
	cout << " ##Simulation results##" << endl << endl;
	cout << "    Number of runnable instances executed         : "
			<< nbRunnablesCompleted << endl;
	if (params.dontHandlePeriodic()) {
		cout << "    Number of application iterations executed     : "
				<< (nbRunnablesCompleted / runnables.size()) << endl;
	}
	cout << "    Execution time of the application             : "
			<< endTimeInNano << " ns" << endl;
	int nbDeadlineMisses = 0;
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			nbDeadlineMisses = nbDeadlineMisses
					+ pes[row][col]->deadlinesMissed;
		}
	}
	cout << "    Number of runnables which missed deadlines    : "
			<< nbDeadlineMisses << endl;
	cout << "    Number of packets exchanged through NoC       : "
			<< noc_->nbPkts << endl;
	cout << "    Simulation time                               : "
			<< (double) (end - start) / CLOCKS_PER_SEC << " s" << endl;

	double meanNbPacketsPerLink = 0;
	unsigned int nbRoutes = 0;
	typedef map<int, unsigned int>::const_iterator it_type;
	for (it_type iterator = noc_->routeToNbPkts.begin();
			iterator != noc_->routeToNbPkts.end(); iterator++) {
		meanNbPacketsPerLink += iterator->second;
		nbRoutes++;
	}
	meanNbPacketsPerLink = meanNbPacketsPerLink / nbRoutes;
	cout << "    Avg number of packets exchanged per NoC link  : "
			<< meanNbPacketsPerLink << endl;
	double variance = 0;
	for (it_type iterator = noc_->routeToNbPkts.begin();
			iterator != noc_->routeToNbPkts.end(); iterator++) {
		double distToMean = iterator->second - meanNbPacketsPerLink;
		variance += (distToMean * distToMean);
	}
	variance = variance / nbRoutes;
	double rsdLinksLoad = (sqrt(variance) / meanNbPacketsPerLink) * 100;
	cout << "    RSD number of packets exchanged per NoC link  : "
			<< rsdLinksLoad << " %" << endl << endl;

	// Dump NoC traces
	if (params.getBuiltinAnalyses()) {
		dumpNoCLoadGraphFile(endTimeInNano);
		dumpCoresLoadGraphFile(endTimeInNano);
	}
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			pes[row][col]->dumpNoCTraces();
		}
	}

	sc_stop();
}

void dcSystem::updateInstructionsExecTimeBounds(string newAppFile) {

	// Parse new application
	dcAmaltheaParser amaltheaParser;
	AmApplication* amApplication = new AmApplication();
	amaltheaParser.ParseAmaltheaFile(newAppFile, amApplication);
	dcApplication *app = new dcApplication();
	dcTaskGraph * taskGraph = app->createGraph("dcTaskGraph");
	app->CreateGraphEntities(taskGraph, amApplication, params.getSeqDep());

	// Compare and update instructions
	vector<dcInstruction *> instructions = application->GetAllInstructions(
			taskGraph);
	vector<dcInstruction *> newInstructions = app->GetAllInstructions(
			taskGraph);
	for (vector<dcInstruction *>::size_type i = 0; i < instructions.size();
			i++) {
		dcInstruction *newInst = newInstructions.at(i);
		dcInstruction *oldInst = instructions.at(i);
		if (newInst->GetName() != oldInst->GetName()) {
			cerr << "error while updating instructions execution time bounds"
					<< endl;
			exit(-1);
		}
		string instrName = newInst->GetName();
		if (instrName == "sw:InstructionsDeviation") {
			dcExecutionCyclesDeviationInstruction *newInstDev =
					static_cast<dcExecutionCyclesDeviationInstruction*>(newInst);
			dcExecutionCyclesDeviationInstruction *oldInstDev =
					static_cast<dcExecutionCyclesDeviationInstruction*>(oldInst);
			oldInstDev->SetLowerBound(newInstDev->GetLowerBound());
			oldInstDev->SetUpperBound(newInstDev->GetUpperBound());
			oldInstDev->SetKappa(newInstDev->GetKappa());
			oldInstDev->SetLambda(newInstDev->GetLambda());
			oldInstDev->SetMean(newInstDev->GetMean());
			oldInstDev->SetRemainPromille(newInstDev->GetRemainPromille());
			oldInstDev->SetSD(newInstDev->GetSD());
		}
		if (instrName == "sw:InstructionsConstant") {
			dcExecutionCyclesConstantInstruction *newInstCst =
					static_cast<dcExecutionCyclesConstantInstruction*>(newInst);
			dcExecutionCyclesConstantInstruction *oldInstCst =
					static_cast<dcExecutionCyclesConstantInstruction*>(oldInst);
			oldInstCst->SetValue(newInstCst->GetValue());
		}
	}

	// Delete objects
	delete taskGraph;
	delete app;
	delete amApplication;
}

}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
