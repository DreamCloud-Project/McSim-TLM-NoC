/*
 * processingElementType.h
 *
 *  Created on: Nov 27, 2015
 *      Author: manu
 */

#ifndef SRC_PLATFORM_SRC_SRC_CXX_MODULES_DREAMCLOUD_PLATFORM_SCLIB_PROCESSINGELEMENTTYPE_HXX_
#define SRC_PLATFORM_SRC_SRC_CXX_MODULES_DREAMCLOUD_PLATFORM_SCLIB_PROCESSINGELEMENTTYPE_HXX_

namespace dreamcloud {
namespace platform_sclib {

class processingElementType {

public:
	processingElementType(unsigned long int frequencyInHz,
			unsigned long int nbCyclesPerInstructions);
	virtual ~processingElementType();
	unsigned long getFrequencyInHz() const;
	unsigned long getNbCyclesPerInstructions() const;
	void setFrequencyInHz(unsigned long frequencyInHz);

private:

	// PE "speed" parameters
	// From this we compute runnables items execution time by
	// execT = ((item->getNbInsts() * nbCyclesPerInstructions) / frequencyInHz) seconds
	// nbCyclesPerInstructions is called instructionsPerCycle :( in the
	// Amalthea model. We use the invert here to be aligned with UoY IA.
	unsigned long int frequencyInHz;
	unsigned long int nbCyclesPerInstructions;
};

}
} /* namespace dreamcloud */

#endif /* SRC_PLATFORM_SRC_SRC_CXX_MODULES_DREAMCLOUD_PLATFORM_SCLIB_PROCESSINGELEMENTTYPE_HXX_ */
