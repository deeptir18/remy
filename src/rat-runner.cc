#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "evaluator.hh"
#include "configrange.hh"
#include "network.hh"


using namespace std;

int main( int argc, char *argv[] )
{
  WhiskerTree whiskers;
  RemyBuffers::ConfigVector input_nets;
  string networks_filename;
  bool provided_dna = false; 
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
      provided_dna = true;
      whiskers = WhiskerTree( tree );

      if ( close( fd ) < 0 ) {
	perror( "close" );
	exit( 1 );
      }

      if ( tree.has_config() ) {
	printf( "Prior assumptions:\n%s\n\n", tree.config().DebugString().c_str() );
      }

      if ( tree.has_optimizer() ) {
	printf( "Remy optimization settings:\n%s\n\n", tree.optimizer().DebugString().c_str() );
      }
    } else if ( arg.substr( 0, 8 ) == "pts_cfg=" ) {
        networks_filename = string( arg.substr( 8 ) ) ;
        int cfd = open ( networks_filename.c_str(), O_RDONLY );
        if ( cfd < 0 ) {
          perror( "open" );
          exit( 1 );
        }
        if ( !input_nets.ParseFromFileDescriptor( cfd ) ) {
          fprintf( stderr, "Could not parse input config vector of pts from file %s. \n", networks_filename.c_str() );
          exit ( 1 );
        }
        if ( close( cfd ) < 0 ) {
          perror( "close" );
          exit( 1 );
        }
     }
  }


  if ( networks_filename.empty() ) {
    fprintf( stderr, "Provide an input config points protobuf. \n");
    exit ( 1 );
  }

  if ( ! (provided_dna) ) {
    fprintf( stderr, "Provide an input dna file. \n");
    exit ( 1 );
  }


  ConfigRange configuration_range;
  for (int i = 0; i < input_nets.config_size(); i++ ) {
      const RemyBuffers::NetConfig &config = input_nets.config(i);
      configuration_range.configs.push_back( NetConfig().set_link_ppt( config.link_ppt() ).set_delay( config.delay() ).set_num_senders( config.num_senders() ).set_on_duration( config.mean_on_duration() ).set_off_duration( config.mean_off_duration() ).set_buffer_size( config.buffer_size() ) );
    configuration_range.is_range = false;

  }
  Evaluator eval( configuration_range, configuration_range.is_range );

  auto outcome = eval.score( whiskers, false, 10 );
  printf( "score = %f\n", outcome.score );
  double norm_score = 0;

  for ( auto &run : outcome.throughputs_delays ) {
    printf( "===\nconfig: %s\n", run.first.str().c_str() );
    for ( auto &x : run.second ) {
      printf( "sender: [tp=%f, del=%f]\n", x.first / run.first.link_ppt, x.second / run.first.delay );
      norm_score += log2( x.first / run.first.link_ppt ) - log2( x.second / run.first.delay );
    }
  }

  printf( "normalized_score = %f\n", norm_score );

  printf( "Whiskers: %s\n", outcome.used_whiskers.str().c_str() );

  return 0;
}
