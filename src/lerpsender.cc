#include "lerpsender.hh"
#include <limits>
#include <iomanip>
#include <algorithm>
#include <boost/array.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp> 
#include "delaunay_d_interp.h"
#include <stdlib.h>
#include "p2.h"
#include <stdio.h>

using namespace std;

static constexpr double INITIAL_WINDOW = 100; /* INITIAL WINDOW OF 1 */

/*****************************************************************************/
#define DEFAULT_INCR 1
#define DEFAULT_MULT 1
#define DEFAULT_SEND 3
#define UPPER_QUANTILE 0.95
	PointGrid::PointGrid( bool track )
	:	 _track ( track ),
		 // _acc ( NUM_SIGNALS, p2_t( UPPER_QUANTILE ) ),
		 _send_acc( UPPER_QUANTILE ),
		 _rec_acc( UPPER_QUANTILE ),
		 _rtt_acc( UPPER_QUANTILE ),
     _slow_rec_acc( UPPER_QUANTILE ),
     _debug( false ),
	   _points( )
{
	// Default values: set all 8 corners of the box to the default action
	for ( int i = 0; i <= MAX_SEND_EWMA; i += MAX_SEND_EWMA) {
		for ( int j = 0; j <= MAX_REC_EWMA; j += MAX_REC_EWMA) {
			for ( int k = 0; k <= MAX_RTT_RATIO; k += MAX_RTT_RATIO) {
        for ( int l = 0; l <= MAX_SLOW_REC_EWMA; l += MAX_SLOW_REC_EWMA ) {
				  _points[make_tuple(i,j,k, l)] = make_tuple(double(DEFAULT_INCR),double(DEFAULT_MULT),double(DEFAULT_SEND));
        }
			}
		}
	}
}


// Copy constructor
PointGrid::PointGrid( PointGrid & other, bool track )
	:	_track( track ),
		 _send_acc( UPPER_QUANTILE ),
		 _rec_acc( UPPER_QUANTILE ),
		 _rtt_acc( UPPER_QUANTILE ),
     _slow_rec_acc( UPPER_QUANTILE),
    _debug( other._debug ),
	  _points( other._points )
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
				 << RTT_RATIO(t) << ",slow_rec="
         << SLOW_REC_EWMA( t )
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

void PointGrid::add_point( PointObj p )
{
  _points[p.first] = p.second;
}

string _point_str( PointObj p ) {
	return _stuple_str(p.first) + " -> " + _atuple_str(p.second);
}

string PointGrid::str() {
	string ret;
	for ( auto it = begin(); it != end(); ++it ) {
		ret += _stuple_str(it->first) + string( " -> " ) + _atuple_str(it->second) + string( "\n" ); 
	}
	return ret;
}

void PointGrid::track ( double s, double r, double t, double sr ) {
	if (_track) {
	//printf("%f %f %f\n", s, r ,t);
		/*
		_acc[0]( s );
		_acc[1]( r );
		_acc[2]( t );
		_acc[0].add( s );
		_acc[1].add( r );
		_acc[2].add( t );
		*/
		_send_acc.add( s );
		_rec_acc.add( r );
		_rtt_acc.add( t );
    _slow_rec_acc.add( sr );
	}
}

SignalTuple PointGrid::get_median_signal() {
	assert(_track);
	return make_tuple(
			/*
			boost::accumulators::median( _acc[0] ),
			boost::accumulators::median( _acc[1] ),
			boost::accumulators::median( _acc[2] )
			_acc[0].result( ),
			_acc[1].result( ),
			_acc[2].result( )
			*/
			_send_acc.result( ),
			_rec_acc.result( ),
			_rtt_acc.result( ),
      _slow_rec_acc.result( )
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
     _largest_ack( -1 ),
     _signal_list( )
{
  for ( auto it = _grid._points.begin(); it != _grid._points.end(); ++it ) {
    ActionTuple a = it->second;
    SignalTuple signal = it->first;
    array< double, 4 > args = {SEND_EWMA( signal ), REC_EWMA( signal ), RTT_RATIO( signal ), SLOW_REC_EWMA( signal )};
    _signal_list.push_back( make_pair( args, a ) );
  }

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
  double obs_slow_rec_ewma = (double)(memory.field( 3 ) );
	_grid.track( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio, obs_slow_rec_ewma );

	ActionTuple a = interpolate( make_tuple( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio, obs_slow_rec_ewma ) );

  _the_window = min( max( 0.0,  _the_window * CWND_MULT(a) + CWND_INC(a)  ), 1000000.0 );
  _intersend_time = MIN_SEND(a);
  if ( _grid._debug ) {
    if ( ( rand() % 100000 ) == 87 ) {
      printf("Obs values: [%f, %f, %f, %f]\n", obs_send_ewma, obs_rec_ewma, obs_rtt_ratio, obs_slow_rec_ewma );
      printf("Window/intersend values: [%f, %f]\n", _the_window, _intersend_time );
    }
  }
}


ActionTuple LerpSender::interpolate( SignalTuple t ) {
	double obs_send_ewma = SEND_EWMA(t);
	double obs_rec_ewma = REC_EWMA(t);
	double obs_rtt_ratio = RTT_RATIO(t);
  double slow_rec_ewma = SLOW_REC_EWMA(t);
	return interpolate_delaunay( obs_send_ewma, obs_rec_ewma, obs_rtt_ratio, slow_rec_ewma);
}

ActionTuple LerpSender::interpolate_delaunay( double s, double r, double t, double sr ) {
  printf("Trying to interpolate %f, %f, %f, %f\n", s, r, t, sr);
  Delaunay_incremental_interp_d incr_triang(4);
  Delaunay_incremental_interp_d mult_triang(4);
  Delaunay_incremental_interp_d send_triang(4);
  for ( auto it: _signal_list ) {
    ActionTuple a = it.second;
    double window_incr = CWND_INC( a );
    double window_mult = CWND_MULT( a );
    double intersend = MIN_SEND( a );
    array < double, 4 > args = it.first;
    incr_triang.insert( args.begin(), args.end(), window_incr );
    mult_triang.insert( args.begin(), args.end(), window_mult );
    send_triang.insert( args.begin(), args.end(), intersend );
  }

  // now interpolate each
  array< double, 4 > interp_args = { s, r, t, sr };
  printf("About to create triangulations\n");
  double interpolated_incr = incr_triang.interp( interp_args.begin(), interp_args.end() );
  double interpolated_mult = mult_triang.interp( interp_args.begin(), interp_args.end() );
  double interpolated_send = send_triang.interp( interp_args.begin(), interp_args.end() );
  printf("Finished interpolation -> %f, %f %f\n",  interpolated_incr, interpolated_mult, interpolated_send);
  return make_tuple( interpolated_incr, interpolated_mult, interpolated_send );
}



/*****************************************************************************/
