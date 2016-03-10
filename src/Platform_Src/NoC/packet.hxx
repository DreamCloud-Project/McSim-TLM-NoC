////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#ifndef DREAMCLOUD__PLATFORM_SCLIB__NOC_PACKET_HXX
#define DREAMCLOUD__PLATFORM_SCLIB__NOC_PACKET_HXX

#ifndef SC_USE_STD_STRING
#define SC_USE_STD_STRING
#endif

////////////////////
//    INCLUDES    //
////////////////////
#include <systemc.h>
#include <vector>
#include <string>
#include <sstream>
#include <utility>

namespace dreamcloud {
namespace platform_sclib {
namespace noc_ppa {

////////////////////
//      USING     //
////////////////////
using std::vector;
using std::string;
using std::pair;
using std::ostringstream;

class Packet {
public:
	inline void set_id(unsigned int id) {
		id_ = id;
	}
	inline void set_priority(unsigned int prio) {
		priority_ = prio;
	}
	inline void set_read_request_id(int info) {
		read_request_id = info;
	}
	inline void set_source(pair<unsigned int, unsigned int> src) {
		source_ = src;
	}
	inline void set_destination(pair<unsigned int, unsigned int> dst) {
		destination_ = dst;
	}
	inline void set_injection_time() {
		injection_time_ = sc_time_stamp();
	}
	inline void set_delivery_time() {
		delivery_time_ = sc_time_stamp();
	}
	inline void set_delivery_time_no_contention(sc_time t) const {
		delivery_time_no_contention_ = t;
	}

	inline void set_rd_wr(bool rd_wr) {
		rd_wr_ = rd_wr;
	}
	inline void set_requestedSize(unsigned int requestedSize) {
		requestedSize_ = requestedSize;
	}
	inline void set_writeSize(unsigned int writeSize) {
		writeSize_ = writeSize;
	}
	inline void set_req_resp(bool req_resp) {
		req_resp_ = req_resp;
	}
	inline void set_write_rq_ID(int write_rq_ID) {
		write_rq_ID_ = write_rq_ID;
	}
	inline void set_write_rq_size(int write_rq_size) {
		write_rq_size_ = write_rq_size;
	}
	inline void set_write_request_ID(int write_request_ID) {
		write_request_ID_ = write_request_ID;
	}
	inline void set_packet_size(int pkt_size) {
		pkt_size_ = pkt_size;
	}

	inline unsigned int get_id() const {
		return id_;
	}
	inline unsigned int get_delivery_time() const {
		return delivery_time_.value() / 1E3;
	}
	inline unsigned int get_injection_time() const {
		return injection_time_.value() / 1E3;
	}
	inline unsigned int get_latency() const {
		return delivery_time_.value() / 1E3 - injection_time_.value() / 1E3;
	}
	inline unsigned int get_latency_no_contention() const {
		return delivery_time_no_contention_.value() / 1E3 - injection_time_.value() / 1E3;
	}
	inline unsigned int get_priority() const {
		return priority_;
	}
	inline unsigned int get_read_request_id() const {
		return read_request_id;
	}
	inline pair<unsigned int, unsigned int> get_source() const {
		return source_;
	}
	inline pair<unsigned int, unsigned int> get_destination() const {
		return destination_;
	}
	inline bool isWrite() const {
		return rd_wr_;
	}
	inline int get_requestedSize() const {
		return requestedSize_;
	}
	inline int get_writeSize() const {
		return writeSize_;
	}
	inline bool isReadResponse() const {
		return req_resp_;
	}
	inline int get_write_rq_ID() {
		return write_rq_ID_;
	}
	inline int get_write_rq_size() {
		return write_rq_size_;
	}
	inline int get_write_request_ID() {
		return write_request_ID_;
	}
	inline int get_packet_size() {
		return pkt_size_;
	}

	string to_str() const {
		ostringstream stream("");
		stream << *this;
		return stream.str();
	}

	inline bool operator==(const Packet& p) const {
		return (p.id_ == id_ && p.priority_ == priority_ && p.read_request_id == read_request_id
				&& p.pkt_size_ == pkt_size_
				&& (p.source_.first == source_.first
						&& p.source_.second == source_.second)
				&& (p.rd_wr_ == rd_wr_ && p.requestedSize_ == requestedSize_
						&& p.writeSize_ == writeSize_
						&& p.req_resp_ == req_resp_)
				&& (p.write_rq_ID_ == write_rq_ID_
						&& p.write_request_ID_ == write_request_ID_
						&& p.write_rq_size_ == write_rq_size_)
				&& (p.destination_.first == destination_.first
						&& p.destination_.second == destination_.second));
	}

	inline Packet& operator=(const Packet& p) {
		id_ = p.id_;
		priority_ = p.priority_;
		read_request_id = p.read_request_id;
		source_.first = p.source_.first;
		source_.second = p.source_.second;
		destination_.first = p.destination_.first;
		destination_.second = p.destination_.second;
		injection_time_ = p.injection_time_;
		delivery_time_no_contention_ = p.delivery_time_no_contention_;
		delivery_time_ = p.delivery_time_;
		pkt_size_ = p.pkt_size_;

		rd_wr_ = p.rd_wr_;
		requestedSize_ = p.requestedSize_;
		writeSize_ = p.writeSize_;
		req_resp_ = p.req_resp_;
		write_rq_ID_ = p.write_rq_ID_;
		write_rq_size_ = p.write_rq_size_;
		write_request_ID_ = p.write_request_ID_;

		return *this;
	}

	inline bool operator<(const Packet& p) const {
		return (p.priority_ < priority_ || p.id_ < id_);
	}

	inline friend ostream& operator<<(ostream& os, Packet const &p) {
		os.flush();
		os << p.id_ << ",";
		os << p.priority_ << ",";
		os << p.read_request_id << ",";
		os << "(" << p.source_.first << " " << p.source_.second << ")" << ",";
		os << "(" << p.destination_.first << " " << p.destination_.second << ")"
				<< ",";
		os << p.injection_time_.value() / 1E3 << ",";
		os << p.delivery_time_.value() / 1E3 << ",";
		os << p.delivery_time_.value() / 1E3 - p.injection_time_.value() / 1E3
				<< ",";
		os
				<< p.delivery_time_no_contention_.value() / 1E3
						- p.injection_time_.value() / 1E3;
		os << "\n";
		return os;
	}

protected:
	// empty

private:
	unsigned int id_;
	unsigned int priority_;
	int read_request_id;
	pair<unsigned int, unsigned int> source_;
	pair<unsigned int, unsigned int> destination_;
	sc_time injection_time_;
	sc_time delivery_time_;
	mutable sc_time delivery_time_no_contention_;
	bool rd_wr_;
	unsigned int requestedSize_; //bytes to be read
	unsigned int writeSize_;
	bool req_resp_;

	int write_rq_ID_;
	int write_rq_size_;
	int write_request_ID_;
	int pkt_size_;

};

extern void sc_trace(sc_trace_file *tf, const Packet& p, const sc_string& name);

}
}
}

#endif

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
