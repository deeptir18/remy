#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ratbreeder.hh"
#include "dna.pb.h"
#include "configrange.hh"
#include "lerpsender.cc"
#include "memory.hh"
/*This file takes in an input whisker as well as hardcodes a grid for the grid ::q
  :interpolation and a grid map for the delaunay interpolation,
 and for a range of points prints out what the given action maps to (so we can more clearly see the difference between what we have and Remy*/
using namespace std;
typedef tuple< double, double, double, double > NewSignalTuple; // with slow rec ewma
#define SEND_EWMA(s) get <0>(s)
#define REC_EWMA(s)  get <1>(s)
#define RTT_RATIO(s) get <2>(s)
#define SLOW_REC_EWMA(s) get <3>(s)

string _stuplenew_str( NewSignalTuple t ) {
	ostringstream stream;
	stream << "S[send=" 
		     << SEND_EWMA(t) << ",rec=" 
		     << REC_EWMA(t) << ",ratio=" 
				 << RTT_RATIO(t) << ", slow_rec=" 
         << SLOW_REC_EWMA(t) 
				 << "]";
	return stream.str();
}
ActionTuple interpolate_with_whisker( WhiskerTree whiskers, NewSignalTuple s )
{
 // create a fake memory object
 // modify it to contain the new signal tuple
 // then find the whisker with this signal tuple and return this whisker's action 
  // order send_ewma, rec_ewma, rtt_ratio, slow_rec_ewma, rtt_diff, queueing_delay
  Memory memory( { SEND_EWMA( s ), REC_EWMA( s ), RTT_RATIO( s ), SLOW_REC_EWMA( s ), 0, 0 } );
  const Whisker & whisker( whiskers.use_whisker( memory, false ) );
  double window_increment = double(whisker.window_increment());
  double window_multiple = whisker.window_multiple();
  double intersend = whisker.intersend();
  return make_tuple( window_increment, window_multiple, intersend );
}

ActionTuple grid_interpolator(SignalTuple s)
{
  // hardcode the point grid in this
  PointGrid grid;
  grid._points[make_tuple(0,0,0)] = make_tuple( 54, .82, .35 );
  LerpSender sender( grid );
  // could possibly add different inner points to see what happens
  return sender.interpolate( s );
}


ActionTuple delaunay_interpolator(SignalTuple s)
{
  PointGrid grid;
  grid._points[make_tuple(0,0,0)] = make_tuple( 54, .82, .35 );
  grid._points[make_tuple(.45, .65, 1.2)] = make_tuple(20, .9, .03);

  Delaunay_incremental_interp_d incr_triang(3);
  Delaunay_incremental_interp_d mult_triang(3);
  Delaunay_incremental_interp_d send_triang(3);
  for ( auto it = grid._points.begin(); it != grid._points.end(); ++it ) {
    ActionTuple a = it->second;
    SignalTuple signal = it->first;
    array< double, 3 > args = {SEND_EWMA( signal ), REC_EWMA( signal ), RTT_RATIO( signal )};
    double window_incr = CWND_INC( a );
    double window_mult = CWND_MULT( a );
    double intersend = MIN_SEND( a );
    incr_triang.insert( args.begin(), args.end(), window_incr );
    mult_triang.insert( args.begin(), args.end(), window_mult );
    send_triang.insert( args.begin(), args.end(), intersend );
  }

  // now interpolate each
  array< double, 3 > interp_args = { SEND_EWMA( s ), REC_EWMA( s ), RTT_RATIO( s )};
  double interpolated_incr = incr_triang.interp( interp_args.begin(), interp_args.end() );
  double interpolated_mult = mult_triang.interp( interp_args.begin(), interp_args.end() );
  double interpolated_send = send_triang.interp( interp_args.begin(), interp_args.end() );
  return make_tuple( interpolated_incr, interpolated_mult, interpolated_send );
}

// prints out what the three interpolations give us
void compare_interpolation( WhiskerTree whiskers, NewSignalTuple s )
{
  SignalTuple short_signal = make_tuple( SEND_EWMA( s ), REC_EWMA( s ), RTT_RATIO( s ) );
  ActionTuple whisker_a = interpolate_with_whisker( whiskers, s );
  ActionTuple grid_a = grid_interpolator( short_signal);
  ActionTuple delaunay_a = delaunay_interpolator( short_signal );
  printf("%s->", _stuplenew_str( s ).c_str() );
  printf("{whisker: %s}, ", _atuple_str( whisker_a ).c_str());
  printf("{Our grid: %s}, ", _atuple_str( grid_a ).c_str() );
  printf("{Delaunay grid: %s}\n", _atuple_str( delaunay_a ).c_str() );
}

int main( int argc, char *argv[] )
{
	RemyBuffers::ConfigRange input_config;
  WhiskerTree whiskers;
  string config_filename;
	for ( int i = 1; i < argc; i++ ) {
		string arg( argv[i] );
     if ( arg.substr( 0, 3 ) == "if=" ) {
      string filename( arg.substr( 3 ) );
      int fd = open( filename.c_str(), O_RDONLY );
      if ( fd < 0 ) {
	perror( "open" );
	exit( 1 );
      }

      RemyBuffers::WhiskerTree tree;
      if ( !tree.ParseFromFileDescriptor( fd ) ) {
	fprintf( stderr, "Could not parse %s.\n", filename.c_str() );
	exit( 1 );
      }
      whiskers = WhiskerTree( tree );

      if ( close( fd ) < 0 ) {
	perror( "close" );
	exit( 1 );
      }
    } else if ( arg.substr(0, 3 ) == "cf=" ) {
      config_filename = string( arg.substr( 3 ) );
      int cfd = open( config_filename.c_str(), O_RDONLY );
      if ( cfd < 0 ) {
        perror( "open config file error");
        exit( 1 );
      }
      if ( !input_config.ParseFromFileDescriptor( cfd ) ) {
        fprintf( stderr, "Could not parse input config from file %s. \n", config_filename.c_str() );
        exit ( 1 );
      }
      if ( close( cfd ) < 0 ) {
        perror( "close" );
        exit( 1 );
      }
    }
	}

  // tell me the action for a given point
  NewSignalTuple signal = make_tuple( .75, .8, .2, .3);
  compare_interpolation( whiskers, signal );

}
