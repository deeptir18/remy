#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits>
#include "ratbreeder.hh"
#include "configrange.pb.h"
#include "configrange.hh"

using namespace std;

int main( int argc, char *argv[] )
{
  WhiskerTree whiskers;
  string output_filename;
  InputConfigRange::ConfigRange input_config;
  bool supplied_config = false;
  string training_config_file;
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
    } else if ( arg.substr(0, 3 ) == "cf=" ) {
      string config_filename( arg.substr( 3 ) );
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
      supplied_config = true;
    } else if ( arg.substr( 0, 3 ) == "tf=" ) {
      training_config_file = string( arg.substr( 3 ) );
    }
  }
  if ( !(supplied_config) ) {
    fprintf( stderr, "Provide an input config protobuf. \n");
    exit ( 1 );
  }
 
  ConfigRange configuration_range;
  configuration_range.link_packets_per_ms = make_pair( input_config.link_ppt().lo(), input_config.link_ppt().hi() );
  configuration_range.link_ppt_incr = input_config.link_ppt().incr();

  configuration_range.rtt_ms = make_pair( input_config.delay().lo(), input_config.delay().hi() );
  configuration_range.rtt_incr = input_config.delay().incr();

  configuration_range.num_senders = make_pair( input_config.num_senders().lo(), input_config.num_senders().hi() );
  configuration_range.senders_incr = input_config.num_senders().incr();

  configuration_range.mean_on_duration = make_pair( input_config.mean_on_duration().lo(), input_config.mean_on_duration().hi() );
  configuration_range.on_incr = input_config.mean_on_duration().incr();

  configuration_range.mean_off_duration = make_pair( input_config.mean_off_duration().lo(), input_config.mean_off_duration().hi() );
  configuration_range.off_incr = input_config.mean_off_duration().incr();

  configuration_range.bdp_multiplier = make_pair( input_config.bdp_multiplier().lo(), input_config.bdp_multiplier().hi() );
  configuration_range.bdp_incr = input_config.bdp_multiplier().incr();

  RatBreeder breeder( configuration_range );

  unsigned int run = 0;

  printf( "#######################\n" );
  printf( "Optimizing for link packets_per_ms in [%f, %f]\n",
	  configuration_range.link_packets_per_ms.first,
	  configuration_range.link_packets_per_ms.second );
  printf( "Optimizing for rtt_ms in [%f, %f]\n",
	  configuration_range.rtt_ms.first,
	  configuration_range.rtt_ms.second );
  printf( "Optimizing for num_senders = [%d, %d]\n",
	  configuration_range.num_senders.first,
          configuration_range.num_senders.second );
  printf( "Optimizing for mean_on_duration in [%f, %f], mean_off_duration in [%f, %f]\n",
	  configuration_range.mean_on_duration.first, configuration_range.mean_on_duration.second, configuration_range.mean_off_duration.first, configuration_range.mean_off_duration.second );
  if ( (configuration_range.bdp_multiplier.first != numeric_limits<unsigned int>::max()) && ( configuration_range.bdp_multiplier.second != numeric_limits<unsigned int>::max()) ) {
    printf( "Optimizing for bdp_multiplier_range = [%d, %d]\n",
          configuration_range.bdp_multiplier.first,
          configuration_range.bdp_multiplier.second );
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
  
  if ( !training_config_file.empty() ) {
    printf( "Writing training configs to \"%s.N\".\n", training_config_file.c_str() );
  } else {
    printf( "Not recording the training config parameters to a protobuf.\n" );
  }
  InputConfigRange::ConfigVector training_configs;
  bool written = false;

  while ( 1 ) {
    auto outcome = breeder.improve( whiskers ) ;
    printf( "run = %u, score = %f\n", run, outcome.score );

    printf( "whiskers: %s\n", whiskers.str().c_str() );
    if ( !(written) ) {
      for ( auto &run : outcome.throughputs_delays) {
        // record the config to the protobuf
        InputConfigRange::NetConfig* net_config = training_configs.add_config();
        *net_config = run.first.DNA();
        written = true;
      
      }
    }
    for ( auto &run : outcome.throughputs_delays ) { 
      for ( auto &x : run.second ) {
	printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
      }
    }
    
    if ( !training_config_file.empty() ) {
      char tof[ 128 ];
      snprintf( tof, 128, "%s.%d", training_config_file.c_str(), run );
      fprintf( stderr, "Writing to \"%s\"... ", tof );
      int tfd = open( tof, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR );
      if ( tfd < 0 ) {
        perror( "open" );
        exit( 1 );
      }
      if ( not training_configs.SerializeToFileDescriptor( tfd ) ) {
        fprintf( stderr, "Could not serialize training ConfigVector.\n" );
        exit( 1 );
      }
     if ( close( tfd ) < 0 ) {
        perror( "close" );
        exit( 1 );
      }
    }

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
      remycc.mutable_config()->CopyFrom( configuration_range.DNA() );
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

  return 0;
}
