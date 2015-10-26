# abstract-simulator

This repository contains the simulator used to simulate embedded applications described using 
the Amalthea application model on top of NoC based architecture. 
Once you have unzipped the archive provided in this repository, follow the steps below to run a simulation.

## DIRECTORY CONTENT

- compile.py
	Compiles the simulator and the energy estimator from the src folder (see using the simulator section below)

- OUTPUT_FILES
	Default directory where the simulate.py script generates the results of simulation. It contains:
		Instruction_fixed_Power.txt - Internal file used by energy estimator
		Label_accesses_Power.txt - Internal file used by energy estimator
		Mapping.csv - Runnables to cores mapping location
		OUTPUT_CORE_WAVE.vcd - Waveform showing runnables activation along time and cores
		OUTPUT_Energy.log - Main file showing energy estimation results
		OUTPUT_Execution_Report.log - Reporting of the simulation as shown on stdout when running simulate.py
		OUTPUT_NoC_Traces.csv - Information about packets transferred through the NoC
		OUTPUT_RUNNABLE_IDs.csv - Internal file used by energy estimator
		OUTPUT_Runnable_Traces.csv - Information about runnables activation in a CSV format
		Parameters.txt - Internal file used by energy estimator
		Runnables.txt - Internal file used by energy estimator
		dcRunGraphFile.gv - Graphviz file of the application at runnables level
		dcTasksGraphFile.gv - Graphviz file of the application at task level

- README
	This file

- simulate.py
	Runs the simulator using the provided arguments (see using the simulator section below)

- src
	Contains the source code for the platform and for the energy estimator

## USING THE SIMULATOR

To ease the usage of the simulator, two python scripts are provided:

- compile.py for compilation
- simulate.py for launching the simulation

The requirements for using both script are the following ones:

- define the SYSTEMC_HOME variable pointing to a SystemC 2.3.1 root folder
- have the xerces-c-dev library installed in standard includes and libs folders (using apt-get for example)
  or have xerces-c-dev library in a custom folder and define XERCES_HOME
