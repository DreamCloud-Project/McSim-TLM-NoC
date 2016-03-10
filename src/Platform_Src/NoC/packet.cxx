////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include "packet.hxx"

namespace dreamcloud { namespace platform_sclib { namespace noc_ppa {

void sc_trace(sc_trace_file *tf, const Packet& p, const sc_string& name)
{
  sc_trace(tf, p.get_id(), name + ".id");
  sc_trace(tf, p.get_priority(), name + ".priority");
  sc_trace(tf, p.get_read_request_id(), name + ".info");
  sc_trace(tf, p.get_source().first, name + ".source.first");
  sc_trace(tf, p.get_source().second, name + ".source.second");
  sc_trace(tf, p.get_destination().first, name + ".destination.first");
  sc_trace(tf, p.get_destination().second, name + ".destination.second");
}

}}}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
