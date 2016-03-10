#include "dcSimuParams.hxx"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>

std::string getCmdOption(char ** begin, char ** end,
		const std::string & option) {
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end) {
		return *itr;
	}
	return "";
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
	return std::find(begin, end, option) != end;
}

void dcSimuParams::printHelp() {
	std::cerr << "usage is:" << std::endl << binary
			<< " [-d] [-fd] [-h] [-np] [-r] [-ba] [-genRandomSeed seed] [-wekaFile file]\n"
					"(-a app_file | -f mode_file)\n"
					"-freq freq_in_hertz\n"
			        "-simuEnd end_in_nano\n"
					"-i iterations\n"
					"-m mapping_heuristic\n"
					"-o output_folder\n"
					"-s scheduling_strategy\n"
					"-x rows -y cols" << std::endl;
}

bool dcSimuParams::getHelp() {
	return help;
}

bool dcSimuParams::getSeqDep() {
	return seqDep;
}

bool dcSimuParams::getBuiltinAnalyses() {
	return ba;
}

int dcSimuParams::getSeed() {
	return seed;
}

bool dcSimuParams::getGenerateWaveforms() {
	return true;
}

bool dcSimuParams::dontHandlePeriodic() {
	return dontHandlePer;
}

bool dcSimuParams::getFullDuplex() {
	return fullDuplex;
}

bool dcSimuParams::getRandomNonDet() {
	return randomNonDet;
}

std::string dcSimuParams::getOutputFolder() {
	return outputFolder;
}

std::string dcSimuParams::getMappingHeuristic() {
	return mappingHeuristic;
}

std::string dcSimuParams::getMappingFile() {
	return mappingFile;
}

unsigned int dcSimuParams::getMappingSeed() {
	return mappingSeed;
}

std::string dcSimuParams::getSchedulingStrategy() {
	return schedulingStrategy;
}

std::string dcSimuParams::getAppXml() {
	return appXml;
}

std::string dcSimuParams::getModeFile() {
	return modeFile;
}

std::string dcSimuParams::getWekaFile() {
	return wekaFile;
}

unsigned int dcSimuParams::getRows() {
	return rows;
}

unsigned int dcSimuParams::getCols() {
	return cols;
}

unsigned int dcSimuParams::getIterations() {
	return its;
}

unsigned long int dcSimuParams::getCoresFrequencyInHz() const {
	return coresFrequencyInHz;
}

unsigned long int dcSimuParams::getSimuEnd() const {
	return simuEnd;
}

double dcSimuParams::getCoresPeriodInNano() const {
	double period = 1E9 / (double) coresFrequencyInHz;
	return period;
}

dcSimuParams::dcSimuParams(int argc, char **argv) {
	binary = argv[0];

	if (cmdOptionExists(argv, argv + argc, "-d")) {
		seqDep = true;
	} else {
		seqDep = false;
	}

	if (cmdOptionExists(argv, argv + argc, "-ba")) {
		ba = true;
	} else {
		ba = false;
	}

	if (cmdOptionExists(argv, argv + argc, "-fd")) {
		fullDuplex = true;
	} else {
		fullDuplex = false;
	}

	if (cmdOptionExists(argv, argv + argc, "-h")) {
		help = true;
	} else {
		help = false;
	}

	if (cmdOptionExists(argv, argv + argc, "-np")) {
		dontHandlePer = true;
	} else {
		dontHandlePer = false;
	}

	if (cmdOptionExists(argv, argv + argc, "-r")) {
		randomNonDet = true;
	} else {
		randomNonDet = false;
	}

	outputFolder = getCmdOption(argv, argv + argc, "-o");
	if (outputFolder.empty()) {
		printHelp();
		exit(-1);
	}
	struct stat info;
	if (stat(outputFolder.c_str(), &info) != 0) {
		std::cerr << "cannot access output folder " << outputFolder
				<< std::endl;
		exit(-1);
	} else if (!(info.st_mode & S_IFDIR)) {
		std::cerr << "output folder " << outputFolder
				<< " is not a folder but a file" << std::endl;
		exit(-1);
	}

	mappingHeuristic = getCmdOption(argv, argv + argc, "-m");
	if (mappingHeuristic.empty()) {
		printHelp();
		exit(-1);
	}
	if (mappingHeuristic != "KhalidDC" && mappingHeuristic != "MinComm"
			&& mappingHeuristic != "Static" && mappingHeuristic != "StaticSM"
			&& mappingHeuristic != "ZigZag" && mappingHeuristic != "ZigZagSM"
			&& mappingHeuristic != "3Core" && mappingHeuristic != "Random"
			&& mappingHeuristic != "StaticModes"
			&& mappingHeuristic != "Weka") {
		std::cerr << "invalid mapping heuristic: " << mappingHeuristic
				<< std::endl
				<< "  valid ones are KhalidDC, MinComm, Static, StaticSM, ZigZag, ZigZagSM, 3Core, Random, StaticModes, Weka"
				<< std::endl;
		exit(-1);
	}
	std::string prefix("Static");
	std::string wekaPrefix("Weka");
	if (!mappingHeuristic.compare(0, prefix.size(), prefix)
			|| !mappingHeuristic.compare(0, wekaPrefix.size(), wekaPrefix)) {
		mappingFile = getCmdOption(argv, argv + argc, mappingHeuristic);
		struct stat fileStatus;
		errno = 0;
		if (stat(mappingFile.c_str(), &fileStatus) == -1) {
			if (errno == ENOENT)
				throw(std::runtime_error(
						mappingFile
								+ " does not exist, or path is an empty string."));
			else if ( errno == ENOTDIR)
				throw(std::runtime_error(
						"Mapping file: A component of the path is not a directory."));
			else if ( errno == ELOOP)
				throw(std::runtime_error(
						"Mapping: file Too many symbolic links encountered while traversing the path."));
			else if ( errno == EACCES)
				throw(std::runtime_error("Mapping file: Permission denied."));
			else if ( errno == ENAMETOOLONG)
				throw(std::runtime_error("Mapping file:  Can not be read."));
		}
	}
	std::string randomFixed("Random");
	if (!mappingHeuristic.compare(randomFixed)) {
		mappingSeed = std::stoi(
				getCmdOption(argv, argv + argc, mappingHeuristic));
	}

	schedulingStrategy = getCmdOption(argv, argv + argc, "-s");
	if (schedulingStrategy.empty()) {
		printHelp();
		exit(-1);
	}
	if (schedulingStrategy != "fcfs" && schedulingStrategy != "prio") {
		std::cerr << "invalid scheduling strategy:" << schedulingStrategy
				<< std::endl << "  valid ones are fcfs and prio" << std::endl;
		exit(-1);
	}

	// Get application or mode file
	appXml = getCmdOption(argv, argv + argc, "-a");
	modeFile = getCmdOption(argv, argv + argc, "-f");
	if (appXml.empty() && modeFile.empty()) {
		printHelp();
		exit(-1);
	}
	if (!appXml.empty() && !modeFile.empty()) {
		printHelp();
		exit(-1);
	}
	if (!appXml.empty() && !std::ifstream(appXml)) {
		std::cerr << "application file " << appXml << " doesn't exist"
				<< std::endl;
		exit(-1);
	} else if (!modeFile.empty()) {
		if (!std::ifstream(modeFile)) {
			std::cerr << "mode file " << modeFile << " doesn't exist"
					<< std::endl;
			exit(-1);
		}
	}

	wekaFile = getCmdOption(argv, argv + argc, "-wekaFile");
	if (!wekaFile.empty() && !std::ifstream(wekaFile)) {
		std::cerr << "weka file " << wekaFile << " doesn't exist" << std::endl;
		exit(-1);
	}

	std::string xString = getCmdOption(argv, argv + argc, "-x");
	if (xString.empty()) {
		printHelp();
		exit(-1);
	}
	rows = std::stoi(xString);

	std::string freqString = getCmdOption(argv, argv + argc, "-freq");
	if (freqString.empty()) {
		printHelp();
		exit(-1);
	}
	coresFrequencyInHz = std::stol(freqString);

	std::string simuEndString = getCmdOption(argv, argv + argc, "-simuEnd");
	if (!simuEndString.empty()) {
		simuEnd = std::stol(simuEndString);
	} else {
		simuEnd = 0;
	}


	std::string seedString = getCmdOption(argv, argv + argc, "-genRandomSeed");
	if (!seedString.empty()) {
		seed = std::stoi(seedString);
	} else {
		seed = -1;
	}

	std::string yString = getCmdOption(argv, argv + argc, "-y");
	if (yString.empty()) {
		printHelp();
		exit(-1);
	}
	cols = std::stoi(yString);

	std::string itString = getCmdOption(argv, argv + argc, "-i");
	if (itString.empty()) {
		printHelp();
		exit(-1);
	}
	its = std::stoi(itString);
}
