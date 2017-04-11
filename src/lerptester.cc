#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ratbreeder.hh"
#include "dna.pb.h"
#include "configrange.hh"
#include "lerpsender.hh"
#include "lerpbreeder.hh"

using namespace std;


void print_range( const Range & range, const string & name )
{
  printf( "Optimizing for %s over [%f : %f : %f]\n", name.c_str(),
    range.low, range.incr, range.high );
}

int main( int argc, char *argv[] )
{
	RemyBuffers::ConfigRange input_config;
	double rtt_ratio = 0;
	double send_ewma = 0;
	double rec_ewma = 0;
	string config_filename;
  bool interp_test_only = true;
	for ( int i = 1; i < argc; i++ ) {
		string arg( argv[i] );
		if ( arg.substr( 0, 4 ) == "rtt=" ) {
     	rtt_ratio = atof( arg.substr( 4 ).c_str() );
		} else if ( arg.substr( 0, 5 ) == "send=" ) {
			send_ewma = atof( arg.substr( 5 ).c_str() );
		} else if ( arg.substr( 0, 4 ) == "rec=" ) {
			rec_ewma = atof( arg.substr( 4 ).c_str() );
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

  if (!( config_filename.empty() )) {
    interp_test_only = false;
    printf("Will use provided config to run an evaluation test\n");
  }
	PointGrid grid;

  if ( interp_test_only) {

	// replace 0,0,0 and make a sender
	grid._points[make_tuple(0,0,0)] = make_tuple( 60, .8, .22);
	//Evaluator< WhiskerTree > eval( config_range );
  LerpSender sender( grid );
	//printf("Grid is %s\n", grid.str().c_str() );

	SignalTuple signal = make_tuple( send_ewma, rec_ewma, rtt_ratio );
	ActionTuple a = sender.interpolate( signal );
	printf("Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );


  //SignalTuple ns = make_tuple( 30, 20, 25 );
  //ActionTuple na = make_tuple( 100, .9, .14 );
  //sender.add_inner_point( make_pair( ns, na ), grid );
	//printf("Grid is %s\n", grid.str().c_str() );

  //signal = make_tuple( send_ewma, rec_ewma, rtt_ratio );
	//a = sender.interpolate( signal );
	printf("Now Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );
  } else {
	  grid._points[make_tuple(0,0,0)] = make_tuple( 60, .8, .22);
    grid._debug = false;
    ConfigRange config_range = ConfigRange( input_config );
    Evaluator< WhiskerTree > eval( config_range );

    // add a median
    SignalTuple median = eval.grid_get_median_signal( grid, 1 );
    printf("New median is %s\n", _stuple_str( median ).c_str() );
    // test that the observed values are not crazy after adding the median
    grid._debug = true;
    eval.score_lerp( grid, 1 );
  }

}
