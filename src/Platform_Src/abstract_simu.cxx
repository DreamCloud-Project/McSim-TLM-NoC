////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include "dcSystem.hxx"
#include "dcSimuParams.hxx"

////////////////////
//      USING     //
////////////////////
using namespace dreamcloud::platform_sclib;

using namespace dreamcloud::platform_sclib::noc_ppa;
using namespace std;

int sc_main(int argc, char** argv) {

	// Parse arguments
	dcSimuParams params(argc, argv);
	if (params.getHelp()) {
		params.printHelp();
		exit(0);
	}

	// SystemC evaluation
	sc_core::sc_report_handler::set_actions("/IEEE_Std_1666/deprecated",
			sc_core::SC_DO_NOTHING);
	sc_clock clock("my_clock", params.getCoresPeriodInNano(), SC_NS, 0.5);
	dcSystem system("dcSsytem", params);
	system.clock(clock);

	// SystemC simulation
	sc_start();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
