/*
 * dcSimuParams.hxx
 *
 *  Created on: Sep 10, 2015
 *      Author: manu
 */

#ifndef MAIN_NOC_PPA_CMAIN_DCSIMUPARAMS_HXX_
#define MAIN_NOC_PPA_CMAIN_DCSIMUPARAMS_HXX_

#include <string>

class dcSimuParams {
public:
	std::string getOutputFolder();
	std::string getMappingHeuristic();
	std::string getMappingFile();
	unsigned int getMappingSeed();
	std::string getSchedulingStrategy();
	std::string getAppXml();
	std::string getModeFile();
	std::string getWekaFile();
	dcSimuParams(int argc, char** argv);
	void printHelp();
	bool getHelp();
	bool getSeqDep();
	bool getBuiltinAnalyses();
	int getSeed();
	bool getFullDuplex();
	bool getGenerateWaveforms();
	bool dontHandlePeriodic();
	bool getRandomNonDet();
	unsigned int getRows();
	unsigned int getCols();
	unsigned int getIterations();
	unsigned long int getCoresFrequencyInHz() const;
	unsigned long int getSimuEnd() const;
	double getCoresPeriodInNano() const;
	bool getUseMicroworkload() const;
	unsigned int getMicroworkloadWidth() const;
	unsigned int getMicroworkloadHeight() const;

private:
	std::string outputFolder;
	std::string mappingHeuristic;
	std::string mappingFile;
	std::string schedulingStrategy;
	std::string appXml;
	std::string modeFile;
	std::string wekaFile;
	bool dontHandlePer;
	bool help;
	bool fullDuplex;
	bool randomNonDet;
	bool seqDep;
	bool ba;
	unsigned int rows;
	unsigned int cols;
	unsigned int its;
	unsigned int mappingSeed;
	unsigned long int simuEnd;
	char* binary;
	unsigned long int coresFrequencyInHz;
	int seed;
	bool useMicroworkload;
	unsigned int microworkloadWidth;
	unsigned int microworkloadHeight;
};



#endif /* MAIN_NOC_PPA_CMAIN_DCSIMUPARAMS_HXX_ */
