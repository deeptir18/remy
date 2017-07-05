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
  double slow_rec_ewma = 0;
	string config_filename;
  bool interp_test_only = true;
  RemyBuffers::WhiskerTree tree;
  WhiskerTree whiskers;
  bool included_whisker = false;
	for ( int i = 1; i < argc; i++ ) {
		string arg( argv[i] );
		if ( arg.substr( 0, 4 ) == "rtt=" ) {
     	rtt_ratio = atof( arg.substr( 4 ).c_str() );
		} else if ( arg.substr( 0, 5 ) == "send=" ) {
			send_ewma = atof( arg.substr( 5 ).c_str() );
		} else if ( arg.substr( 0, 4 ) == "rec=" ) {
			rec_ewma = atof( arg.substr( 4 ).c_str() );
		} else if ( arg.substr( 0, 9 ) == "slow_rec=" ) {
      slow_rec_ewma = atof( arg.substr( 9 ).c_str() );
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
    } else if ( arg.substr( 0, 3 ) == "if=" ) {
      included_whisker = true;
      string filename( arg.substr( 3 ) );
      int fd = open( filename.c_str(), O_RDONLY );
      if ( fd < 0 ) {
	perror( "open" );
	exit( 1 );
      }

      if ( !tree.ParseFromFileDescriptor( fd ) ) {
	fprintf( stderr, "Could not parse %s.\n", filename.c_str() );
	exit( 1 );
      }
      whiskers = WhiskerTree( tree );

      if ( close( fd ) < 0 ) {
	perror( "close" );
	exit( 1 );
      }
    }
	}

  if ( included_whisker ) {
    printf("Included a whisker\n");
  }

  if (!( config_filename.empty() )) {
    interp_test_only = false;
    printf("Will use provided config to run an evaluation test\n");
  }
	PointGrid grid;

  if ( interp_test_only) {

	// replace 0,0,0 and make a sender inc=54,mult=0.82,intr=0.35
	grid._points[make_tuple(0,0,0,0)] = make_tuple( 54, .82, .35);
	//Evaluator< WhiskerTree > eval( config_range );
  LerpSender sender( grid );
	//printf("Grid is %s\n", grid.str().c_str() );

	SignalTuple signal = make_tuple( send_ewma, rec_ewma, rtt_ratio, slow_rec_ewma );
	ActionTuple a = sender.interpolate( signal );
	printf("Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );


	printf("Now Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );
  }

}
