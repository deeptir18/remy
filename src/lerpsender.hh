#ifndef LERPSENDER_HH
#define LERPSENDER_HH

#include <cassert>
#include <vector>
#include <string>
#include "memory.hh"
#include "packet.hh"

#include <unordered_map>

using namespace std;

#define MAX_SEND_EWMA 163840
#define MAX_REC_EWMA 163840
#define MAX_RTT_RATIO 163840

typedef tuple<double,double,double> SignalTuple;
#define SEND_EWMA(s) get <0>(s)
#define REC_EWMA(s)  get <1>(s)
#define RTT_RATIO(s) get <2>(s)
typedef tuple<double,double,double> ActionTuple;
#define CWND_INC(a)  get <0>(a)
#define CWND_MULT(a) get <1>(a)
#define MIN_SEND(a)  get <2>(a)
typedef pair<SignalTuple,ActionTuple> Point;
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
public:
	vector<SignalTuple> _signals;
	SignalActionMap _points;
	PointGrid();
	void add_points ( const vector<Point> points );
	void set_point ( const Point point );
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
  void update_actions( const Memory memory );
  void packets_received( const std::vector< Packet > & packets );
  void reset( const double & tickno ); /* start new flow */

  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );

  LerpSender & operator=( const LerpSender & ) { assert( false ); return *this; }

  double next_event_time( const double & tickno ) const;


};

	//double _known_points[2][2][2][3] = {
		//{{{100 , 0.2  , 10},     // send_low rec_low rtt_low
			//{50  , 0.01 , 1000}},  // send_low rec_low rtt_high
		 //{{20  , 0.5  , 100},    // send_low rec_high rtt_low
			//{10  , 0.4  , 2000}}}, // send_low rec_high rtt_high
		//{{{70  , 0.01 , 200},    // send_high rec_low rtt_low
			//{100 , 0.05 , 3000}},  // send_high rec_low rtt_high
		 //{{50  , 0.1  , 500},    // send_high rec_high rtt_low
			//{60  , 0.05 , 5000}}}  // send_high rec_high rtt_high
	//};
#endif
