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

#include "boost/multi_array.hpp"
using namespace std;

#define MAX_SEND_EWMA 300
#define MAX_REC_EWMA 300
#define MAX_RTT_RATIO 100

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

#define DEFAULT_INCR 1
#define DEFAULT_MULT 1
#define DEFAULT_SEND 3
struct InterpInfo {
  InterpInfo() :  window_incr_vals( 8 ), window_mult_vals( 8 ), intersend_vals( 8) {}
	double min_send_ewma;
	double max_send_ewma;
	double min_rec_ewma;
	double max_rec_ewma;
	double min_rtt_ratio;
	double max_rtt_ratio;
	array<int, 3> grid_sizes;
	std::vector< double > window_incr_vals;
	std::vector< double > window_mult_vals;
	std::vector< double > intersend_vals;
};

string _stuple_str( SignalTuple t );
string _atuple_str( ActionTuple t );
string _point_str( Point p );
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
	SignalActionMap _points; // map of signal to action
  vector< vector< double > > _signals; // list of values for each signal
	PointGrid( bool track = false );
	PointGrid( PointGrid & other, bool track );

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
  Memory _memory;
	std::vector< InterpInfo > _interp_info_list; // everywhere the grid is modified, remake the interp info list
  unsigned int _packets_sent, _packets_received;
  double _last_send_time;
  /* _the_window is the congestion window */
  double _the_window;
  double _intersend_time;
  unsigned int _flow_id;
	std::vector< InterpInfo > get_interp_info( void );
  /* Largest ACK so far */
  int _largest_ack;
	string interp_str();
	double interpolate_action( vector< double > actions, double x0_min_factor, double x1_min_factor, double x2_min_factor );
  void print_interp_info( InterpInfo info );

public:
  LerpSender( PointGrid & grid );
  void reset( const double & tickno ); /* start new flow */
  void packets_received( const std::vector< Packet > & packets );
  double next_event_time( const double & tickno ) const;
  void update_actions( const Memory memory );
	void update_interp_info( void );
  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );
  LerpSender & operator=( const LerpSender & ) { assert( false ); return *this; }

	void add_inner_point ( const Point point, PointGrid & grid );
	ActionTuple interpolate ( double s, double r, double t );
	ActionTuple interpolate_linterp( double s, double r, double t, InterpInfo info);
	ActionTuple interpolate ( SignalTuple t );

};
#endif
