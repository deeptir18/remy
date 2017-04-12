#include <limits>
#include <iomanip>
#include <algorithm>
#include <boost/array.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp> 
#include <stdlib.h>
#include "linterp.h"
#include "p2.h"
#include <stdio.h>

#include "lerpsender.hh"
using namespace std;

static constexpr double INITIAL_WINDOW = 100; /* INITIAL WINDOW OF 1 */

/*****************************************************************************/
#define DEFAULT_INCR 1
#define DEFAULT_MULT 1
#define DEFAULT_SEND 3
#define UPPER_QUANTILE 0.95
	PointGrid::PointGrid( bool track )
	:	 _track ( track ),
		 _acc ( NUM_SIGNALS, p2_t( UPPER_QUANTILE ) ),
     _debug( false ),
	   _points( ),
     _signals( NUM_SIGNALS )
{
	// Default values: set all 8 corners of the box to the default action
	for ( int i = 0; i <= MAX_SEND_EWMA; i += MAX_SEND_EWMA) {
		for ( int j = 0; j <= MAX_REC_EWMA; j += MAX_REC_EWMA) {
			for ( int k = 0; k <= MAX_RTT_RATIO; k += MAX_RTT_RATIO) {
				_points[make_tuple(i,j,k)] = 
					make_tuple(DEFAULT_INCR,DEFAULT_MULT,DEFAULT_SEND);
			}
		}
	}
	_signals[0] = { 0, MAX_SEND_EWMA };
	_signals[1] = { 0, MAX_REC_EWMA };
	_signals[2] = { 0, MAX_RTT_RATIO };
}

/*
void PointGrid::Test( ) {
		boost::array<double,5> probs = {0.001,0.01,0.1,.99,0.999};
		acc_quantile_t accc( acc_prob_param = probs);

		for (double i=0.0; i<10000.; ++i) {
			accc(i);
		}

		for (std::size_t i=0; i < probs.size(); ++i)
		{
			    printf("%f: %f\n", probs[i],boost::accumulators::extended_p_square(accc)[i]);
		}
		return;
}
*/

// Copy constructor
PointGrid::PointGrid( PointGrid & other, bool track )
	:	_track( track ),
		_acc( NUM_SIGNALS, p2_t( UPPER_QUANTILE ) ),
		// _acc( NUM_SIGNALS ),
    _debug( false ),
	  _points( other._points ),
	  _signals( other._signals )
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
	return _points.size();
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
	// printf("%f %f %f\n", s, r ,t);
	if (_track) {
		/*
		_acc[0]( s );
		_acc[1]( r );
		_acc[2]( t );
		*/
		_acc[0].add( s );
		_acc[1].add( r );
		_acc[2].add( t );
	}
}

SignalTuple PointGrid::get_median_signal() {
	assert(_track);
	return make_tuple(
			/*
			boost::accumulators::median( _acc[0] ),
			boost::accumulators::median( _acc[1] ),
			boost::accumulators::median( _acc[2] )
			*/
			_acc[0].result( ),
			_acc[1].result( ),
			_acc[2].result( )
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
{
	// now create the interp info list - only update it when adding an inner point
	_interp_info_list = get_interp_info();
}

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
  if ( _grid._debug ) {
    printf("Obs values: [%f, %f, %f]\n", obs_send_ewma, obs_rec_ewma, obs_rtt_ratio );
  }
	_grid.track( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio );

	ActionTuple a = interpolate( make_tuple( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio ) );

  _the_window = min( max( 0.0,  _the_window * CWND_MULT(a) + CWND_INC(a)  ), 1000000.0 );
  _intersend_time = MIN_SEND(a);
}

void LerpSender::add_inner_point( const Point point, PointGrid & grid ) {
	// Add inner point
	grid._points[point.first] = point.second;

	// Find enclosing box
	double x0min, x0max, x1min, x1max, x2min, x2max;
	double obs_send_ewma, obs_rec_ewma, obs_rtt_ratio;

	tie (obs_send_ewma, obs_rec_ewma, obs_rtt_ratio) = point.first;

  int i = 0;
  while ( obs_send_ewma > _grid._signals[0][i] ) { i ++; }
  x0min = _grid._signals[0][ max( i-1, 0 ) ];
  x0max = _grid._signals[0][ min( i, int(_grid._signals[0].size() -  1) ) ];

  i = 0;
  while ( obs_rec_ewma > _grid._signals[1][i] ) { i ++; }
  x1min = _grid._signals[1][ max( i-1, 0 ) ];
  x1max = _grid._signals[1][ min( i, int(_grid._signals[1].size() - 1) ) ];

  i = 0;
  while ( obs_rtt_ratio > _grid._signals[2][i] ) { i ++; }
  x2min = _grid._signals[2][ max( i-1, 0 ) ];
  x2max = _grid._signals[2][ min( i, int(_grid._signals[2].size() - 1) ) ];

	// Add all side points
	InterpInfo interp;
	for ( InterpInfo info: _interp_info_list ) {
		if ( ( x0min == info.min_send_ewma ) && ( x0max == info.max_send_ewma ) && ( x1min == info.min_rec_ewma ) && ( x1max == info.max_rec_ewma ) &&  ( x2min == info.min_rtt_ratio ) && ( x2max == info.max_rtt_ratio ) ) {
			interp = info;
    }
	}
	for (double &x : vector<double>{x0min,SEND_EWMA(point.first),x0max}) {
		for (double &y : vector<double>{x1min,REC_EWMA(point.first),x1max}) {
			for (double &z : vector<double>{x2min,RTT_RATIO(point.first),x2max}) {
				if ( ( x == SEND_EWMA(point.first ) ) || 
						 ( y == REC_EWMA(point.first ) )  || 
						 ( z == RTT_RATIO(point.first ) ) ) {
					SignalTuple tmp = make_tuple(x,y,z);
					grid._points[tmp] = interpolate_linterp(x,y,z,interp);
					if (find( grid._signals[0].begin(), grid._signals[0].end(), x ) == grid._signals[0].end()) {
						grid._signals[0].push_back( x );
					}
					if (find( grid._signals[1].begin(), grid._signals[1].end(), y ) == grid._signals[1].end()) {
						grid._signals[1].push_back( y );
					}
					if (find( grid._signals[2].begin(), grid._signals[2].end(), z ) == grid._signals[2].end()) {
						grid._signals[2].push_back( z );
					}
				}
			}
		}
	}
  // modify the mapping for the median itself
  grid._points[point.first] = point.second;
	for (int i=0; i<3; i++) {
		sort(grid._signals[i].begin(), grid._signals[i].end());
	}
	// update the interp info the reflect the modified grid
	_interp_info_list = get_interp_info();

}

vector< InterpInfo > LerpSender::get_interp_info( void )
{
	vector< InterpInfo > vec;

  // create every possible pair of sends, recs, and ratios -> 0,1 1,2 ...
  vector< pair< double, double > > sends;
  vector< pair< double, double > > recs;
  vector< pair< double, double > > ratios;
  int num_signals = int( _grid._signals[0].size() );
  for ( int i = 0; i < num_signals; i++ ) {
    if ( i != num_signals - 1 ) {
      sends.push_back( make_pair( _grid._signals[0][i], _grid._signals[0][i+1]));
      recs.push_back( make_pair( _grid._signals[1][i], _grid._signals[1][i+1]));
      ratios.push_back( make_pair( _grid._signals[2][i], _grid._signals[2][i+1]));
    }
  }
				for ( pair< double, double > send_vals: sends ) {
					for ( pair< double, double > rec_vals: recs ) {
						for ( pair < double, double > ratio_vals: ratios ) {
              double send_min = send_vals.first;
              double send_max = send_vals.second;
              double rec_min = rec_vals.first;
              double rec_max = rec_vals.second;
              double ratio_min = ratio_vals.first;
              double ratio_max = ratio_vals.second;
							InterpInfo info;
							info.min_send_ewma = send_min;
							info.max_send_ewma = send_max;
							info.min_rec_ewma = rec_min;
							info.max_rec_ewma = rec_max;
							info.min_rtt_ratio = ratio_min;
							info.max_rtt_ratio = ratio_max;

							info.grid_sizes[0] = 2;
							info.grid_sizes[1] = 2;
							info.grid_sizes[2] = 2;

							for (int i = 0; i < 8; i ++ ) {
								double cur_send = info.min_send_ewma;
								double cur_rec = info.min_rec_ewma;
								double cur_ratio = info.min_rtt_ratio;
							  if (i >= 4)  {
									cur_send = info.max_send_ewma;
								}
								if ( (i == 2) || (i== 3) || (i == 6) || (i == 7) ) {
									cur_rec = info.max_rec_ewma;
								}
								if ( (i == 1) || (i == 3) || (i == 5) || (i == 7) ) {
									cur_ratio = info.max_rtt_ratio;
								}
								ActionTuple a = _grid._points[ make_tuple( cur_send, cur_rec, cur_ratio ) ];
								info.window_incr_vals[i] = (CWND_INC( a ));
								info.window_mult_vals[i] = ( CWND_MULT( a ));
								info.intersend_vals[i] = ( MIN_SEND( a ));
							}

							vec.push_back( info );
						}
					}
				}
	return vec;
}

double LerpSender::interpolate_action( vector< double > actions, double x0_min_factor, double x1_min_factor, double x2_min_factor )
{
	double x0_max_factor = 1 - x0_min_factor;
	double x1_max_factor = 1 - x1_min_factor;
	double x2_max_factor = 1 - x2_min_factor;

	double c00 = ( actions[0] )*x0_min_factor + ( actions[4] )*x0_max_factor;
	double c01 = ( actions[1] )*x0_min_factor + ( actions[5] )*x0_max_factor;
	double c10 = ( actions[2] )*x0_min_factor + ( actions[6] )*x0_max_factor;
	double c11 = ( actions[3] )*x0_min_factor + ( actions[7] )*x0_max_factor;

	double c0 = c00*x1_min_factor + c10*x1_max_factor;
	double c1 = c01*x1_min_factor + c11*x1_max_factor;

	double c = c0*( x2_min_factor ) + c1*( x2_max_factor );
	return c;

}
ActionTuple LerpSender::interpolate( SignalTuple t ) {
	double obs_send_ewma = SEND_EWMA(t);
	double obs_rec_ewma = REC_EWMA(t);
	double obs_rtt_ratio = RTT_RATIO(t);

	double x0min, x0max, x1min, x1max, x2min, x2max;
  int i = 0;

  while ( obs_send_ewma >= _grid._signals[0][i] ) {
    i ++;
    if ( i == int( _grid._signals[0].size() - 1 ) ) { break; }
  }
  x0min = _grid._signals[0][ max( i-1, 0 ) ];
  x0max = _grid._signals[0][ min( i, int(_grid._signals[0].size() -  1) ) ];

  i = 0;
  while ( obs_rec_ewma >= _grid._signals[1][i] ) {
    i ++;
    if ( i == int( _grid._signals[0].size() - 1 ) ) { break; }
  }
  x1min = _grid._signals[1][ max( i-1, 0 ) ];
  x1max = _grid._signals[1][ min( i, int(_grid._signals[1].size() - 1) ) ];

  i = 0;
  while ( obs_rtt_ratio >= _grid._signals[2][i] ) {
    i ++;
    if ( i == int( _grid._signals[0].size() - 1 ) ) { break; }

  }
  x2min = _grid._signals[2][ max( i-1, 0 ) ];
  x2max = _grid._signals[2][ min( i, int(_grid._signals[2].size() - 1) ) ];
	InterpInfo interp;
	for ( InterpInfo info: _interp_info_list ) {
		if ( ( x0min == info.min_send_ewma ) && ( x0max == info.max_send_ewma ) && ( x1min == info.min_rec_ewma ) && ( x1max == info.max_rec_ewma ) &&  ( x2min == info.min_rtt_ratio ) && ( x2max == info.max_rtt_ratio ) ) {
			interp = info;
    }
	}
	return interpolate_linterp( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio, interp );
}

void LerpSender::update_interp_info( void )
{
	_interp_info_list = get_interp_info();
}
// return an evenly spaced 1-d grid of doubles.
vector<double> linspace(double first, double last, int len) {
  vector<double> result(len);
  double step = (last-first) / (len - 1);
  for (int i=0; i<len; i++) { result[i] = first + i*step; }
  return result;
}

void
LerpSender::print_interp_info( InterpInfo info ) {
  // prints an interp info
  printf("----\n");
  printf("Min and max for send_ewma, rec_ewma, rtt_ratio are [%f, %f], [%f, %f], [%f, %f]\n", info.min_send_ewma, info.max_send_ewma, info.min_rec_ewma, info.max_rec_ewma, info.min_rtt_ratio, info.max_rtt_ratio);
  printf("Window incr vals: [%f, %f, %f, %f, %f, %f, %f, %f]\n", info.window_incr_vals[0], info.window_incr_vals[1], info.window_incr_vals[2], info.window_incr_vals[3], info.window_incr_vals[4], info.window_incr_vals[5], info.window_incr_vals[6], info.window_incr_vals[7]);
  printf("Window mult vals: [%f, %f, %f, %f, %f, %f, %f, %f]\n", info.window_mult_vals[0], info.window_mult_vals[1], info.window_mult_vals[2], info.window_mult_vals[3], info.window_mult_vals[4], info.window_mult_vals[5], info.window_mult_vals[6], info.window_mult_vals[7]);
  printf("Intersend vals: [%f, %f, %f, %f, %f, %f, %f, %f]\n", info.intersend_vals[0], info.intersend_vals[1], info.intersend_vals[2], info.intersend_vals[3], info.intersend_vals[4], info.intersend_vals[5], info.intersend_vals[6], info.intersend_vals[7]);
  printf("Grid sizes: [%d, %d, %d]\n", info.grid_sizes[0], info.grid_sizes[1], info.grid_sizes[2]);
  printf("----\n");
}

ActionTuple LerpSender::interpolate_linterp( double s, double r, double t, InterpInfo info) {
  int length = 2;
  vector<double> grid1 = linspace(info.min_send_ewma, info.max_send_ewma, length);
  vector<double> grid2 = linspace(info.min_rec_ewma, info.max_rec_ewma, length);
  vector<double> grid3 = linspace(info.min_rtt_ratio, info.max_rtt_ratio, length);
  vector< vector<double>::iterator > grid_iter_list;

  grid_iter_list.push_back(grid1.begin());
  grid_iter_list.push_back(grid2.begin());
	grid_iter_list.push_back(grid3.begin());


  //print_interp_info( info );

	InterpMultilinear<3, double > interp_ML_incr( grid_iter_list.begin(), info.grid_sizes.begin(), info.window_incr_vals.data(), info.window_incr_vals.data() + 8);
	InterpMultilinear<3, double > interp_ML_mult( grid_iter_list.begin(), info.grid_sizes.begin(), info.window_mult_vals.data(), info.window_mult_vals.data() + 8);
	InterpMultilinear<3, double > interp_ML_send( grid_iter_list.begin(), info.grid_sizes.begin(), info.intersend_vals.data(), info.intersend_vals.data() + 8);
  array< double, 3> args = {s,r,t};

	double incr = interp_ML_incr.interp( args.begin() );
	double mult = interp_ML_mult.interp( args.begin() );
	double send = interp_ML_send.interp( args.begin() );

  return make_tuple( incr, mult, send );
}

ActionTuple LerpSender::interpolate( double obs_send_ewma, double obs_rec_ewma, double obs_rtt_ratio ) {

	double x0min, x0max, x1min, x1max, x2min, x2max;

  int i = 0;
  while ( obs_send_ewma > _grid._signals[0][i] ) { i ++; }
  x0min = _grid._signals[0][ max( i-1, 0 ) ];
  x0max = _grid._signals[0][ min( i, int(_grid._signals[0].size() -  1) ) ];

  i = 0;
  while ( obs_rec_ewma > _grid._signals[1][i] ) { i ++; }
  x1min = _grid._signals[1][ max( i-1, 0 ) ];
  x1max = _grid._signals[1][ min( i, int(_grid._signals[1].size() - 1) ) ];

  i = 0;
  while ( obs_rtt_ratio > _grid._signals[2][i] ) { i ++; }
  x2min = _grid._signals[2][ max( i-1, 0 ) ];
  x2max = _grid._signals[2][ min( i, int(_grid._signals[2].size() - 1) ) ];

	/*
	if (rand() % 1000000 == 5) {
		printf("signal: %f %f %f || ", obs_send_ewma, obs_rec_ewma, obs_rtt_ratio );
		printf("x0: %f,%f x1: %f,%f x2: %f,%f\n", x0min,x0max, x1min,x1max, x2min,x2max );
	}
	*/

	double x0d = ( obs_send_ewma - x0min )/( x0max - obs_send_ewma );
	double x1d = ( obs_rec_ewma - x1min )/ ( x1max - obs_rec_ewma );
	double x2d = ( obs_rtt_ratio - x2min )/ (x2max - obs_rtt_ratio);
	double x0_min_factor = ( obs_send_ewma == x0min ) ? 1 : ( obs_send_ewma == x0max ) ? 0 : ( 1 - x0d );
	double x1_min_factor = ( obs_rec_ewma == x1min ) ? 1 : ( obs_rec_ewma == x1max ) ?  0 : ( 1 - x1d );
	double x2_min_factor = ( obs_rtt_ratio == x2min ) ? 1 : ( obs_rtt_ratio == x2max ) ?  0 : ( 1 - x2d );

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
	vector< double > increments;
	vector< double > multiples;
	vector< double > intersends;

	for ( int i = 0; i < 8; i ++ ) {
		increments.emplace_back( CWND_INC( actions[i] ) );
		multiples.emplace_back( CWND_MULT( actions[i]  ));
		intersends.emplace_back( MIN_SEND( actions[i] ));
	}

	double inc = interpolate_action( increments, x0_min_factor, x1_min_factor, x2_min_factor );
	double mult = interpolate_action( multiples, x0_min_factor, x1_min_factor, x2_min_factor );
	double send = interpolate_action( intersends, x0_min_factor, x1_min_factor, x2_min_factor );
	return make_tuple( inc, mult, send);

}

/*****************************************************************************/
