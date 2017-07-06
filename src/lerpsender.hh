#ifndef LERPSENDER_HH
#define LERPSENDER_HH

#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include "memory.hh"
#include "packet.hh"

#include <unordered_map>

#include <boost/array.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp> 
#include "delaunay_d_interp.h"
#include "p2.h"

using namespace std;

#define MAX_SEND_EWMA 300
#define MAX_REC_EWMA 300
#define MAX_RTT_RATIO 100
#define MAX_SLOW_REC_EWMA 100
#define NUM_SIGNALS 4
typedef tuple<double,double,double,double> SignalTuple;

#define SEND_EWMA(s) get <0>(s)
#define REC_EWMA(s)  get <1>(s)
#define RTT_RATIO(s) get <2>(s)
#define SLOW_REC_EWMA(s) get <3>(s)
typedef tuple<double,double,double> ActionTuple;
#define CWND_INC(a)  get <0>(a)
#define CWND_MULT(a) get <1>(a)
#define MIN_SEND(a)  get <2>(a)
typedef pair<SignalTuple,ActionTuple> PointObj;

#define DEFAULT_INCR 1
#define DEFAULT_MULT 1
#define DEFAULT_SEND 3
#define DEFAULT_SLOWR 4
string _stuple_str( SignalTuple t );
string _atuple_str( ActionTuple t );
string _point_str( PointObj p );
struct HashAction{
	size_t operator()(const ActionTuple& a) const{
		return (
			hash<double>()(get<0>(a)) ^ 
			hash<double>()(get<1>(a)) ^
			hash<double>()(get<2>(a))
		);

   }
};

struct HashSignal{
	size_t operator()(const SignalTuple& s) const{
		return (
			hash<double>()(get<0>(s)) ^ 
			hash<double>()(get<1>(s)) ^
			hash<double>()(get<2>(s)) ^
      hash<double>()(get<3>(s))
		);
	}
};
struct EqualSignal {
  inline bool operator()( const SignalTuple& s, const SignalTuple& t ) const{
		return (
						 (get<0>(s) == get<0>(t)) && 
						 (get<1>(s) == get<1>(t)) && 
						 (get<2>(s) == get<2>(t)) &&
             (get<3>(s) == get<3>(t))
				   );
  }
};
typedef unordered_map<SignalTuple,ActionTuple,HashSignal,EqualSignal> SignalActionMap;

struct CompareSignals {
	inline bool operator() (const SignalTuple& s, const SignalTuple& t) const{
		return (
						 (get<0>(s) < get<0>(t)) && 
						 (get<1>(s) < get<1>(t)) && 
						 (get<2>(s) < get<2>(t)) &&
						 (get<3>(s) < get<3>(t))
				   );
	}
};

typedef boost::accumulators::accumulator_set < double,
				boost::accumulators::stats<
				boost::accumulators::tag::median > > acc_median_t;

class PointGrid 
{

private:
	bool _track;
	// mutable vector< p2_t > _acc;
	// mutable vector< acc_median_t > _acc;
	p2_t _send_acc, _rec_acc, _rtt_acc, _slow_rec_acc;
public:
	// can access these two directly for now
  bool _debug;
	SignalActionMap _points; // map of signal to action
  void add_point( PointObj p );
	PointGrid( bool track = false );
	PointGrid( PointGrid & other, bool track );

	SignalActionMap::iterator begin();
	SignalActionMap::iterator end();
  int size();

  string str();

  void track( double s, double r, double t, double sr );
  SignalTuple get_median_signal ();

};

class LerpSender
{
private:
	PointGrid & _grid;
  Memory _memory;
  unsigned int _packets_sent, _packets_received;
  double _last_send_time;
  /* _the_window is the congestion window */
  double _the_window;
  double _intersend_time;
  unsigned int _flow_id;
  /* Largest ACK so far */
  int _largest_ack;
  vector< pair < array< double, 4 >, ActionTuple > > _signal_list;
  Delaunay_incremental_interp_d incr_triang();

public:
  LerpSender( PointGrid & grid );
  void reset( const double & tickno ); /* start new flow */
  void packets_received( const std::vector< Packet > & packets );
  double next_event_time( const double & tickno ) const;
  void update_actions( const Memory memory );
  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );
  LerpSender & operator=( const LerpSender & ) { assert( false ); return *this; }

	void add_inner_point ( const PointObj point, PointGrid & grid );
  ActionTuple interpolate_delaunay( double s, double r, double t, double sr );
	ActionTuple interpolate ( SignalTuple t );

};
#endif
