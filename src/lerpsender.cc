#include <limits>
#include <iomanip>
#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>

#include "lerpsender.hh"
using namespace std;

static constexpr double INITIAL_WINDOW = 100; /* INITIAL WINDOW OF 1 */

/*****************************************************************************/
#define DEFAULT_INCR 1
#define DEFAULT_MULT 1
#define DEFAULT_SEND 3
	PointGrid::PointGrid( bool track )
	:	 _track ( track ),
		 _acc ( NUM_SIGNALS ), // if true, accumulates all signals here
	   _signals( ( (int) pow( 2, NUM_SIGNALS ) ) ),
	   _points( ),
     _signal_values( )
{
  vector< double > send_ewma_vals;
  vector< double > rec_ewma_vals;
  vector< double > rtt_ratio_vals;
  _signal_values.emplace_back( send_ewma_vals );
  _signal_values.emplace_back( rec_ewma_vals );
  _signal_values.emplace_back( rtt_ratio_vals );
	// Default values: set all 8 corners of the box to the default action
	for ( int i = 0; i <= MAX_SEND_EWMA; i += MAX_SEND_EWMA) {
		for ( int j = 0; j <= MAX_REC_EWMA; j += MAX_REC_EWMA) {
			for ( int k = 0; k <= MAX_RTT_RATIO; k += MAX_RTT_RATIO) {
				_points[make_tuple(i,j,k)] = 
					make_tuple(DEFAULT_INCR,DEFAULT_MULT,DEFAULT_SEND);
			}
		}
	}
	// Keep a sorted vector of the 3D coordinates to assist in finding the
	// bounding box when interpolating
	int i = 0;
	for ( auto it = _points.begin(); it != _points.end(); ++it,i++ ) {
    SignalTuple signal = it->first;
		_signals[i] = (signal);
    for ( int j = 0; j < 3; j ++ ) {
      vector< double > vals = _signal_values.at( j );
      double val =  ( i == 0 ) ? SEND_EWMA( signal ) : ( i == 1 ) ? REC_EWMA( signal ) : RTT_RATIO( signal );
      if ( find( vals.begin(), vals.end(), val ) == vals.end() ) {
        vals.emplace_back( val );
      }
      sort( vals.begin(), vals.end() );
      _signal_values.at( j ) = vals;
    }
	}
	sort(_signals.begin(), _signals.end(), CompareSignals());
}

// Copy constructor
PointGrid::PointGrid( PointGrid & other, bool track )
	:	_track( track ),
		_acc( other._acc ),
	  _signals( other._signals ),
	  _points( other._points ),
    _signal_values( other._signal_values )
{}

// For iterating through all points in the grid
SignalActionMap::iterator PointGrid::begin() {
	return _points.begin();
}
SignalActionMap::iterator PointGrid::end() {
	return _points.end();
}

// Number of points in the grid
int PointGrid::size() {
	return _signals.size();
}

string _stuple_str( SignalTuple t ) {
	ostringstream stream;
	stream << "S[send=" 
		     << SEND_EWMA(t) << ",rec=" 
		     << REC_EWMA(t) << ",ratio=" 
				 << RTT_RATIO(t)
				 << "]";
	return stream.str();
}

string _atuple_str( ActionTuple t ) {
	ostringstream stream;
	stream << "A[inc=" 
		     << CWND_INC(t) << ",mult=" 
		     << CWND_MULT(t) << ",intr=" 
				 << MIN_SEND(t)
				 << "]";
	return stream.str();
}

string _point_str( Point p ) {
	return _stuple_str(p.first) + " -> " + _atuple_str(p.second);
}

string PointGrid::str() {
	string ret;
	for ( auto it = begin(); it != end(); ++it ) {
		ret += _stuple_str(it->first) + string( " -> " ) + _atuple_str(it->second) + string( "\n" ); 
	}
	return ret;
}

void PointGrid::track ( double s, double r, double t ) {
	if (_track) {
		_acc[0]( s );
		_acc[1]( r );
		_acc[2]( t );
	}
}

SignalTuple PointGrid::get_median_signal() {
	assert(_track);
	return make_tuple( 
			boost::accumulators::median( _acc[0] ), 
			boost::accumulators::median( _acc[1] ),
			boost::accumulators::median( _acc[2] )
	);
}
/*****************************************************************************/

/*****************************************************************************/
LerpSender::LerpSender(PointGrid & grid)
  :  _grid( grid ),
     _memory(),
		 _packets_sent( 0 ),
     _packets_received( 0 ),
     _last_send_time( 0 ),
     _the_window( INITIAL_WINDOW ),
     _intersend_time( 0 ),
     _flow_id( 0 ),
     _largest_ack( -1 )
{}

void LerpSender::reset( const double & )
{
   _memory.reset();
  _last_send_time = 0;
  _the_window = 0;
  _intersend_time = 0;
  _flow_id++;
  _largest_ack = _packets_sent - 1;
  assert( _flow_id != 0 );

  update_actions( _memory );
}

void LerpSender::packets_received( const vector< Packet > & packets ) {
  _packets_received += packets.size();
  _memory.packets_received( packets, _flow_id, _largest_ack );
  _largest_ack = max( packets.at( packets.size() - 1 ).seq_num, _largest_ack );

  update_actions( _memory );
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
	_grid.track( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio );

	ActionTuple a = interpolate( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio );

  _the_window = min( max( 0.0,  _the_window * CWND_MULT(a) + CWND_INC(a)  ), 1000000.0 );
  _intersend_time = MIN_SEND(a);
}

void LerpSender::add_inner_point( const Point point, PointGrid & grid ) {
	// Add inner point
	grid._points[point.first] = point.second;

	// Add all side points
	for (double &x : vector<double>{0,SEND_EWMA(point.first),MAX_SEND_EWMA}) {
		for (double &y : vector<double>{0,REC_EWMA(point.first),MAX_REC_EWMA}) {
			for (double &z : vector<double>{0,RTT_RATIO(point.first),MAX_RTT_RATIO}) {
				if ( ( x == SEND_EWMA(point.first ) ) || 
						 ( y == REC_EWMA(point.first ) )  || 
						 ( z == RTT_RATIO(point.first ) ) ) {
					SignalTuple tmp = make_tuple(x,y,z);
					grid._signals.push_back(tmp);
					grid._points[tmp] = interpolate(tmp);
          vector< double > send_ewma_vals = grid._signal_values.at( 0 );
          if ( find( send_ewma_vals.begin(), send_ewma_vals.end(), x ) == send_ewma_vals.end() ) {
            send_ewma_vals.emplace_back( x );
            grid._signal_values.at( 0 ) = send_ewma_vals;
          }
          vector< double > rec_ewma_vals = grid._signal_values.at( 1 );
          if ( find( rec_ewma_vals.begin(), rec_ewma_vals.end(), x ) == rec_ewma_vals.end() ) {
            rec_ewma_vals.emplace_back( x );
            grid._signal_values.at( 1 ) = rec_ewma_vals;
          }
          vector< double > rtt_ratio_vals = grid._signal_values.at( 2 );
          if ( find( rtt_ratio_vals.begin(), rtt_ratio_vals.end(), x ) == rtt_ratio_vals.end() ) {
            rtt_ratio_vals.emplace_back( x );
            grid._signal_values.at( 2 ) = rtt_ratio_vals;
           }
          }
				}
			}
		}
	// Resort signals
	sort(grid._signals.begin(), grid._signals.end(), CompareSignals());
  for ( int i = 0; i < 3; i ++ ) {
    vector< double > vals = grid._signal_values.at( i );
    sort( vals.begin(), vals.end() );
    grid._signal_values.at( i ) = vals;
  }
}

ActionTuple LerpSender::interpolate( SignalTuple t ) {
	return interpolate(SEND_EWMA(t),REC_EWMA(t),RTT_RATIO(t));
}
ActionTuple LerpSender::interpolate( double obs_send_ewma, double obs_rec_ewma, double obs_rtt_ratio ) {
	double x0min, x0max, x1min, x1max, x2min, x2max;

  vector< double > send_ewma_vals;
  vector< double > rec_ewma_vals;
  vector< double > rtt_ratio_vals;

  send_ewma_vals = _grid._signal_values.at( 0 );
  rec_ewma_vals = _grid._signal_values.at( 1 );
  rtt_ratio_vals =_grid._signal_values.at( 2 );

  int i = 0;
  while ( obs_send_ewma < send_ewma_vals.at( i ) ) { i ++; }
  x0min = send_ewma_vals.at( max( i, 0 ) );
  x0max = send_ewma_vals.at( min( i+1, int(send_ewma_vals.size() -  1) ) );

  i = 0;
  while ( obs_rec_ewma < rec_ewma_vals.at( i ) ) { i ++; }
  x1min = rec_ewma_vals.at( max( i, 0 ) );
  x1max = rec_ewma_vals.at( min( i+1, int(rec_ewma_vals.size() - 1) ) );

  i = 0;
  while ( obs_rtt_ratio < rtt_ratio_vals.at( i ) ) { i ++; }
  x2min = rtt_ratio_vals.at( max( i, 0 ) );
  x2max = rtt_ratio_vals.at( min( i+1, int(rtt_ratio_vals.size() - 1) ) );

  double x0_max_t = (x0max - obs_send_ewma),
         x0_t_min = (obs_send_ewma - x0min),
         x1_max_t = (x1max - obs_rec_ewma),
         x1_t_min = (obs_rec_ewma - x1min),
         x2_max_t = (x2max - obs_rtt_ratio),
         x2_t_min = (obs_rtt_ratio - x2min);

  vector<ActionTuple> actions = {
		_grid._points[make_tuple(x0min, x1min, x2min)],
		_grid._points[make_tuple(x0min, x1min, x2max)],
		_grid._points[make_tuple(x0min, x1max, x2min)],
		_grid._points[make_tuple(x0min, x1max, x2max)],
		_grid._points[make_tuple(x0max, x1min, x2min)],
		_grid._points[make_tuple(x0max, x1min, x2max)],
		_grid._points[make_tuple(x0max, x1max, x2min)],
		_grid._points[make_tuple(x0max, x1max, x2max)],
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

	return make_tuple(window_increment, window_multiplier, intersend);
}

/*****************************************************************************/
