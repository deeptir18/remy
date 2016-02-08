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
using namespace std;

int main( int argc, char *argv[] )
{
  WhiskerTree whiskers;
  string output_filename;
  RemyBuffers::ConfigRange input_config;
  string config_filename;
  RemyBuffers::ConfigVector input_nets;
  bool input_pts = false; // if the user provides an input tht is a vector of points

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
    } else if ( arg.substr(0, 4 ) == "pcf=" ) {
      input_pts = true;
      config_filename = string( arg.substr( 4 ) );
      int pfd = open( config_filename.c_str(), O_RDONLY );
      if ( pfd < 0 ) {
        perror( "open config file error");
        exit( 1 );
      }
      if ( !input_nets.ParseFromFileDescriptor( pfd ) ) {
        fprintf( stderr, "Could not parse input config vector of pts from file %s. \n", config_filename.c_str() );
        exit ( 1 );
      }
      if ( close( pfd ) < 0 ) {
        perror( "close" );
        exit( 1 );
      }
    } 
  }

  if ( config_filename.empty() ) {
    fprintf( stderr, "Provide an input config range or points protobuf (generated using './configure'). \n");
    exit ( 1 );
  }


  ConfigRange configuration_range;
  printf(" The input pts has size %d\n, ", input_nets.config_size() );
  if (input_pts) {
    for (int i = 0; i < input_nets.config_size(); i++ ) {
      const RemyBuffers::NetConfig &config = input_nets.config(i);
      configuration_range.configs.push_back( NetConfig().set_link_ppt( config.link_ppt() ).set_delay( config.delay() ).set_num_senders( config.num_senders() ).set_on_duration( config.mean_on_duration() ).set_off_duration( config.mean_off_duration() ).set_buffer_size( config.buffer_size() ) );
    configuration_range.is_range = false;
    }
    cout <<  configuration_range.configs.size() << "\n";
    for ( auto &config : configuration_range.configs ) {
      printf( " NUM SENDERS for the things are %f \n", config.num_senders );
    }
  } else { 
    configuration_range.link_ppt = Range( input_config.link_packets_per_ms() );
    configuration_range.rtt = Range( input_config.rtt() ); 
    configuration_range.num_senders = Range( input_config.num_senders() ); 
    configuration_range.mean_on_duration = Range( input_config.mean_on_duration() ); 
    configuration_range.mean_off_duration = Range( input_config.mean_off_duration() );
    configuration_range.buffer_size = Range( input_config.buffer_size() ); 
  }
  RatBreeder breeder( configuration_range );
  unsigned int run = 0;
  if ( !( input_pts ) ) {
    printf( "#######################\n" );
    printf( "Optimizing for link packets_per_ms in [%f, %f]\n",
	  configuration_range.link_ppt.low,
	  configuration_range.link_ppt.high );
    printf( "Optimizing for rtt_ms in [%f, %f]\n",
	  configuration_range.rtt.low,
	  configuration_range.rtt.high );
    printf( "Optimizing for num_senders in [%f, %f]\n",
	  configuration_range.num_senders.low, configuration_range.num_senders.high );
    printf( "Optimizing for mean_on_duration in [%f, %f], mean_off_duration in [ %f, %f]\n",
       	  configuration_range.mean_on_duration.low, configuration_range.mean_on_duration.high, configuration_range.mean_off_duration.low, configuration_range.mean_off_duration.high );
    if ( configuration_range.buffer_size.low != numeric_limits<unsigned int>::max() ) {
      printf( "Optimizing for buffer_size in [%f, %f]\n",
              configuration_range.buffer_size.low,
              configuration_range.buffer_size.high );
    } else {
      printf( "Optimizing for infinitely sized buffers. \n");
    }

  }  else {
    printf( "Optimizing for a specific set of networks specified, not a range" );
  }
  printf( "Initial rules (use if=FILENAME to read from disk): %s\n", whiskers.str().c_str() );
  printf( "#######################\n" );

  if ( !output_filename.empty() ) {
    printf( "Writing to \"%s.N\".\n", output_filename.c_str() );
  } else {
    printf( "Not saving output. Use the of=FILENAME argument to save the results.\n" );
  }

  RemyBuffers::ConfigVector training_configs;
  bool written = false;

  while ( 1 ) {
    auto outcome = breeder.improve( whiskers );
    printf( "run = %u, score = %f\n", run, outcome.score );

    printf( "whiskers: %s\n", whiskers.str().c_str() );

    for ( auto &run : outcome.throughputs_delays ) {
      if ( !(written) ) {
        for ( auto &run : outcome.throughputs_delays) {
          // record the config to the protobuf
          RemyBuffers::NetConfig* net_config = training_configs.add_config();
          *net_config = run.first.DNA();
          written = true;
      
        }
      }
      printf( "===\nconfig: %s\n", run.first.str().c_str() );
      for ( auto &x : run.second ) {
	printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
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
      remycc.mutable_configvector()->CopyFrom( training_configs );
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
