#include <limits>
#include <algorithm>

#include "lerpsender.hh"

using namespace std;

static constexpr double INITIAL_WINDOW = 100; /* INITIAL WINDOW OF 1 */

LerpSender::LerpSender()
  :  _packets_sent( 0 ),
     _packets_received( 0 ),
     _last_send_time( 0 ),
     _the_window( INITIAL_WINDOW ),
     _intersend_time( 0 ),
     _flow_id( 0 ),
     _memory(),
     _largest_ack( -1 ),
		 _known_signals( 8 ),
		 _known_points( {
					{ make_tuple(0,0,0) , make_tuple(0,0,0) }
		} )
{
	for ( auto it = _known_points.begin(); it != _known_points.end(); ++it ) {
		_known_signals.push_back(it->first);
	}
	sort(_known_signals.begin(), _known_signals.end(), CompareSignals());
}

void LerpSender::packets_received( const vector< Packet > & packets ) {
  _packets_received += packets.size();
  _largest_ack = max( packets.at( packets.size() - 1 ).seq_num, _largest_ack );
  _the_window = max( INITIAL_WINDOW, _the_window );
  _memory.packets_received( packets, _flow_id, _largest_ack );

  update_actions( _memory );
  //printf("Updated window to be %f and intersend to be %f\n", _the_window, _intersend_time );
}

void LerpSender::reset( const double & )
{
  _the_window = INITIAL_WINDOW;
  _last_send_time = 0;
  _flow_id++;
  _intersend_time = 0;
  _largest_ack = _packets_sent - 1;
   _memory.reset();
  assert( _flow_id != 0 );
}

double LerpSender::next_event_time( const double & tickno ) const
{
  if ( _packets_sent < _largest_ack + 1 + _the_window ) {
    if ( _last_send_time + _intersend_time <= tickno ) {
      return tickno;
    } else {
      return _last_send_time + _intersend_time;
    }
    return tickno;
  } else {
    return std::numeric_limits<double>::max();
  }
}

void LerpSender::update_actions( const Memory memory )
{
  double obs_send_ewma = (double)(memory.field( 0 ) );
  double obs_rec_ewma = (double)(memory.field( 1 ) );
  double obs_rtt_ratio = (double)(memory.field( 2 ) );
  
	double x0min, x0max, x1min, x1max, x2min, x2max;

	int i = 0;
	while (SEND_EWMA(_known_signals[i]) < obs_send_ewma) { i++; }
	x0min = SEND_EWMA(_known_signals[i-1]);
	x0max = SEND_EWMA(_known_signals[i]);
	while (REC_EWMA(_known_signals[i]) < obs_rec_ewma) { i++; }
	x1min = REC_EWMA(_known_signals[i-1]);
	x1max = REC_EWMA(_known_signals[i]);
	while (RTT_RATIO(_known_signals[i]) < obs_rtt_ratio) { i++; }
	x2min = RTT_RATIO(_known_signals[i-1]);
	x2max = RTT_RATIO(_known_signals[i]);

  double x0_max_t = (x0max - obs_send_ewma),
         x0_t_min = (obs_send_ewma - x0min),
         x1_max_t = (x1max - obs_rec_ewma),
         x1_t_min = (obs_rec_ewma - x1min),
         x2_max_t = (x2max - obs_rtt_ratio),
         x2_t_min = (obs_rtt_ratio - x2min);

	vector<ActionTuple> actions = {
		_known_points[make_tuple(x0min, x1min, x2min)],
		_known_points[make_tuple(x0min, x1min, x2max)],
		_known_points[make_tuple(x0min, x1max, x2min)],
		_known_points[make_tuple(x0min, x1max, x2max)],
		_known_points[make_tuple(x0max, x1min, x2min)],
		_known_points[make_tuple(x0max, x1min, x2max)],
		_known_points[make_tuple(x0max, x1max, x2min)],
		_known_points[make_tuple(x0max, x1max, x2max)],
	};

  double window_increment =
        ((((x0_max_t * ((x1_max_t * x2_max_t * CWND_INC(actions[0])) + 
                       (x1_max_t * x2_t_min * CWND_INC(actions[1])) +
                       (x1_t_min * x2_max_t * CWND_INC(actions[2])) + 
                       (x1_t_min * x2_t_min * CWND_INC(actions[3])))) +
           (x0_t_min * (x1_max_t * x2_max_t * CWND_INC(actions[4])) + 
                       (x1_max_t * x2_t_min * CWND_INC(actions[5])) +
                       (x1_t_min * x2_max_t * CWND_INC(actions[6])) + 
                       (x1_t_min * x2_t_min * CWND_INC(actions[7])))))
           / 
           ((x0max - x0min) * (x1max - x1min) * (x2max - x2min))
         );

  double window_multiplier = 
        ((((x0_max_t * ((x1_max_t * x2_max_t * CWND_MULT(actions[0])) + 
                       (x1_max_t * x2_t_min * CWND_MULT(actions[1])) +
                       (x1_t_min * x2_max_t * CWND_MULT(actions[2])) + 
                       (x1_t_min * x2_t_min * CWND_MULT(actions[3])))) +
           (x0_t_min * (x1_max_t * x2_max_t * CWND_MULT(actions[4])) + 
                       (x1_max_t * x2_t_min * CWND_MULT(actions[5])) +
                       (x1_t_min * x2_max_t * CWND_MULT(actions[6])) + 
                       (x1_t_min * x2_t_min * CWND_MULT(actions[7])))))
           / 
           ((x0max - x0min) * (x1max - x1min) * (x2max - x2min))
         );

  double intersend = 
        ((((x0_max_t * ((x1_max_t * x2_max_t * MIN_SEND(actions[0])) + 
                       (x1_max_t * x2_t_min * MIN_SEND(actions[1])) +
                       (x1_t_min * x2_max_t * MIN_SEND(actions[2])) + 
                       (x1_t_min * x2_t_min * MIN_SEND(actions[3])))) +
           (x0_t_min * (x1_max_t * x2_max_t * MIN_SEND(actions[4])) + 
                       (x1_max_t * x2_t_min * MIN_SEND(actions[5])) +
                       (x1_t_min * x2_max_t * MIN_SEND(actions[6])) + 
                       (x1_t_min * x2_t_min * MIN_SEND(actions[7])))))
           / 
           ((x0max - x0min) * (x1max - x1min) * (x2max - x2min))
         );

  _the_window = min( max( 0, int( _the_window * window_multiplier + window_increment ) ), 1000000 );
  _intersend_time = intersend;
}

void LerpSender::add_points( const vector<Point> points ) {
	for (size_t i=0; i<points.size(); i++) {
		Point p = points[i];
		_known_signals.push_back(p.first);
		_known_points.insert(p);
	}
	sort(_known_signals.begin(), _known_signals.end(), CompareSignals());
}

void LerpSender::set_point( const Point point ) {
	_known_points[point.first] = point.second;
}
