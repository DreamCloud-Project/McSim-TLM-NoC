# McSim-TLM-NoC: Manycore platform Simulation tool for NoC-based platform at a Transactional Level Modeling level

This repository contains a simulator able to simulate embedded applications described using
the [AMALTHEA](http://www.amalthea-project.org/) application model on top of NoC-based manycore architectures at TLM level. This simulator has been jointly defined by the LIRMM laboratory of Montpellier and the University of York. 

To get the simulator you must clone this repository and its submodules. To clone the repository if you have an GitHub account with an SSH key registered use `git clone git@github.com:DreamCloud-Project/McSim-TLM-NoC.git`. Else use `git clone https://github.com/DreamCloud-Project/McSim-TLM-NoC.git`. Then do `cd McSim-TLM-NoC` followed by `git submodule init` followed by `git submodule update` to clone submodules.

## Using the simulator

To ease the usage of the simulator, two python scripts are provided:  

- compile.py for compilation  
- simulate.py for launching the simulation  

The requirements for using these scripts are the following ones:  

- have a Linux system
- have a C++ compiler
- have CMake installed on your system ([https://cmake.org](https://cmake.org/))
- have SystemC 2.3.1
    * download SystemC (Core SystemC Language and Examples) from here http://accellera.org/downloads/standards/systemc/systemc-license-agreement
    * decompress the archive
    * run `configure` and then `make install` 
    * you should see a new `include` folder and a new `lib-linux64` folder in your root folder of SystemC 
    * define the `SYSTEMC_HOME` environment variable to the root folder of SystemC 
- have the xerces-c-dev library installed in standard includes and libs folders (using `apt-get install libxerces-c-dev` for example on Ubuntu)
  or have xerces-c-dev library in a custom folder and define `XERCES_HOME` environment variable

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
                   [-m MAPPING_STRATEGY [MAPPING_STRATEGY ...]] [-mw]
                   [-mww MICRO_WORKLOAD_WIDTH] [-mwh MICRO_WORKLOAD_HEIGHT]
                   [-np] [-nfd] [-o OUTPUT_FOLDER] [-r] [-seed SEED_RANDOM]
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
  -mw, --micro_workload
                        simulate a micro workload of the application instead
                        of the real application
  -mww MICRO_WORKLOAD_WIDTH, --micro_workload_width MICRO_WORKLOAD_WIDTH
                        the width of the simulated micro workload. To be used
                        with -mw only
  -mwh MICRO_WORKLOAD_HEIGHT, --micro_workload_height MICRO_WORKLOAD_HEIGHT
                        the height of the simulated micro workload. To be used
                        with -mw only
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

This software is made available under the  GNU Lesser General Public License v3.0

Report bugs at: mcsim-support@lirmm.fr  

(C)2016 CNRS and Université de Montpellier and University of York  
