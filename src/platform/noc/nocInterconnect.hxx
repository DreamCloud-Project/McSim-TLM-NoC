////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef DREAMCLOUD__PLATFORM_SCLIB__NOC_PPA__NOCINTERCONNECT_HXX
#define DREAMCLOUD__PLATFORM_SCLIB__NOC_PPA__NOCINTERCONNECT_HXX

////////////////////
//    INCLUDES    //
////////////////////
#include <map>
#include <sstream>
#include <string>

#include "../../Platform_Src/dcSimuParams.hxx"
#include "../../Platform_Src/noc/packet.hxx"

////////////////////
//      USING     //
////////////////////
using std::map;
using std::ostringstream;

namespace dreamcloud {
namespace platform_sclib {
namespace noc_ppa {

class nocInterconnect: sc_module {
public:
	sc_in<Packet> packet_in;
	sc_out<Packet> packet_out;

	int nbPkts;
	int nbReadRequestPktsSent = 0;
	int nbReadAnswerPktsSent = 0;
	int nbWriteRequestPktsSent = 0;
	map<int, unsigned int> routeToNbPkts;
	unsigned long int nbUpdatePktList = 0;

	SC_HAS_PROCESS(nocInterconnect);
	nocInterconnect(sc_module_name name, dcSimuParams params_) :
			nbPkts(0), params(params_){
		SC_METHOD(receive_packet);
		sensitive << packet_in;
		dont_initialize();

		SC_METHOD(deliver_packet);
		sensitive << deliver_PKT_to_PE_signal_notify;

		SC_METHOD(update_packet_list);
		sensitive << update_event;
		dont_initialize();
	}

	inline ~nocInterconnect() {
	}

	class packetAttributes {
	public:
		packetAttributes() :
				active_(false) {
		}
		~packetAttributes() {
		}

		inline void add_packet_to_list(const Packet &packet) {
			interference_list_.push_back(packet);
		}
		inline bool is_active() const {
			return active_;
		}
		inline void activate() {
			active_ = true;
		}
		inline void deactivate() {
			active_ = false;
		}
		inline void set_remaining_time(sc_time time_value) {
			remaining_time_ = time_value;
		}
		inline sc_time get_remaining_time() const {
			return remaining_time_;
		}
		inline void update_last_active() {
			last_active_ = sc_time_stamp();
		}
		inline vector<Packet>& get_intereference_list_ref() {
			return interference_list_;
		}
		inline const vector<Packet>& get_intereference_list() const {
			return interference_list_;
		}
		inline void add_link_to_route(int link) {
			route_.push_back(link);
		}
		inline bool route_defined() const {
			return !route_.empty();
		}
		inline vector<int>& get_route_ref() {
			return route_;
		}
		inline const vector<int>& get_route() const {
			return route_;
		}

		sc_time update_remaining_time();

		////////////////////
		//    TYPEDEFS    //
		////////////////////
		typedef vector<Packet> interferenceList;

	private:
		bool active_;
		sc_time last_active_;
		sc_time remaining_time_;
		interferenceList interference_list_;
		vector<int> route_; // Each integer in the vector represents a link
	};

	//////////////////////////
	//      COMPARATORS     //
	//////////////////////////
	struct packetListCmp {

		bool operator()(const Packet &p1, const Packet &p2) {
			bool res;
			if (p1.get_priority() != p2.get_priority()) {
				res = p1.get_priority() < p2.get_priority();
			} else {
				res = p1.get_id() < p2.get_id();
			}
			return res;
		}
	};

	////////////////////
	//    TYPEDEFS    //
	////////////////////
	typedef map<Packet, packetAttributes, packetListCmp> packetList;

private:
	void receive_packet();
	void deliver_packet();
	void update_packet_list();
	bool route_interference(const Packet &packet1, const Packet& packet2);
	bool calculate_route(const Packet& packet, vector<int>& route);
	void packets_in_NoC();
	void inc_route_cnt(int route);

	packetList packet_list;
	packetList packet_list_delivered;
	vector<Packet> toBeDelivered;
	sc_event update_event;
	sc_event deliver_PKT_to_PE_signal_notify;
	packetList::iterator it_packet_delivered;
	dcSimuParams params;
};

}
}
}

#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
