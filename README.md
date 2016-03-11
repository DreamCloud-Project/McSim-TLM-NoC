# abstract-simulator

This repository contains the simulator used to simulate embedded applications described using
the Amalthea application model on top of NoC-based architectures. To get the simulator you must clone this repository and its submodules using the following commands:

```
# if you have an GitHub account with an SSH key registered
git clone git@github.com:DreamCloud-Project/abstract-simulator.git 
# else
git clone https://github.com/DreamCloud-Project/abstract-simulator.git
git submodule init 
git submodule update
```

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

The requirements for using these scripts are the following ones:  

- have CMake installed on your system [(https://cmake.org)](https://cmake.org/)
- define the SYSTEMC_HOME variable pointing to a SystemC 2.3.1 root folder
- have the xerces-c-dev library installed in standard includes and libs folders (using apt-get for example)
  or have xerces-c-dev library in a custom folder and define XERCES_HOME

### Compiling the simulator

Compilation is done through the compile.py script which documentation is the following:  

```
>> ./compile.py --help
usage: compile.py [-h] [-v] {build,clean} ...

Abstract simulator compiler script

optional arguments:
  -h, --help     show this help message and exit
  -v, --verbose  enable verbose output

valid subcommands:
  {build,clean}  
```

### Running the simulator

To run a particular simulation, just run the simulate.py script. By
default this script runs one iteration of the Demo Car application on
a 4x4 NoC using ZigZag mapping, First Come First Serve (fcfs)
scheduling and without repeating periodic runnables.  You can play
with all these parameters which documentation is the following:

```
>> ./simulate.py --help
usage: simulate.py [-h] [-d] [-da {DC}] [-ca CUSTOM_APPLICATION] [-f FREQ]
                   [-mf MODES_FILE] [-i ITERATIONS]
                   [-m MAPPING_STRATEGY [MAPPING_STRATEGY ...]] [-np] [-nfd]
                   [-o OUTPUT_FOLDER] [-r] [-seed SEED_RANDOM]
                   [-s {fcfs,prio}] [-v] [-x ROWS] [-y COLS] [-ba]

Abstract simulator runner script

optional arguments:
  -h, --help            show this help message and exit
  -d, --syntax_dependency
                        consider successive runnables in tasks call graph as
                        dependent
  -da {DC}, --def_application {DC}
                        specify the application to be simulated among the
                        default ones
  -ca CUSTOM_APPLICATION, --custom_application CUSTOM_APPLICATION
                        specify a custom application file to be simulated
  -f FREQ, --freq FREQ  specify the frequency of cores in the NoC (i.g 400MHz
                        or 1GHz)
  -mf MODES_FILE, --modes_file MODES_FILE
                        specify a modes switching file to be simulated
  -i ITERATIONS, --iterations ITERATIONS
                        specify the number of application to execute (has no
                        effect with -p)
  -m MAPPING_STRATEGY [MAPPING_STRATEGY ...], --mapping_strategy MAPPING_STRATEGY [MAPPING_STRATEGY ...]
                        specify the mapping strategy used to map runnables on
                        cores. Valide strategies are ['MinComm', 'Static',
                        'ZigZag', 'Random', 'StaticModes']
  -np, --no_periodicity
                        run periodic runnables only once
  -nfd, --no_full_duplex
                        don't use a full duplex NoC
  -o OUTPUT_FOLDER, --output_folder OUTPUT_FOLDER
                        specify the absolute path of the output folder where
                        simulation results will be generated
  -r, --random          replace constant seed used to generate distributions
                        by a random one based on the time
  -seed SEED_RANDOM, --seed_random SEED_RANDOM
                        the seed
  -s {fcfs,prio}, --scheduling_strategy {fcfs,prio}
                        specify the scheduling strategy used by cores to
                        choose the runnable to execute
  -v, --verbose         enable verbose output
  -x ROWS, --rows ROWS  specify the number of rows in the NoC
  -y COLS, --cols COLS  specify the number of columns in the NoC
  -ba, --built_in_analyses
                        generate graphs for built in analyzes about NoC links
                        and cores utilization
```

## Licence

This software is made available under the Apache Software License. Version 2.0  

Report bugs at: dreamcloud-support@lirmm.fr  

(C)2015 CNRS LIRMM / Université de Montpellier and University of York  
ADAC Group LIRMM and Real-Time System Reasearch Group
