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
  bool lerp_test = false;
	for ( int i = 1; i < argc; i++ ) {
		string arg( argv[i] );
    if ( arg.substr(0, 4 ) == "test" ) {
      lerp_test = true;
    }
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
  if ( lerp_test ) {
    // set the grid to have an "optimal" point for 0,0,0 for testing
    // inc=51.671,mult=0.34791,intr=0.466448
    grid._points[make_tuple(0,0,0)] = make_tuple(54, .82, .35 );
  }
  ConfigRange config_range = ConfigRange( input_config );
	LerpBreeder breeder( config_range );


  unsigned int run = 0;

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
	printf("RUN IS CURRENTLY %d\n", run);
  printf( "#######################\n" );
	while ( 1 ) {
		printf("The point grid is: \n%s\n", grid.str().c_str() );

		auto outcome = breeder.improve( grid );
    printf( "run = %u, score = %f\n", run, outcome.score );
		printf( "Current point grid:\n %s\n", grid.str().c_str() );

		/*for ( auto& run : outcome.throughputs_delays ) {
      printf( "===\nconfig: %s\n", run.first.str().c_str() );
      for ( auto &x : run.second ) {
				printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
      }
		}*/
		run ++;
	}
	return 0;
}
