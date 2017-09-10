#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>

#include "smartbreeder.hh"
#include "dna.pb.h"
#include "configrange.hh"
using namespace std;

jmp_buf flush_whisker_and_quit;
bool already_handling_signal = false;

void handle_sigint(int signum) {
	if (!already_handling_signal) {
		already_handling_signal = true;
		printf("Caught signal %d. Jumping back out to main loop.\n", signum);
		longjmp(flush_whisker_and_quit, 1);
	} else {
		printf("Already handling signal %d.\n", signum);
	}
}
void print_range( const Range & range, const string & name )
{
  printf( "Optimizing for %s over [%f : %f : %f]\n", name.c_str(),
    range.low, range.incr, range.high );
}

void print_outcome(const Evaluator<WhiskerTree>::Outcome outcome)
{
  double norm_score = 0;
  for ( auto &run: outcome.throughputs_delays ) {
    printf( "===\nconfig: %s\n", run.first.str().c_str() );
    for ( auto &x : run.second ) {
      printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
      norm_score += log2( x.first / run.first.link_ppt ) - log2( x.second / run.first.delay );
    }
  }
  printf( "normalized_score = %f\n", norm_score );

  printf( "Whiskers: %s\n", outcome.used_actions.str().c_str() );
  printf("Outcome.score is %f\n", outcome.score );
}

int main( int argc, char *argv[] )
{
  WhiskerTree whiskers;
  string output_filename;
  BreederOptions options;
  WhiskerImproverOptions whisker_options;
  RemyBuffers::ConfigRange input_config;
  string config_filename;

  for ( int i = 1; i < argc; i++ ) {
    string arg( argv[ i ] );
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

    } else if ( arg.substr( 0, 3 ) == "of=" ) {
      output_filename = string( arg.substr( 3 ) );

    } else if ( arg.substr( 0, 4 ) == "opt=" ) {
      whisker_options.optimize_window_increment = false;
      whisker_options.optimize_window_multiple = false;
      whisker_options.optimize_intersend = false;
      for ( char & c : arg.substr( 4 ) ) {
        if ( c == 'b' ) {
          whisker_options.optimize_window_increment = true;
        } else if ( c == 'm' ) {
          whisker_options.optimize_window_multiple = true;
        } else if ( c == 'r' ) {
          whisker_options.optimize_intersend = true;
        } else {
          fprintf( stderr, "Invalid optimize option: %c\n", c );
          exit( 1 );
        }
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

  if ( config_filename.empty() ) {
    fprintf( stderr, "An input configuration protobuf must be provided via the cf= option. \n");
    fprintf( stderr, "You can generate one using './configuration'. \n");
    exit ( 1 );
  }

  options.config_range = ConfigRange( input_config );



  printf( "#######################\n" );
  printf( "Evaluator simulations will run for %d ticks\n",
    options.config_range.simulation_ticks );
  printf( "Optimizing window increment: %d, window multiple: %d, intersend: %d\n",
          whisker_options.optimize_window_increment, whisker_options.optimize_window_multiple,
          whisker_options.optimize_intersend);
  print_range( options.config_range.link_ppt, "link packets_per_ms" );
  print_range( options.config_range.rtt, "rtt_ms" );
  print_range( options.config_range.num_senders, "num_senders" );
  print_range( options.config_range.mean_on_duration, "mean_on_duration" );
  print_range( options.config_range.mean_off_duration, "mean_off_duration" );
  print_range( options.config_range.stochastic_loss_rate, "stochastic_loss_rate" );
  if ( options.config_range.buffer_size.low != numeric_limits<unsigned int>::max() ) {
    print_range( options.config_range.buffer_size, "buffer_size" );
  } else {
    printf( "Optimizing for infinitely sized buffers. \n");
  }

  printf( "Initial rules (use if=FILENAME to read from disk): %s\n", whiskers.str().c_str() );
  printf( "#######################\n" );

  if ( !output_filename.empty() ) {
    printf( "Writing to \"%s.N\".\n", output_filename.c_str() );
  } else {
    printf( "Not saving output. Use the of=FILENAME argument to save the results.\n" );
  }
  volatile unsigned int run = 0;
  // make an instance of "smart breeder"
  SmartBreeder breeder( options, whisker_options );
  printf("Registering signal handler.\n");
  signal(SIGINT, handle_sigint);

  volatile bool keep_going = true;
  while ( keep_going ) {
	Evaluator< WhiskerTree >::Outcome outcome;
	volatile int num_generations = ( run == 0 ) ? 2 : 3; // 2 if 1st outfile, 3 afterwards
	if (setjmp(flush_whisker_and_quit) == 0) { // real return
		already_handling_signal = false;
		outcome = breeder.improve( whiskers, num_generations );
		already_handling_signal = true;
		printf( "run = %u, score = %f\n", run, outcome.score );
	} else {
		printf("Returned to main loop from signal handler.\n");
		keep_going = false;
	}

    printf( "whiskers: %s\n", whiskers.str().c_str() );


    if ( !output_filename.empty() ) {
      char of[ 128 ];
      snprintf( of, 128, "%s.%d", output_filename.c_str(), run );
      fprintf( stderr, "Writing to \"%s\"... ", of );
      int fd = open( of, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR );
      if ( fd < 0 ) {
				perror( "open" );
				exit( 1 );
      }

      auto remycc = whiskers.DNA();
      remycc.mutable_config()->CopyFrom( options.config_range.DNA() );
      remycc.mutable_optimizer()->CopyFrom( Whisker::get_optimizer().DNA() );
      if ( not remycc.SerializeToFileDescriptor( fd ) ) {
				fprintf( stderr, "Could not serialize RemyCC.\n" );
				exit( 1 );
      }

      if ( close( fd ) < 0 ) {
				perror( "close" );
				exit( 1 );
      }

      fprintf( stderr, "done.\n" );
    }

    fflush( NULL );
    run++;
  }
}

