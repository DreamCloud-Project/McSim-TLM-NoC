////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include "../../Platform_Src/noc/nocInterconnect.hxx"

#include "../../Platform_Src/dcConfiguration.hxx"
#include "../../Platform_Src/dcConfiguration.hxx"
#include "../../Platform_Src/simulators-commons/utils/dcVector.hxx"
#include "../../Platform_Src/simulators-commons/utils/Math.hxx"

#define DBG_NOC( x ) //

////////////////////
//      USING     //
////////////////////
using namespace std;

namespace dreamcloud {
namespace platform_sclib {
namespace noc_ppa {

sc_time nocInterconnect::packetAttributes::update_remaining_time() {
	if ((sc_time_stamp() - last_active_) >= remaining_time_)
		remaining_time_ = SC_ZERO_TIME;
	else
		remaining_time_ -= sc_time_stamp() - last_active_;

	return remaining_time_;
}

void nocInterconnect::deliver_packet() {
	Packet temp;
	if (!toBeDelivered.empty()) {
		temp = toBeDelivered.front();
		packet_out.write(toBeDelivered.front());
		toBeDelivered.erase(toBeDelivered.begin());
		next_trigger(SC_ZERO_TIME);
	}
}

void nocInterconnect::receive_packet() {
	Packet packet(packet_in.read());
	packet.set_id(++nbPkts);

	if (packet.isWrite()) {
		nbWriteRequestPktsSent++;
	} else {
		if (packet.isReadResponse()) {
			nbReadAnswerPktsSent++;
		} else {
			nbReadRequestPktsSent++;
		}
	}

	//DBG_NOC( debug(0, "[nocInterconnect::receive_packet]   Received Packet \t  %s TIME: %s", packet.to_str(), sc_time_stamp()); )

	std::pair<Packet, packetAttributes> foo;
	foo = make_pair(packet, packetAttributes());
	packet_list.insert(foo);
	packetList::iterator it_packet(packet_list.find(packet));
	calculate_route(packet, it_packet->second.get_route_ref());
	int hops = (it_packet->second.get_route_ref()).size();

	unsigned long int routerDelay = ONE_ROUTER_DELAY
			* params.getCoresPeriodInNano();
	int remainingTime = (routerDelay * (hops + 1)) + PACKET_SIZE_IN_BYTES; // +(ONE_HOP_DELAY*hops)    //ignore link delay
	it_packet->second.set_remaining_time(sc_time(remainingTime, SC_NS));
	it_packet->first.set_delivery_time_no_contention(
			sc_time(remainingTime + sc_time_stamp().value() / 1E3, SC_NS));

	// Compute interference list for the new packet
	// and add new packet route to interference list
	// of packets which intersect and have lower priority
	for (packetList::iterator it_pl(packet_list.begin());
			it_pl != packet_list.end(); ++it_pl) {
		if (it_pl->first == packet)
			continue;

		if (route_interference(packet, it_pl->first)) {
			if (packet.get_priority() < it_pl->first.get_priority()) {
				//DBG_NOC( debug(5, "[nocInterconnect::receive_packet] Adding packet [%d] to interference list of packet [%d]",
				//packet.get_id(), it_pl->first.get_id()); )
				it_pl->second.add_packet_to_list(packet);
			} else {
				//DBG_NOC( debug(5, "[nocInterconnect::receive_packet] Adding packet [%d] to interference list of packet [%d]",
				//it_pl->first.get_id(), packet.get_id()); )
				it_packet->second.add_packet_to_list(it_pl->first);
			}
		}
	}
	update_packet_list();
}

void nocInterconnect::update_packet_list() {
	sc_time next_terminate(SC_ZERO_TIME);

	for (packetList::iterator it_pl(packet_list.begin());
			it_pl != packet_list.end();) {

		// If the packet is active
		if (it_pl->second.is_active()) {
			it_pl->second.update_remaining_time();
			it_pl->second.update_last_active();

			//DBG_NOC( debug(5, "[nocInterconnect::update_packet_list] Packet [%d]; remaining time [%s]",
			//             it_pl->first.get_id(), it_pl->second.get_remaining_time()); )

			// If the packet has still remaining time before delivery
			if (it_pl->second.get_remaining_time().value()) {

				// Verifying if there is at least one active packet in the interference list.
				// If yes, packet must be deactivated.
//				packetAttributes::interferenceList interf_list(
//						it_pl->second.get_intereference_list_ref());
//				for (packetAttributes::interferenceList::const_iterator it_il(
//						interf_list.begin()); it_il != interf_list.end();) {
				for (packetAttributes::interferenceList::iterator it_il(
						it_pl->second.get_intereference_list_ref().begin());
						it_il
								!= it_pl->second.get_intereference_list_ref().end();
						) {
					nbUpdatePktList++;
					packetList::const_iterator it_packet(
							packet_list.find(*it_il));

					// If the interference packet has not been found in the main list, it means that it has terminated.
					// In this case, the packet is removed from the interference list. This is not totally mandatory.
					// It is just to reduce the number of iterations in future updates.
					if (it_packet == packet_list.end()) {
						//interf_list.erase(it_il);
						it_pl->second.get_intereference_list_ref().erase(it_il);
						//++it_il;
					}

					// Else if the interference packet is still in the NoC
					// We deactivate the current packet if this interference
					// packet is active
					else {
						if (it_packet->second.is_active()) {
							//DBG_NOC( debug(5, "[nocInterconnect::update_packet_list] Deactivating packet %s", it_pl->first); )
							it_pl->second.deactivate();
							break;
						}
						++it_il;
					}
				}

				// Updating next update time stamp
				if (it_pl->second.is_active()) {
					sc_time remaining_time(it_pl->second.get_remaining_time());
					if (next_terminate == SC_ZERO_TIME
							|| remaining_time < next_terminate)
						next_terminate = remaining_time;
				}
				++it_pl;
			}

			// Else the packet must be delivered NOW
			else {
				toBeDelivered.push_back(it_pl->first);
				deliver_PKT_to_PE_signal_notify.notify();
				//next_trigger(NEXT_TRIGGER_INTERVAL, SC_NS);
				packet_list.erase(it_pl++);
			}
		}

		// Else in the case when the packet is inactive
		else {
			bool some_active_packet(false);
			// Verifying if all packets in the interference list are deactivated.
			// If yes, packet can be activated.
//			packetAttributes::interferenceList interf_list(
//					it_pl->second.get_intereference_list_ref());
//			for (packetAttributes::interferenceList::const_iterator it_il(
//					interf_list.begin()); it_il != interf_list.end();) {
			for (packetAttributes::interferenceList::iterator it_il(
					it_pl->second.get_intereference_list_ref().begin());
					it_il != it_pl->second.get_intereference_list_ref().end();
					) {
				packetList::const_iterator it_packet(packet_list.find(*it_il));
				nbUpdatePktList++;
				if (it_packet == packet_list.end()) {
					//interf_list.erase(it_il);
					it_pl->second.get_intereference_list_ref().erase(it_il);
					//++it_il;
				} else {
					if (it_packet->second.is_active()) {
						some_active_packet = true;
						break;
					}
					++it_il;
				}
			}

			if (!some_active_packet) {
				// DBG_NOC( debug(5, "[nocInterconnect::update_packet_list] Activating packet %s", it_pl->first); )

				it_pl->second.activate();
				it_pl->second.update_last_active();

				// Updating next update time stamp
				sc_time remaining_time(it_pl->second.get_remaining_time());
				if (next_terminate == SC_ZERO_TIME
						|| remaining_time < next_terminate)
					next_terminate = remaining_time;
			}

			++it_pl;
		}
	}

	if (next_terminate != SC_ZERO_TIME) {
		// DBG_NOC( debug(5, "[nocInterconnect::update_packet_list] Next update will occur in [%s]", next_terminate); )
		update_event.cancel();
		update_event.notify(next_terminate);
	}
}

bool nocInterconnect::route_interference(const Packet &packet1,
		const Packet& packet2) {
	packetList::const_iterator it_p1(packet_list.find(packet1));
	packetList::const_iterator it_p2(packet_list.find(packet2));
	if (it_p1 == packet_list.end()) {
		std::cerr
				<< "[nocInterconnect::route_interference] Cannot calculate route interference: packet ["
				<< packet1.get_id() << "] has not been found";
		exit(-1);
	}

	if (it_p2 == packet_list.end()) {
		std::cerr
				<< "[nocInterconnect::route_interference] Cannot calculate route interference: packet ["
				<< packet2.get_id() << "] has not been found";
		exit(-1);
	}

	return dcVector<int>::intersection(it_p1->second.get_route(),
			it_p2->second.get_route());
}

bool nocInterconnect::calculate_route(const Packet& packet,
		vector<int>& route) {
	pair<unsigned int, unsigned int> source(packet.get_source()), destination(
			packet.get_destination());
	int delta_x(destination.second - source.second), delta_y(
			destination.first - source.first);
	int curr_x(source.second), curr_y(source.first);

	route.clear();

	//DBG_NOC( debug(0, "[nocInterconnect::calculate_route] Defining route of Packet %s", packet.get_id()); )

	if (delta_x != 0) {
		bool left(delta_x < 0 ? true : false);

		for (unsigned int i(curr_x); i != destination.second;
				left ? --i : ++i) {
			int r;
			if (left) {
				//DBG_NOC( debug(10, "[nocInterconnect::calculate_route] link X (%d,%d)-(%d,%d)", curr_y, i - 1, curr_y, i); )
				if (params.getFullDuplex()) {
					r = pairing_function(pairing_function(curr_y, i),
							pairing_function(curr_y, i - 1));
				} else {
					r = pairing_function(pairing_function(curr_y, i - 1),
							pairing_function(curr_y, i));
				}
				route.push_back(r);
				curr_x = i - 1;
			} else {
				//DBG_NOC( debug(10, "[nocInterconnect::calculate_route] link X (%d,%d)-(%d,%d)", curr_y, i, curr_y, i + 1); )
				r = pairing_function(pairing_function(curr_y, i),
						pairing_function(curr_y, i + 1));
				route.push_back(r);
				curr_x = i + 1;
			}
			inc_route_cnt(r);
		}
	}

	//DBG_NOC( debug(10, "[nocInterconnect::calculate_route] Current_X: %d, Current_Y: %d", curr_x, curr_y); )

	if (delta_y != 0) {
		bool up(delta_y < 0 ? true : false);

		for (unsigned int i(curr_y); i != destination.first; up ? --i : ++i) {
			int r;
			if (up) {
				//    DBG_NOC( debug(10, "[nocInterconnect::calculate_route] link X (%d,%d)-(%d,%d)", i - 1, curr_x, i, curr_x); )
				if (params.getFullDuplex()) {
					r = pairing_function(pairing_function(i, curr_x),
							pairing_function(i - 1, curr_x));
				} else {
					r = pairing_function(pairing_function(i - 1, curr_x),
							pairing_function(i, curr_x));
				}
				route.push_back(r);
				curr_y = i - 1;
			} else {
				//  DBG_NOC( debug(10, "[nocInterconnect::calculate_route] link X (%d,%d)-(%d,%d)", i, curr_x, i + 1, curr_x); )
				r = pairing_function(pairing_function(i, curr_x),
						pairing_function(i + 1, curr_x));
				route.push_back(r);
				curr_y = i + 1;
			}
			inc_route_cnt(r);
		}
	}
	return !route.empty();
}

void nocInterconnect::inc_route_cnt(int route) {
	map<int, unsigned int>::const_iterator it = routeToNbPkts.find(route);
	if (it != routeToNbPkts.end()) {
		routeToNbPkts[route] = it->second + 1;
	} else {
		routeToNbPkts[route] = 1;
	}
}

}
}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
