#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <string.h>
#include <string>
#include <iostream>

//#define DEBUG 1

//inst_complex_min and inst_complex_max must lie between 0 and 9 (inclusively)
int Select_complexity_inst(int inst_complex_min, int inst_complex_max)
{
  //return a random number between inst_complex_min and inst_complex_max
  float rand_out = 0;
  int randm = RAND_MAX;
  int limit = inst_complex_max+10;
  int rnd = RAND_MAX/limit;
  int r = rand();
  while( (inst_complex_max < (r/rnd)) || ((r/rnd) < inst_complex_min))
    {
      r = rand();
    }
  return (r/rnd);
}




//Define and declare a runnable structure
struct runnable
{
  int index;
  int Nb_instructions;
  int Nb_label_accesses;
  int Nb_packets; //Theoretical number of total packets exchanged by the Runnable if the all Label acccesses were to be performed through the NoC
};

//Define and declare a hardware library 
struct cpu
{
  std::string name;
  int technology; //nm
  int temperature; //in K
  float stat_pow;
  float empty_pow;
  float table_complex_pow[7];
};

void initialise(struct cpu *CPU, float stat_pow, float config_freq, float clk_freq)
{
  //(*CPU).table_complex_pow[0] should be the dynamic power of "do nothing"
  //float empty_pow,
  (*CPU).technology = 28;
  (*CPU).temperature = 310;
  //(*CPU).empty_pow = (clk_freq*1E+3/config_freq)*empty_pow;//config_freq is in GHz and config_freq is in MHz. Uncomment this line if empty power is available.
  (*CPU).stat_pow = stat_pow;
  (*CPU).table_complex_pow[0] = (clk_freq*1E+3/config_freq)*stat_pow*0.1; //(W)
  (*CPU).table_complex_pow[1] = (clk_freq*1E+3/config_freq)*stat_pow*0.3; //(W)
  (*CPU).table_complex_pow[2] = (clk_freq*1E+3/config_freq)*stat_pow*0.5; //(W)
  (*CPU).table_complex_pow[3] = (clk_freq*1E+3/config_freq)*stat_pow*0.7; //(W)
  (*CPU).table_complex_pow[4] = (clk_freq*1E+3/config_freq)*stat_pow*0.9; //(W)
  (*CPU).table_complex_pow[5] = (clk_freq*1E+3/config_freq)*stat_pow*1.1; //(W)
  (*CPU).table_complex_pow[6] = (clk_freq*1E+3/config_freq)*stat_pow*1.3; //(W)
}

// Inst_length is the number of cycles; clock is the clock frequency in GHz
float get_Energy_Instruction(struct cpu **library, int lib, int Inst_length,int Inst_complex, float clock)
{
  float Power_dynamic=0;
  Power_dynamic = ((library[lib]->table_complex_pow)[Inst_complex-1]);
  return (((Inst_length)/(clock*1E+9))*Power_dynamic);
}

//Initialise a vector of runnables
struct runnable *initialise_Runnables(int Nb_runnables)
{

  struct runnable *Run;
  Run = (struct runnable *) malloc(sizeof(struct runnable) * Nb_runnables); //Run contains a description of all runnables

  int i = 0;
  for(i=0;i<Nb_runnables;i=i+1)
    {
      Run[i].index=i;
      Run[i].Nb_instructions=0;
      Run[i].Nb_label_accesses=0;
      Run[i].Nb_packets=0;
    }
  return Run;
}


//Check the presence of the string Runnable_name in the array of strings Runnables_Neme_Idx_buffer
int Check_presence(char** Runnables_Name_Idx_buffer, char* Runnable_name, int i) 
{

  int counter = 0;
  int present = 1;
  while((counter<(i+1))&&(present!=0))
    {
      present = strcmp(Runnables_Name_Idx_buffer[counter],Runnable_name);
      counter++;
    }
  return (present==0);
}

//Get the index of the string Runnable_name in the array Runnables_Name_Idx
int Get_Runnable_idx(char** Runnables_Name_Idx, char* Runnable_name)
{
  int counter = 0;
  int found = 0;
  while(!found)
    {
      found = (strcmp(Runnables_Name_Idx[counter],Runnable_name)==0);
      counter++;
    }

  return (counter-1);
}




int main(int argc, char** argv)
{

  // Get input folders
  if (argc != 3) {
	  std::cerr << "Energy estimator requires two parameters" << std::endl;
	  exit(-1);
  }
  std::string inputFolder = argv[1];
  std::string configFolder = argv[2];
  std::cout << inputFolder << " " << configFolder << std::endl;

  // variables for NoC
  int Size_Dist_vector = 0;
  int Columns = 0;
  int Rows = 0;
  int Number_in_out_Router = 5; //Number of input/output Links per Router (we assume the same number of inputs and outputs)
  int N_5 = 0;//Number of routers with 5 inputs/outputs
  int N_4 = 0;//Number of routers with 4 inputs/outputs
  int N_3 = 0;//Number of routers with 3 inputs/outputs
  int N_2 = 0;//Number of routers with 2 inputs/outputs

  int N_channels = 0;
  int Number_VC_per_input=0;
  int seed_mode;
  float N_clocks = 0;
  int r = 0;
  int N_flt_pack = 32; // Number of flits in packet
  int N_bytes_flit = 1; //Nb of bytes per flit. A flit equals a phit. A phit is 8 bits wide (width of the buss).
  float N_clk_flt = 1; // Average number of clock (NoC clock) cycles per flit
  float N_clk_flt_link = 0; // Average number of clk cycles for a flit to traverse a Link
  float N_clk_flt_router = 0; // Average number of clk cycles for a flit to traverse a Router
  float E_total_NoC = 0;
  float E_static_NoC = 0;
  float E_dynamic_NoC = 0;
  int i = 0;
  int j = 0;
  int S = 0;
  int N_flits = 0;
  int N_mesg = 0; // Overall umber of packets 
  int a=0;
  int b=0;
  int c=0;
  int d=0;
  int l = 0; // latency

  float Duration_packet = 0;
  float Duration_flit_hop = 0;
  float Mean_duration_flit = 0;
  float Mean_duration_packet = 0;
  float Mean_duration = 0; // Mean duration of a flit (ns) travelling one hop 
  float exec_time = 0; // Execution time of the application (ns)
   
  float Dyn_energy_RB_SB = 0;
  float Stat_energy_RB_SB = 0;
  float Tot_energy_RB_SB = 0;

  float Tot_energy_labels = 0;
  float Stat_energy_labels = 0;
  float Dyn_energy_labels = 0; // Energy of the label accesses

  float Dyn_energy_label_set = 0;
  float Stat_energy_label_set = 0;
  float Tot_energy_label_set = 0;

  float Tot_energy_WRB_REB = 0;
  float Dyn_energy_WRB_REB = 0; 
  float Stat_energy_WRB = 0;
  float Stat_energy_REB = 0;
  float Stat_energy_PRB = 0;
  float Tot_energy_preemptions = 0;//nJ

  // Parameters labels
  int size_Labels = 0; // Size in MB of local RAM 
  int Nb_banks_labels = 0;
  int Nb_bits_read_out = 0; //same value for labels and buffers
  //We assume in each case that the number of read ports equals the number of banks (the same holds for the number of write ports)
  float Dynamic_energy_read_port_label = 0; //nJ
  float Static_power_bank_label = 0; //Watts
  float Percentage_refresh_of_leakage = 0;
 
  // Parameters buffers
  int Nb_banks_buffers = 0;
  int size_WRB = 0; // Size in kB of WRB 
  int size_REB = 0; // Size in kB of REB 
  int size_PRB = 0; // Size in kB of PRB (Periodic Runnables Buffer) 
  float Dynamic_energy_read_port_WRB = 0; //nJ
  float Dynamic_energy_read_port_REB = 0; //nJ
  float Dynamic_energy_read_port_PRB = 0; //nJ

  float Static_power_bank_WRB = 0; //Watts
  float Static_power_bank_REB = 0; //Watts
  float Static_power_bank_PRB = 0; //Watts

  int size_SB = 0; //Size in Bytes of SB 
  int size_RB = 0; //Size in Bytes of RB 
  float Dynamic_energy_read_port_RB_SB = 0; //nJ
  float Static_power_bank_RB_SB = 0; //Watts

  // Variables for Instructions 
  FILE *fp;
  int size_line = 500;
  char line[500]; 
  char c1, c2, c3, c4, c5; // For extracting the name of a runnable
  char runn_idx[5];
  char str[13];
  int periodic;
  int Ins_length = 0; // Size of the instruction in nb of cycles
  float Tot_energy_exec = 0; // Energy of the execution instructions
  float Tot_energy = 0;
  float Tot_energy_static = 0;
  float Tot_energy_dynamic = 0;

  float Tot_energy_cores = 0;
  float Tot_energy_cores_static = 0;
  float Tot_energy_cores_dynamic = 0;

  float Tot_energy_cpu_idle = 0;

  float Power_static = 0; //Static power of a cpu
  float Power_empty = 0; //Dynamic power of CPU when execting an emprty loop
  //float Power_dynamic = 0; //Dynamic power of the cpu 
  float Power_dynamic_mean = 0;
  float Complexity_mean = 0;
  float Power_static_R = 0; // Static power per router (5 in/out + 2Virtual Channels per input)
  float Power_dynamic_VR_flit = 0; // Dynamic power for transmitting a flit through a virtual router
  float Power_static_Link = 0; // Static power per Link (Links of a router). A router with 5 inputs and 5 outputs has 5 Links! A link is thus considered bidirectionnal(2 wires)
  //We assume : flit=phit i.e. a flit has the size of the width of a one-directionnal Link wire
  float Power_dynamic_Link_flit = 0; // Dynamic power for transmitting a flit on a Link
  float config_freq = 0; //in MHz

  int Time_ins = 0; // Total execution time of the instructions (in the number of cycles) 
  float clk = 0; // clock frequency in GHz 
  int N_processors = 0;
  float test = 0;
  int label_data = 0;
  int rd_or_wr = 0; // To be used for the 3rd column of the Label_accesses_Power.txt. 0 is for a read request; 1 is for a write request.
  int packets = 0; // Total number of packets (generated by the label accesses) 
  int local_packets = 0;
  int local_flits = 0;
  int total_flits = 0;
  int remote_flits = 0;

  // Variables for runnables
  struct runnable *Run_Idx; // Run contains information about all runnables (name, nb_instructions, nb_labels_accesses, nb_packets)
  int Nb_runnables = 0;
  int Nb_mult_occ_runnables = 0; //Number of all accurrences of runnables (several runnables occur multiple times)
  int runn_extract = 0;
  int x = 0;
  int y = 0;
  char state;
  int Nb_bytes_per_instruction = 0; // For storing runnables in WRB and REB during premptions: The size of a runnable is estimated as : nb_instr*Nb_bytes_per_instruction+Nb_bytes_offset
  int Nb_bytes_offset = 0;

  //Configuration variables 
  int lib = 0;//library index
  int inst_complex_min = 0;//complexity of the computation instruction
  int inst_complex_max = 0;//complexity of the computation instruction
  int Inst_complex;
  char cpu_name[3];


  //*******************************************************************************************************************************************************************
  //Calibration of NoC power characteristics
  //For a router, the number of input ports is kept constnt and is equal to 5. The number of Virtual Channels per input is the unique variable of the router model. 
  //This variable is set in the Configuration.txt file
  //*******************************************************************************************************************************************************************

  //Arbiter
  float arbiter_static[2];
  arbiter_static[0] = 1.57*1E-8;
  arbiter_static[1] = 1.489*1E-7;
  float arbiter_dynamic = 2*1E-12;//Power per flit per input per VC

  //Crossbar
  float crossbar_static = 6.29*1E-6;
  float crossbar_dynamic = 1.88*1E-12;//Power per flit per input

  //Buffer
  float buffer_static[3];
  buffer_static[0] = 8.87*1E-5;
  buffer_static[1] = -1.0*1E-4;
  buffer_static[2] = 6.979*1E-5;
   
  float buffer_dynamic[3];//Power per flit per input per VC
  buffer_dynamic[0] = 6.63*1E-12;
  buffer_dynamic[1] = 1.72*1E-11;
  buffer_dynamic[2] = 5.16*1E-12;

  //Links
  float links_static[3];//per router
  links_static[0] = 1.43*1E-6;
  links_static[1] = 4.19*1E-7;
  links_static[2] = 1.14*1E-5;

  float links_dynamic[3];//per router per flit per input
  links_dynamic[0] = 6.63*1E-12;
  links_dynamic[1] = 1.72*1E-11;
  links_dynamic[2] = 5.16*1E-12;

  struct cpu ARM;
  ARM.name = "ARM_A15";

  struct cpu ALPHA;
  ALPHA.name = "ALPHA_21364";

  //library is an arry of pointers to (structs cpu)
  struct cpu *library[3];
  library[0]=&ARM;
  library[1]=&ALPHA;

  int line_counter = 0;
  char mapped[6];


  // Variables for Debug
  float t1 = 0;
  float t2 = 0;
  float prop1=0;
  float prop2 = 0;
  float var_test = 0;


  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                Check if the system has enough memory                                      *//
  //*                                                                                                           *//
  //*************************************************************************************************************//


  // Read Execution report for checking if all labels were mapped
  fp = fopen((inputFolder + "/OUTPUT_Execution_Report.log").c_str() , "r");
  if(fp == NULL) {
    perror((inputFolder + "/OUTPUT_Execution_Report.log not found").c_str());
    return(-1);
  }

  while(line_counter<26)
    {
      fgets (line, size_line, fp);
      line_counter++;
    }

  fgets (line, size_line, fp);
  sscanf( line, "%s", mapped);
  char str1[50] = "MAPPED";

  if(strcmp(str1, mapped)==0)
    {
      //printf("Memory too small\n");
      return 0;
    }
  else
    {
      //printf("Memory not too small\n");
    }

  fclose(fp);
  line_counter = 0;

  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                           Read Parameters 1                                               *//
  //*                                                                                                           *//
  //*************************************************************************************************************//


  // Read clock frequency and execution time of the application 
  fp = fopen((inputFolder + "/Parameters.txt").c_str() , "r");
  if(fp == NULL) {
    perror("Parameters.txt not found in the specified directory");
    return(-1);
  }
  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %f", &exec_time);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*c %f", &clk);

  N_clocks = exec_time*clk;

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*c %d", &Rows);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*c %d", &Columns);

  Size_Dist_vector = (Rows-1)+(Columns-1)+1;
  N_processors = Rows*Columns;
  N_channels = 5*(Rows-2)*(Columns-2)+4*3+4*(2*Columns+2*(Rows-2)-4); // Number of channels on the platform (a channel is a link from one router to on neighbour)
 
  //Determine the number of routers with different number of inputs/outputs
  if((Columns>=2) || (Rows >= 2))
    {
      N_5 = (Rows-2)*(Columns-2);
      N_4 = 2*(Columns+Rows-4);
      N_3 = 4;
      N_2 = 0;
    }
  else
    {
      if((Columns>=2) && (Rows<2))
	{
	  N_5 = 0;
	  N_4 = 0;
	  N_3 = Columns-2;
	  N_2 = 2;
	}
      if((Rows>=2) && (Columns<2))
	{
	  N_5 = 0;
	  N_4 = 0;
	  N_3 = Rows-2;
	  N_2 = 2;
	}
      if(Rows==1 && Columns==1)
	{
	  N_5 = 0;
	  N_4 = 0;
	  N_3 = 0;
	  N_2 = 0;
	}
    }



  int Dist_vector[Size_Dist_vector]; //vector of distances : Dis[i] is the number of packets traveling the distance i//
  // Initialisation of the vector of distances//
  for(i=0;i<Size_Dist_vector;i=i+1){
    Dist_vector[i] = 0;
  }
  fclose(fp);



  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                           Read Configuration                                              *//
  //*                                                                                                           *//
  //*************************************************************************************************************//
  fp = fopen((configFolder + "/Configuration_Parameters/Configuration.txt").c_str() , "r");
  if(fp == NULL) {
    perror("Configuration.txt not found in the specified directory");
    return(-1);
  }

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*c %f", &config_freq);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %f", &Power_static);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Power_empty);

  initialise(&ALPHA, Power_static, config_freq, clk); //Power_empty,

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %f", &Power_static);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Power_empty);

  initialise(&ARM, Power_static, config_freq, clk); //Power_empty,

  //fgets (line, size_line, fp);
  //sscanf( line, "%*s %*s %*s %*s %*c %d", &Number_in_out_Router);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %d", &Number_VC_per_input);
  //printf("Num of VC per input : %d\n",Number_VC_per_input);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*c %s", cpu_name);
 
  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*c %d %d", &inst_complex_min, &inst_complex_max);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*c %d", &seed_mode);


  if(!strcmp(cpu_name,"ARM"))
    {
      lib = 0;
      //printf("ARM_A15\n");
    }
  if(!strcmp(cpu_name,"ALP"))
    {
      lib = 1;
      //printf("ALPHA21364\n");
    }
   
  Power_static = library[lib]->stat_pow;
   
  //printf("CPU : %s\n",library[lib]->name);
  //printf("Inst : %f\n",Power_dynamic);

  // Seed the grain for the pseudo-random number generator
  if(seed_mode)
    {
      srand (time (NULL));
    }
  else
    {
      srand (1242);
    }


  // printf("Seed mode : %d \n",seed_mode);

  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                           Read Parameters 2                                               *//
  //*                                                                                                           *//
  //*************************************************************************************************************//
  // Read buffer sizes and Energy parameters 
  fp = fopen((configFolder + "/Configuration_Parameters/Parameters_2.txt").c_str() , "r");
  if(fp == NULL) {
    perror("Parameters_2.txt not found in the specified directory");
    return(-1);
  }

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %d", &size_Labels);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*c %d", &Nb_banks_labels);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %d", &Nb_bits_read_out);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*s %*c %f", &Dynamic_energy_read_port_label);//nJ (per read port)

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %f", &Static_power_bank_label); //Watts

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %f", &Percentage_refresh_of_leakage);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*c %d", &size_WRB);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*c %d", &size_REB);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*c %d", &size_PRB);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*c %d", &Nb_banks_buffers);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Dynamic_energy_read_port_WRB);//nJ

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Dynamic_energy_read_port_REB);//nJ

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Dynamic_energy_read_port_PRB);//nJ

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Static_power_bank_WRB);//Watts

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Static_power_bank_REB);//Watts

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Static_power_bank_PRB);//Watts

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*c %d", &size_SB);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*c %d", &size_RB);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Dynamic_energy_read_port_RB_SB);//nJ Energy per read port (1 byte width)

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*s %*c %f", &Static_power_bank_RB_SB);//Watts

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*c %d", &Nb_bytes_per_instruction);

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %*s %*s %*s %*c %d", &Nb_bytes_offset);
  fclose(fp);

  // Read Number of Runnables //
  fp = fopen((inputFolder + "/OUTPUT_Execution_Report.log").c_str(), "r");
  if(fp == NULL) {
    perror("OUTPUT_Execution_Report.log not found in the specified directory");
    return(-1);
  }
  line_counter = 0;
  while(line_counter<31)
    {
      fgets (line, size_line, fp);
      line_counter++;
    }
  fgets (line, size_line, fp);
  sscanf(line, "    %*s %*s %*s %*s %*s    %*s %d", &Nb_mult_occ_runnables);

  // Check to avoid allocating very large memory area
  // TODO to be handle properly
  if (Nb_mult_occ_runnables < 0 || Nb_mult_occ_runnables > 1E6) {
    std::cerr << "Energy estimator has more than 1 billion of runnables ("
	      << Nb_mult_occ_runnables << "), is it normal ?" << std::endl;
    exit(-1);
  }

#ifdef DEBUG
  fprintf(stderr, "Mult_runn : %d\n", Nb_mult_occ_runnables);
#endif

  fclose(fp);
   

  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                Determine the list of runnables                                            *//
  //*                                                                                                           *//
  //*************************************************************************************************************//
  //At the end of this ssection, the array Runnables_Name_Idx contains the names (only once) of all the Runnables of the application
  int runn_id_counter = 0;
  int max_size_runnable_name = 200; //Max size of a runnable name (in bytes)

  //Runnables_ID_Name
  //The file OUTPUT_FILES/OUTPUT_RUNNABLE_IDs.csv is the list of Runnables with their IDs. The Runnables are listed without taking into account the periodicity : a periodic Runnable is listed so many times as it would be if its periodicity was 1. Runnables_ID_Name just copies this information from OUTPUT_RUNNABLE_IDs.csv.
  char** Runnables_ID_Name;
  Runnables_ID_Name = (char**)calloc(Nb_mult_occ_runnables,sizeof(char *));
  for(i=0;i<Nb_mult_occ_runnables;i++)
    {
      Runnables_ID_Name[i] = (char*)calloc(max_size_runnable_name,sizeof(char));
    }

  //Fill Runnables_ID_Name
  char* Runnable_name;
  Runnable_name = (char*)calloc(max_size_runnable_name,sizeof(char));
  char* eraser;
  eraser = (char*)calloc(max_size_runnable_name,sizeof(char));;

#ifdef DEBUG
  fprintf(stderr, "Parsing OUTPUT_RUNNABLE_IDs trace\n");
#endif

  fp = fopen((inputFolder + "/OUTPUT_RUNNABLE_IDs.csv").c_str(), "r");
  if (fp == NULL) {
    perror("OUTPUT_RUNNABLE_IDs.csv not found in the specified directory");
    return (-1);
  }

  fgets(line, size_line, fp);
  while (fgets(line, size_line, fp) != NULL) {
    sscanf(line, "%d %*c %s", &runn_id_counter, Runnable_name);
    strcpy(Runnables_ID_Name[runn_id_counter], Runnable_name);
  }
  fclose(fp);

#ifdef DEBUG
  fprintf(stderr, "Parsing OUTPUT_RUNNABLE_IDs trace DONE\n");
#endif

  //Runnable_Name_Idx buffer
  //In the array Runnables_ID_Name however, a given Runnable from the application graph may still appear multiple times(not because of periodicity). 
  //We would like to get the list of Runnables as they appear in the application graph. For that, we copy Runnables_ID_Name in Runnables_Name_Idx_buffer.
  char** Runnables_Name_Idx_buffer; 
  Runnables_Name_Idx_buffer = (char**)calloc(Nb_mult_occ_runnables,sizeof(char *));
  for(i=0;i<Nb_mult_occ_runnables;i++)
    {
      Runnables_Name_Idx_buffer[i] = (char*)calloc(max_size_runnable_name,sizeof(char));
    }

  //Fill Runnable_Name_Idx buffer
  for(i=0;i<(runn_id_counter+1);i=i+1)
    {
      strcpy(Runnable_name,Runnables_ID_Name[i]);    
      if(!Check_presence(Runnables_Name_Idx_buffer, Runnable_name,i)) //if Runnable_name is present in Runnable_Name_Idx_buffer, Check_presence returns 1; else it returns 0;
	{
	  strcpy(Runnables_Name_Idx_buffer[Nb_runnables],Runnable_name);
	  Nb_runnables++;
	}
      strcpy(Runnable_name,eraser);
    }

  //Runnables_Name_Idx 
  //Now, we scan Runnables_Name_Idx_buffer and consider single occurences of Runnable names. These Runnable names are put in the Runnables_Name_Idx array.
  //Thus, Runnables_Name_Idx lists the Runnables as they appear in the application graph (each Runnable name is listed only once).
  char** Runnables_Name_Idx;
  Runnables_Name_Idx = (char**)calloc(Nb_runnables, sizeof(char*));
  for(i=0;i<Nb_runnables;i++)
    {
      Runnables_Name_Idx[i] = (char*)calloc(max_size_runnable_name,sizeof(char));
      strcpy(Runnables_Name_Idx[i],Runnables_Name_Idx_buffer[i]);
    }

  //Run_Idx is an array of structures. Each structure describes a Runnable (number of computation instructions, number of label access instructions, number of packets the Runnable will need to exchange if it is executed on a core and the other Runnables on other cores).
  Run_Idx = initialise_Runnables(Nb_runnables);
  strcpy(Runnable_name,eraser); //clean the Runnable_name buffer;
   
  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                          Describe the runnables                                           *//
  //*                                                                                                           *//
  //*************************************************************************************************************//

  fprintf(stderr, "Parsing Runnables.txt trace\n");

  // Extract instruction information
  int Runn_idx = 0;
  int Nb_instructions;
  int Nb_Labels_accesses;
  int Nb_packets;
   
  fp = fopen((inputFolder + "/Runnables.txt").c_str() , "r");
  if(fp == NULL) {
    perror("Runnables.txt not found in the specified directory");
    return(-1);
  }
  char* Runnable_id;
  Runnable_id = (char*)calloc(max_size_runnable_name,sizeof(char));
  while(fgets (line, size_line, fp)!=NULL)
    {
      sscanf( line, "%s %s %d %d %d", Runnable_id, Runnable_name, &Nb_instructions, &Nb_Labels_accesses, &Nb_packets);
      Runn_idx = Get_Runnable_idx(Runnables_Name_Idx,Runnable_name);
      Run_Idx[Runn_idx].Nb_instructions = Nb_instructions;
      Run_Idx[Runn_idx].Nb_label_accesses = Nb_Labels_accesses;
      Run_Idx[Runn_idx].Nb_packets = Nb_packets;
      strcpy(Runnable_name,eraser); //clean the Runnable_name buffer;
    }
  fclose(fp);
  strcpy(Runnable_name,eraser); //clean the Runnable_name buffer;


  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                   Compute the energy consumption of NoC                                   *//
  //*                                                                                                           *//
  //*************************************************************************************************************//

  //fprintf(stderr, "Parsing noc trace\n");

  // opening file for reading 
  fp = fopen((inputFolder + "/OUTPUT_NoC_Traces.csv").c_str(), "r");
  if(fp == NULL) {
    perror("OUTPUT_NoC_Traces.csv not found in the specified directory");
    return(-1);
  }

  // Read each packet and compute distance travelled
  fgets(line, size_line, fp);
  while (fgets(line, size_line, fp) != NULL) {
    int srcX, srcY, dstX, dstY, latency;
    sscanf(line,
	   "%*d %*c %*d %*c %*d %*c %*c %d %d %*c %*c %*c %d %d %*c %*c %*d %*c %*d %*c %d",
	   &srcX, &srcY, &dstX, &dstY, &latency);
    r = abs(srcX - dstX) + abs(srcY - dstY);
    if (r > 0) {
      N_mesg = N_mesg + 1;
      Dist_vector[r] = Dist_vector[r] + 1;
      Duration_packet = Duration_packet + latency;
      Duration_flit_hop = Duration_flit_hop
	+ (latency - N_flt_pack * N_clk_flt / clk) / r; //time (in ns) for a flit to realise a hop (this time is split into time spent inside the router and inside the link)
      N_clk_flt = (((N_mesg - 1) * N_clk_flt
		    + (latency - N_flt_pack * N_clk_flt / clk) * clk / r) / N_mesg);
    }
  }
  fclose(fp);

  // Compute the overall number of remote messages 
  for(i=1;i<Size_Dist_vector;i=i+1)
    {
      S = S+Dist_vector[i]*(i+2);
    }

  Mean_duration_packet = Duration_packet/N_mesg; // Average packet latency 
  Mean_duration_flit = Duration_flit_hop/N_mesg; // Average flit latency 
  // Debug 
  //printf("N_clk_flt : %f\n",N_clk_flt);
  //printf("Mean_duration_packet : %f\n",Mean_duration_packet);
  //printf("Mean_duration_flit_hop : %f\n",Mean_duration_flit);
  //printf("N_mesg : %d\n",N_mesg);
  // End Debug 

    
  // opening file for reading the values of N_clk_flt_link and N_clk_flt_router
  fp = fopen((configFolder + "/../platform/dcConfiguration.hxx").c_str() , "r");
  if(fp == NULL) {
    perror("dcConfiguration.hxx not found in the specified directory");
    return(-1);
  }

  line_counter = 0;
  // Read each packet and compute distance travelled  
  while(line_counter<7)
    {
      fgets (line, size_line, fp);
      line_counter++;
    }

  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %f", &N_clk_flt_link);
  fgets (line, size_line, fp);
  sscanf( line, "%*s %*s %f", &N_clk_flt_router);

  //     printf("N_clocks_flit_link : %f\n",N_clk_flt_link);
  //     printf("N_clocks_flit_router : %f\n",N_clk_flt_router);
  line_counter = 0;
  fclose(fp);


  //Compute total energy consumed by the NoC
  //Routers with 5 inputs
  E_static_NoC = ((((arbiter_static[0]*Number_VC_per_input+arbiter_static[1])+(buffer_static[0]*pow(Number_VC_per_input,2)+buffer_static[1]*Number_VC_per_input+buffer_static[2])+(crossbar_static)+(links_static[0]*pow(Number_VC_per_input,2)+links_static[1]*Number_VC_per_input+links_static[2]))*exec_time*1E-9)*1E+6)*N_5;//µJ
  //Routers with 4 inputs
  E_static_NoC = E_static_NoC + ((((arbiter_static[0]*Number_VC_per_input+arbiter_static[1])+(buffer_static[0]*pow(Number_VC_per_input,2)+buffer_static[1]*Number_VC_per_input+buffer_static[2])+(crossbar_static)+(links_static[0]*pow(Number_VC_per_input,2)+links_static[1]*Number_VC_per_input+links_static[2]))*exec_time*1E-9)*1E+6)*(4/5.0)*N_4;//µJ
  //Routers with 3 inputs
  E_static_NoC = E_static_NoC + ((((arbiter_static[0]*Number_VC_per_input+arbiter_static[1])+(buffer_static[0]*pow(Number_VC_per_input,2)+buffer_static[1]*Number_VC_per_input+buffer_static[2])+(crossbar_static)+(links_static[0]*pow(Number_VC_per_input,2)+links_static[1]*Number_VC_per_input+links_static[2]))*exec_time*1E-9)*1E+6)*(3/5.0)*N_3;//µJ
  //Routers with 2 inputs
  E_static_NoC = E_static_NoC + ((((arbiter_static[0]*Number_VC_per_input+arbiter_static[1])+(buffer_static[0]*pow(Number_VC_per_input,2)+buffer_static[1]*Number_VC_per_input+buffer_static[2])+(crossbar_static)+(links_static[0]*pow(Number_VC_per_input,2)+links_static[1]*Number_VC_per_input+links_static[2]))*exec_time*1E-9)*1E+6)*(2/5.0)*N_2;//µJ

  E_dynamic_NoC = ((clk*1E+3/config_freq)*(S*N_flt_pack)*(((arbiter_dynamic)+(buffer_dynamic[0]*pow(Number_VC_per_input,2)+buffer_dynamic[1]*Number_VC_per_input+buffer_dynamic[2])+(crossbar_dynamic))*N_clk_flt_router+(links_dynamic[0]*pow(Number_VC_per_input,2)+links_dynamic[1]*Number_VC_per_input+links_dynamic[2])*N_clk_flt_link)/(clk*1E+9))*1E+6;// S contains already the number of messages for each travelled distance, E_dynamic_NoC is in (µJ)
  E_total_NoC = E_dynamic_NoC+E_static_NoC; //µJ
     
  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*               Compute energy consumption of instructions (Computation and label accesses)                 *//
  //*                                                                                                           *//
  //*************************************************************************************************************//

  // Read Instruction Lengths 
  fp = fopen((inputFolder + "/Instruction_fixed_Power.txt").c_str(), "r");
  if(fp == NULL) {
    perror((inputFolder + "/Instruction_fixed_Power.txt not found").c_str());
    return(-1);
  }
  int inst_count=0;
  // Read the size of an instruction and compute the energy consumed
  while(fgets (line, size_line, fp)!=NULL)
    {
      sscanf( line, "%*s %*c %*d %*c %d", &Ins_length);
      //fprintf(stderr, "%d\n", Ins_length);
      inst_count++;
      // Determine Power of the instruction
      //First, determine the complexity of the instruction
      //get_Energy_Instruction(struct cpu **library, int lib, int Inst_length,int Inst_complex, float clock);
      Inst_complex = Select_complexity_inst(inst_complex_min,inst_complex_max);
      Tot_energy_exec = Tot_energy_exec + get_Energy_Instruction(library,lib,Ins_length,Inst_complex,clk); //Dynamic-energy J
      Complexity_mean = Complexity_mean + (float)Inst_complex;
      Time_ins = Time_ins+Ins_length;
    }
  fclose(fp);
  Complexity_mean= Complexity_mean/inst_count;
  Power_dynamic_mean = ((library[lib]->table_complex_pow)[(int)(Complexity_mean)-1]);;
  // Compute total number of messages (local or remote) and compute the energy consumed 
  /*
    fp = fopen("./Label_accesses_Power.txt" , "r");
    if(fp == NULL) {
    perror("Label_accesses_Power.txt not found in the specified directory");
    return(-1);
    }
    while(fgets (line, size_line, fp)!=NULL)
    {
    sscanf( line, "%*s %*c %*d %*c %d %*c %d", &label_data, &rd_or_wr); // For New

    //Determine number of packets for the label accesses (remote and local)
    packets = packets + (label_data/(N_flt_pack*8*N_bytes_flit)) + (1-rd_or_wr); //label_data is in bits
    if((label_data%(N_flt_pack*8*N_bytes_flit))>0)
    {
    packets = packets+1;
    }
    }
    total_flits = packets*N_flt_pack;//Use label set (local or remote)
    remote_flits = N_mesg*N_flt_pack; //Additionally use RB and SB 

  */

  // Determine the theoretical number of packets
  // Go through the list of all recorded executed Runnables and determine the theoretical total number of packets
  fp = fopen((inputFolder + "/Instruction_fixed_Power.txt").c_str() , "r");
  if(fp == NULL) {
    perror("Instruction_fixed_Power.txt not found in the specified directory");
    return(-1);
  }

  while(fgets (line, size_line, fp)!=NULL)
    {
      sscanf( line, "%s %*c %*d %*c %d %*c %d", Runnable_name, &label_data, &rd_or_wr); // For New
      //fprintf(stderr, "Runnable Name : %s\n",Runnable_name);
	 
      Runn_idx = Get_Runnable_idx(Runnables_Name_Idx,Runnable_name);
      packets = packets + Run_Idx[Runn_idx].Nb_packets;

    }
  total_flits = packets*N_flt_pack;//Use label set (local or remote)
  remote_flits = N_mesg*N_flt_pack; //Additionally use RB and SB 
    

  // Compute the energy of instructions (label accesses)

  //Dynamic energy of accesses to the label sets (of all cores)
  Dyn_energy_label_set = Dynamic_energy_read_port_label*total_flits;//nJ
      
  //Dynamic energy of accesses to the RB SB buffers
  Dyn_energy_RB_SB = 2*Dynamic_energy_read_port_RB_SB*remote_flits;//nJ

  Dyn_energy_labels = Dyn_energy_label_set+Dyn_energy_RB_SB;//nJ

  Stat_energy_label_set = (1+Percentage_refresh_of_leakage/100)*(exec_time*1E-9)*Static_power_bank_label*Nb_banks_labels;//J
  Stat_energy_WRB = Static_power_bank_WRB*Nb_banks_buffers*(exec_time*1E-9);//J
  Stat_energy_REB = Static_power_bank_REB*Nb_banks_buffers*(exec_time*1E-9);//J
  Stat_energy_PRB = Static_power_bank_PRB*Nb_banks_buffers*(exec_time*1E-9);//J
  Stat_energy_RB_SB = Static_power_bank_RB_SB*Nb_banks_buffers*(exec_time*1E-9);//J

  Stat_energy_labels=Stat_energy_label_set+Stat_energy_RB_SB;//J
  Tot_energy_label_set = Stat_energy_label_set+(Dyn_energy_label_set*1E-9);//J
  Tot_energy_RB_SB = Stat_energy_RB_SB+Dyn_energy_RB_SB*1E-9;//J
 
  Tot_energy_labels = Tot_energy_label_set+Tot_energy_RB_SB;//J
      
  prop2 = Stat_energy_labels;
  prop1 = Dyn_energy_labels*1E-9;//J
  //prop1 = (Dyn_energy_labels)/(Dyn_energy_labels+Stat_energy_labels);
  //prop1 = (Stat_energy_labels)/(Stat_energy_labels+Dyn_energy_labels);
  //printf("Dyn_energy_labels : %f\n",prop1);
  //printf("Stat_energy_labels : %f\n",prop2);
      
  //Tot_energy_remote_labels = get_Energy_Instruction(N_flits, Power_dynamic); //Dynamic Energy
  Tot_energy_cpu_idle = ((exec_time*(1E-9)*N_processors)*Power_static)*1E+6;//µJ

  Tot_energy_cores = (Tot_energy_cpu_idle + Tot_energy_exec*1E+6 + Tot_energy_labels*1E+6 + Tot_energy_preemptions*1E-3 + (Stat_energy_WRB+Stat_energy_REB+Stat_energy_PRB)*1E+6);//µJ
  Tot_energy = Tot_energy_cores+E_total_NoC;//µJ
     
  float E_static_cores=0;
  float E_dynamic_cores=0;

  float E_static_platform=0;
  float E_dynamic_platform=0;


  E_static_cores = Tot_energy_cpu_idle + (Stat_energy_label_set+Stat_energy_WRB+Stat_energy_REB+Stat_energy_PRB+Stat_energy_RB_SB)*1E+6;//µJ;

  E_dynamic_cores = Tot_energy_preemptions + Tot_energy_exec*1E+9 + Dyn_energy_label_set + Dyn_energy_RB_SB;//nJ;

  E_static_platform = E_static_cores+E_static_NoC;
  E_dynamic_platform = (E_dynamic_cores+E_dynamic_NoC)*1E-3;//nJ;
  //     printf("%f\n", Tot_energy_preemptions);
  //     printf("%f\n", Tot_energy_exec*1E+9);
  //     printf("%f\n", Dyn_energy_label_set);
  //     printf("%f\n", Dyn_energy_RB_SB);


  //*************************************************************************************************************//
  //*                                                                                                           *//
  //*                                     Generate Output                                                       *//
  //*                                                                                                           *//
  //*************************************************************************************************************//
  printf("\n");
  printf("**************************************************************\n");
  printf("*                                                            *\n");
  printf("* Estimation of the total energy consumption of an           *\n");
  printf("* application running on a multi-processor compute platform. *\n");   
  printf("* The estimated energy is mapping dependednt.                *\n");
  printf("* The energy consumption computation, takes into account the *\n");
  printf("* energy consumption of cores (instruction execution,        *\n");
  printf("* label accesses (local and remote), context switch) and     *\n");
  printf("* the energy consumption of NoC.                             *\n");
  printf("*                                                            *\n");
  printf("**************************************************************\n");
  printf("\n");
  printf("\n");
  printf(" #Input Configuration Information#\n");
  printf("\n");
  printf("    Type of the core : ");
  if(!strcmp(cpu_name,"ARM"))
    {
      printf("ARM_A15\n");
    }
  if(!strcmp(cpu_name,"ALP"))
    {
      printf("ALPHA21364\n");
    }
  printf("    System  Operating  Frequency : %0.2f (GHz)\n", clk);
  printf("    Total execution time : %d (ns)\n", (int)exec_time);
  printf("    Number  of  Rows  in  Platform : %d\n", Rows);
  printf("    Number  of  Columns in  Platform : %d\n", Columns);
  printf("    Number  of  Cores   in  System : %d\n", N_processors);
  printf("    Packets Exchanged through NoC for Remote Label Accesses : %d\n", N_mesg);
  // Output energies 
  printf("\n");
  printf("\n");
  printf(" #Summary of Results#\n");
  printf("\n");
  printf("    Energy of NoC (dynamic and static) : %0.9f (µJ)\n",E_total_NoC);
  printf("    Energy of cores (dynamic and static due to instruction execution, local label accesses and idle state) : %0.4f (µJ) \n", Tot_energy_cores);
  printf("    Total Energy of the system is : %0.4f (µJ)\n",Tot_energy);
  prop1 = ((Tot_energy_cores/Tot_energy));
  prop2 = ((E_total_NoC/Tot_energy));
  printf("    Proportion of total energy consumption by cores : %0.2f%%\n",prop1*100);
  printf("    Proportion of total energy consumption by NoC : %0.2f%%\n",prop2*100);
  printf("\n");
  printf("\n");
  printf("\n");
  printf(" #Detailed Results#\n");
  printf("\n");
  //printf("    Number of virtual channels : %d\n", N_channels);
  printf("    Average packet latency : %0.4f (ns)\n", Mean_duration_packet);
  printf("    Total execution time of computational instructions : %d (cycles)\n", Time_ins);
  printf("    Data size for label accesses (local and remote) : %d (number of packets)\n",packets);
  printf("    Data size for local label accesses : %d (number of packets)\n",packets-N_mesg);
  printf("    Data size for remote label accesses : %d (number of packets)\n",N_mesg);
  printf("\n");
  printf("    Power of an idle cpu : %0.4f (W) static power obtained by McPAT\n", Power_static);
  printf("    Power of a busy cpu : %0.4f (W) computed as %0.3f times the energy of idle core at 200 MHz\n", Power_dynamic_mean,0.2*Complexity_mean-0.1);
  printf("\n");
  printf("    #Breakdown of Energy Consumption by the platform#\n");
  printf("    Static energy platform : %f (µJ)\n",E_static_platform);
  printf("    Dynamic energy platform : %f (µJ)\n",E_dynamic_platform); 
  printf("\n");

  printf("    #Breakdown of Energy Consumption by Cores#\n");
  printf("    Static energy cores : %f (µJ)\n",E_static_cores);
  printf("    Dynamic energy cores : %f (µJ)\n",E_dynamic_cores*1E-3);
  printf("    Total energy consumed by labels at core levels : %0.4f (µJ)\n",Tot_energy_labels*1E+6);
  printf("    Dynamic energy consumed by label accesses at core levels : %0.4f (µJ)\n",Dyn_energy_labels*1E-3);
  printf("    Dynamic energy consumed by instruction executions : %0.4f (µJ)\n",Tot_energy_exec*1E+6);
  printf("    Energy of interbuffer runnable transfers : %0.4f (µJ) \n", Tot_energy_preemptions*1E-3);
  printf("    Total energy consumed by idle cpu : %0.4f (µJ)\n",Tot_energy_cpu_idle);
  printf("\n");
  printf("    #Breakdown of Energy Consumption by NoC#\n");
  printf("    Static energy NoC : %0.9f (µJ)\n",E_static_NoC);
  printf("    Dynamic energy NoC : %0.9f (nJ)\n",E_dynamic_NoC*1E+3);
  printf("\n");

  prop1 = Tot_energy_cpu_idle/Tot_energy_cores;
  prop2 = ((Tot_energy_exec*1E+6 + Tot_energy_labels*1E+6 + Tot_energy_preemptions*1E-3 + (Stat_energy_WRB+Stat_energy_REB+Stat_energy_PRB)*1E+6)/Tot_energy_cores);
  printf("    Proportion of cores energy due to labels (dynamic and static) and instruction execution (simple dynamic and interbuffers runnable transfers) : %0.2f%%\n",prop2*100);
  printf("    Proportion of cpu energy due to idle state : %0.2f%%\n",prop1*100);
  printf("\n");
  printf("\n");

  printf("**************************************************************\n");
  printf("\n");
  return(0);
}


