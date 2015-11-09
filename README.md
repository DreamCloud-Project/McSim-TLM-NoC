# abstract-simulator

This repository contains the simulator used to simulate embedded applications described using
the Amalthea application model on top of NoC based architecture. The simulator has been validated
using the "toy" applications provided in the apps folder.
Once you have unzipped the archive provided in this repository, follow the steps below to run a simulation.

## Directory content

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

## Using the simulator

To ease the usage of the simulator, two python scripts are provided:

- compile.py for compilation
- simulate.py for launching the simulation

The requirements for using both script are the following ones:

- define the SYSTEMC_HOME variable pointing to a SystemC 2.3.1 root folder
- have the xerces-c-dev library installed in standard includes and libs folders (using apt-get for example)
  or have xerces-c-dev library in a custom folder and define XERCES_HOME
- have the libboost-dev library installed in standard includes and libs folders (using apt-get for example)
  or have libboost-dev library in a custom folder and define BOOST_HOME

### Compiling the simulator

Compilation is done through the compile.py script which documentation is the following:

>> compile.py build -h
usage: compile.py build [-h] [-c] [-v]

optional arguments:
  -h, --help     show this help message and exit
  -c, --clean    clean previously compiled files before compiling
  -v, --verbose  enable verbose output

### Running the simulator

To run a particular simulation, just run the simulate.py script. By
default this script runs one iteration of the Demo Car application on
a 4x4 NoC using ZigZag mapping, First Come First Serve (fcfs)
scheduling and without repeating periodic runnables.  You can play
with all these parameters which documentation is the following:

>> ./simulate.py -h
usage: simulate.py [-h] [-d]
                   [-da {DC,CSE} | -ca CUSTOM_APPLICATION | -mf MODES_FILE]
                   [-i ITERATIONS] [-m {ZigZag,MinComm,KhalidDC}]
                   [-o OUTPUT_FOLDER] [-s {fcfs,prio}] [-x ROWS] [-y COLS]

Abstract Simulator Runner script

optional arguments:
  -h, --help            show this help message and exit
  -d, --syntax_dependency
                        consider successive runnables in tasks call graph as
                        dependent
  -da {DC,CSE}, --def_application {DC,CSE}
                        specify the application to be simulated among the
                        default ones
  -ca CUSTOM_APPLICATION, --custom_application CUSTOM_APPLICATION
                        specify a custom application file to be simulated
  -fd, --full_duplex    use a full duplex NoC
  -mf MODES_FILE, --modes_file MODES_FILE
                        specify a modes switching file to be simulated
  -i ITERATIONS, --iterations ITERATIONS
                        specify the number of application to execute (has no
                        effect with -p)
  -m {ZigZag,MinComm,KhalidDC,3Core}, --mapping_strategy {ZigZag,MinComm,KhalidDC,3Core}
                        specify the mapping strategy used to map runnables on
                        cores
  -np, --no_periodicity
                        run periodic runnables only once
  -o OUTPUT_FOLDER, --output_folder OUTPUT_FOLDER
                        specify the absolute path of the output folder where
                        simulation results will be generated
  -s {fcfs,prio}, --scheduling_strategy {fcfs,prio}
                        specify the scheduling strategy used by cores to
                        choose the runnable to execute
  -x ROWS, --rows ROWS  specify the number of rows in the NoC
  -y COLS, --cols COLS  specify the number of columns in the NoC

## Licence

This software is made available under the Apache Software License. Version 2.0

Report bugs at: dreamcloud-support@lirmm.fr

(C)2015 CNRS LIRMM and University of York
Adac Group LIRMM and Real-Time System Reasearch Group
