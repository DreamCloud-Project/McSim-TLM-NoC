/*
 * processingElementType.cpp
 *
 *  Created on: Nov 27, 2015
 *      Author: manu
 */

#include "processingElementType.hxx"

namespace dreamcloud {
namespace platform_sclib {

processingElementType::processingElementType(unsigned long int frequencyInHz_,
		unsigned long int nbCyclesPerInstructions_) :
		frequencyInHz(frequencyInHz_), nbCyclesPerInstructions(
				nbCyclesPerInstructions_) {
}

processingElementType::~processingElementType() {
	// Nothing to do
}

unsigned long processingElementType::getFrequencyInHz() const {
	return frequencyInHz;
}

unsigned long processingElementType::getNbCyclesPerInstructions() const {
	return nbCyclesPerInstructions;
}

void processingElementType::setFrequencyInHz(unsigned long int frequencyInHz) {
	this->frequencyInHz = frequencyInHz;
}

}
} /* namespace dreamcloud */
