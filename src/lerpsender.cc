#include <limits>
#include <iomanip>
#include <algorithm>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <stdlib.h>
#include "linterp.h"

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

// Copy constructor
PointGrid::PointGrid( PointGrid & other, bool track )
	:	_track( track ),
		_acc( other._acc ),
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
	for (double &x : vector<double>{x0min,SEND_EWMA(point.first),x0max}) {
		for (double &y : vector<double>{x1min,REC_EWMA(point.first),x1max}) {
			for (double &z : vector<double>{x2min,RTT_RATIO(point.first),x2max}) {
				if ( ( x == SEND_EWMA(point.first ) ) || 
						 ( y == REC_EWMA(point.first ) )  || 
						 ( z == RTT_RATIO(point.first ) ) ) {
					SignalTuple tmp = make_tuple(x,y,z);
					grid._points[tmp] = interpolate(tmp);
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
	for (int i=0; i<3; i++) {
		sort(grid._signals[i].begin(), grid._signals[i].end());
	}
	// update the interp info the reflect the modified grid
	_interp_info_list = get_interp_info();

}

vector< InterpInfo > LerpSender::get_interp_info( void )
{
	printf("In get interp info \n");
	vector< InterpInfo > vec;

	for ( double send_min: _grid._signals[0] ) {
		for ( double rec_min: _grid._signals[1] ) {
			for ( double ratio_min: _grid._signals[2] ) {
				for ( double send_max: _grid._signals[0] ) {
					for ( double rec_max: _grid._signals[1] ) {
						for ( double ratio_max: _grid._signals[2] ) {
							if (!( ( send_max <= send_min ) || ( rec_max <= rec_min ) || ( ratio_max <= ratio_min ) )) {
							printf("BAR\n");
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
								printf("I is %d\n", i );
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
								info.window_incr_vals.push_back(CWND_INC( a ));
								info.window_mult_vals.push_back( CWND_MULT( a ));
								info.intersend_vals.push_back( MIN_SEND( a ));
								printf("The cur values are %f, %f, %f\n", cur_send, cur_rec, cur_ratio );
							}

							vector< double > grid1;
							vector< double > grid2;
							vector< double > grid3;

							grid1.push_back( info.min_send_ewma );
							grid1.push_back( info.max_send_ewma );
							grid2.push_back( info.min_rec_ewma );
							grid2.push_back( info.max_rec_ewma );
							grid3.push_back( info.min_rtt_ratio );
							grid3.push_back( info.max_rtt_ratio );

							info.grid_iter_list.push_back( grid1.begin() );
							info.grid_iter_list.push_back ( grid2.begin() );
							info.grid_iter_list.push_back( grid3.begin() );
							vec.push_back( info );
							}
						}
					}
				}
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
	// find the bounding box for the signal tuple, find correct interp info struct
	double obs_send_ewma = SEND_EWMA(t);
	double obs_rec_ewma = REC_EWMA(t);
	double obs_rtt_ratio = RTT_RATIO(t);

	double x0min, x0max, x1min, x1max, x2min, x2max;
	printf("size is %f\n", double( _grid._signals[0].size() ) );
  int i = 0;
  while ( obs_send_ewma >= _grid._signals[0][i] ) { i ++; }
  x0min = _grid._signals[0][ max( i-1, 0 ) ];
  x0max = _grid._signals[0][ min( i, int(_grid._signals[0].size() -  1) ) ];

  i = 0;
  while ( obs_rec_ewma >= _grid._signals[1][i] ) { i ++; }
  x1min = _grid._signals[1][ max( i-1, 0 ) ];
  x1max = _grid._signals[1][ min( i, int(_grid._signals[1].size() - 1) ) ];

  i = 0;
  while ( obs_rtt_ratio >= _grid._signals[2][i] ) { i ++; }
  x2min = _grid._signals[2][ max( i-1, 0 ) ];
  x2max = _grid._signals[2][ min( i, int(_grid._signals[2].size() - 1) ) ];
	printf("6 VALS ARE %f, %f, %f, %f, %f, %f\n", x0min, x0max, x1min, x1max, x2min, x2max );
	printf("Found the bounding box\n");
	InterpInfo interp;
	for ( InterpInfo info: _interp_info_list ) {
		// check if the min, max match for each
		if ( ( x0min == info.min_send_ewma ) && ( x0max == info.max_send_ewma ) && ( x1min == info.min_rec_ewma ) && ( x1max == info.max_rec_ewma ) &&  ( x2min == info.min_rtt_ratio ) && ( x2max == info.max_rtt_ratio ) ) {
			printf("%f\n", info.max_send_ewma );
			interp = info;
			printf("%f\n", interp.min_send_ewma );
		}
	}
	return interpolate_linterp( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio, interp );
}

void LerpSender::update_interp_info( void )
{
	_interp_info_list = get_interp_info();
}

ActionTuple LerpSender::interpolate_linterp( double s, double r, double t, InterpInfo info) {
	// use linterp library to calculate the RGI linear interpolation in the correct bounding cube
	// use a map that stores min_send, min_rec, min_ratio -> max values to all the arguments needed for the interp multilinear function
	printf("Interpolating %f, %f, %f\n", s, r, t);
	printf("In interpolate linterp function\n");
	for ( int i = 0; i < 8; i ++ ) {
		printf("The info for %d is %f\n", i, info.window_incr_vals[i]);
		printf("The info for %d is %f\n", i, info.window_mult_vals[i]);
		printf("The info for %d is %f\n", i, info.intersend_vals[i]);
	}
	InterpMultilinear<3, double > interp_ML_incr( info.grid_iter_list.begin(), info.grid_sizes.begin(), &(info.window_incr_vals[0]), (&(info.window_incr_vals[0])) + 8);
	InterpMultilinear<3, double > interp_ML_mult( info.grid_iter_list.begin(), info.grid_sizes.begin(), &(info.window_mult_vals[0]), (&(info.window_mult_vals[0])) + 8);
	InterpMultilinear<3, double > interp_ML_send( info.grid_iter_list.begin(), info.grid_sizes.begin(), &(info.intersend_vals[0]), (&(info.intersend_vals[0])) + 8);
  array< double, 3> args = {r,s,t};
	printf("Interp multilinear functions seem ok\n");
	double incr = interp_ML_incr.interp( args.begin() );
	double mult = interp_ML_mult.interp( args.begin() );
	double send = interp_ML_send.interp( args.begin() );
	printf("Returning %f, %f, %f\n", incr, mult, send);
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
