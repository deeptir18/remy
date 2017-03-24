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
	string config_filename;
	for ( int i = 1; i < argc; i++ ) {
		string arg( argv[i] );
    if ( arg.substr(0, 3 ) == "cf=" ) {
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

	if ( config_filename.empty() ) {
    fprintf( stderr, "An input configuration protobuf must be provided via the cf=option. \n");
    fprintf( stderr, "You can generate one using './configuration'. \n");
    exit ( 1 );
  }

	PointGrid grid;
  ConfigRange config_range = ConfigRange( input_config );


  printf( "#######################\n" );
  printf( "Evaluator simulations will run for %d ticks\n",
    config_range.simulation_ticks );
  print_range( config_range.link_ppt, "link packets_per_ms" );
  print_range( config_range.rtt, "rtt_ms" );
  print_range( config_range.num_senders, "num_senders" );
  print_range( config_range.mean_on_duration, "mean_on_duration" );
  print_range( config_range.mean_off_duration, "mean_off_duration" );
  print_range( config_range.stochastic_loss_rate, "stochastic_loss_rate" );
  if ( config_range.buffer_size.low != numeric_limits<unsigned int>::max() ) {
    print_range( config_range.buffer_size, "buffer_size" );
  } else {
    printf( "Optimizing for infinitely sized buffers. \n");
  }

	// replace 0,0,0 and make a sender
	grid._points[make_tuple(0,0,0)] = make_tuple( 60, .8, .22 );
	Evaluator< WhiskerTree > eval( config_range );
  LerpSender sender( grid );
	// check if the thing interpolates weirdly
		// make a tuple with the signal value tuples to optimize
		/*vector< SignalTuple > signals;
		for ( double send_ewma = 0; send_ewma <= MAX_SEND_EWMA; send_ewma += MAX_SEND_EWMA ) {
			for ( double rec_ewma = 0; rec_ewma <= MAX_REC_EWMA; rec_ewma += MAX_REC_EWMA ) {
				for ( double rtt_ratio = 0; rtt_ratio <= MAX_RTT_RATIO; rtt_ratio += MAX_RTT_RATIO ) {
					SignalTuple signal = make_tuple( send_ewma, rec_ewma, rtt_ratio );
					signals.emplace_back( signal );
				}
			}
		}
		for ( SignalTuple signal: signals) {
			ActionTuple a = sender.interpolate( signal );
			printf("Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );
	  }*/

		SignalTuple signal = make_tuple( 20,1, 45 );
		ActionTuple a = sender.interpolate( signal );
		printf("Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );

}
