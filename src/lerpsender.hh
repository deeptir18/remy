#ifndef LERPSENDER_HH
#define LERPSENDER_HH

#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include "memory.hh"
#include "packet.hh"

#include <unordered_map>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>

using namespace std;

#define MAX_SEND_EWMA 163840
#define MAX_REC_EWMA 163840
#define MAX_RTT_RATIO 163840

#define NUM_SIGNALS 3 
typedef tuple<double,double,double> SignalTuple;
#define SEND_EWMA(s) get <0>(s)
#define REC_EWMA(s)  get <1>(s)
#define RTT_RATIO(s) get <2>(s)
typedef tuple<double,double,double> ActionTuple;
#define CWND_INC(a)  get <0>(a)
#define CWND_MULT(a) get <1>(a)
#define MIN_SEND(a)  get <2>(a)
typedef pair<SignalTuple,ActionTuple> Point;

string _stuple_str( SignalTuple t );
string _atuple_str( ActionTuple t );
string _point_str( Point p );

struct HashSignal{
	size_t operator()(const SignalTuple& s) const{
		return (
			hash<double>()(get<0>(s)) ^ 
			hash<double>()(get<1>(s)) ^
			hash<double>()(get<2>(s))
		);
	}
};
struct EqualSignal {
  inline bool operator()( const SignalTuple& s, const SignalTuple& t ) const{
		return (
						 (get<0>(s) == get<0>(t)) && 
						 (get<1>(s) == get<1>(t)) && 
						 (get<2>(s) == get<2>(t))
				   );
  }
};
typedef unordered_map<SignalTuple,ActionTuple,HashSignal,EqualSignal> SignalActionMap;

struct CompareSignals {
	inline bool operator() (const SignalTuple& s, const SignalTuple& t) const{
		return (
						 (get<0>(s) < get<0>(t)) && 
						 (get<1>(s) < get<1>(t)) && 
						 (get<2>(s) < get<2>(t))
				   );
	}
};

class PointGrid 
{

private:
	bool _track;
	mutable vector< boost::accumulators::accumulator_set < double,
									boost::accumulators::stats<
									boost::accumulators::tag::median > > > _acc;
public:
	// can access these two directly for now
	vector<SignalTuple> _signals; // list of values for each signal, used to calc which box we're in
	SignalActionMap _points; // map of signal to action

	PointGrid();
	PointGrid( PointGrid & other ); 
	
	SignalActionMap::iterator begin();
	SignalActionMap::iterator end();
	int size(); 

	string str();

	void track( double s, double r, double t );
	SignalTuple get_median_signal (); 

};


class LerpSender
{
private:
	PointGrid & _grid;
  unsigned int _packets_sent, _packets_received;
  double _last_send_time;
  /* _the_window is the congestion window */
  double _the_window;
  double _intersend_time;
  unsigned int _flow_id;
  Memory _memory;

  /* Largest ACK so far */
  int _largest_ack;
	

public:
  LerpSender( PointGrid & grid );
  void reset( const double & tickno ); /* start new flow */
  void packets_received( const std::vector< Packet > & packets );
  double next_event_time( const double & tickno ) const;
  void update_actions( const Memory memory );

  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );
  LerpSender & operator=( const LerpSender & ) { assert( false ); return *this; }

	void add_inner_point ( const Point point, PointGrid & grid );
	ActionTuple interpolate ( double s, double r, double t );
	ActionTuple interpolate ( SignalTuple t );

};
#endif
