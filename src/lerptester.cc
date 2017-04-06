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
	for ( int i = 1; i < argc; i++ ) {
		string arg( argv[i] );
		if ( arg.substr( 0, 4 ) == "rtt=" ) {
     	rtt_ratio = atof( arg.substr( 4 ).c_str() );
		} else if ( arg.substr( 0, 5 ) == "send=" ) {
			send_ewma = atof( arg.substr( 5 ).c_str() );
		} else if ( arg.substr( 0, 4 ) == "rec=" ) {
			rec_ewma = atof( arg.substr( 4 ).c_str() );
		}
	}


	PointGrid grid;



	// replace 0,0,0 and make a sender
	grid._points[make_tuple(0,0,0)] = make_tuple( 60, .8, .22);
	//Evaluator< WhiskerTree > eval( config_range );
  LerpSender sender( grid );
	printf("Grid is %s\n", grid.str().c_str() );

	SignalTuple signal = make_tuple( send_ewma, rec_ewma, rtt_ratio );
	ActionTuple a = sender.interpolate( signal );
	printf("Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );

}
