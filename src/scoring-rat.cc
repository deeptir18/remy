#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ratbreeder.hh"
#include "evaluator.hh"
#include "configrange.pb.h"
#include "dna.pb.h"
using namespace std;
int main(int argc, char *argv[] )
{
  WhiskerTree whiskers;
  InputConfigRange::NetConfig test_config;
  string config_filename;
  string filename;

  unsigned int num_senders;
  double link_ppt;
  double delay;
  double mean_on_duration;
  double mean_off_duration;

  for ( int i = 1; i < argc; i++ ) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "cf=" ) {
      config_filename = ( arg.substr( 3 ) );
      int cfd = open( config_filename.c_str(), O_RDONLY );
      if ( cfd < 0 ) {
        perror( "open config file error");
        exit( 1 );
      }
      if ( !test_config.ParseFromFileDescriptor( cfd ) ) {
        fprintf( stderr, "Could not parse input config from file %s. \n", config_filename.c_str() );
        exit ( 1 );
      }
      if ( close( cfd ) < 0 ) {
        perror( "close" );
        exit( 1 );
      }      
    } else if ( arg.substr( 0, 3 ) == "if=" ) {
      filename = ( arg.substr( 3 ) );
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

      if ( tree.has_config() ) {
        printf( "Prior assumptions:\n%s\n\n", tree.config().DebugString().c_str() );
      }

      if ( tree.has_optimizer() ) {
        printf( "Remy optimization settings:\n%s\n\n", tree.optimizer().DebugString().c_str() );
      }
  }
 }
  if ( config_filename.empty() ) {
    fprintf( stderr, "Provide an input config protobuf with the cf= argument.\n");
    exit ( 1 );
  }
  
  if ( filename.empty() ) {
    fprintf( stderr, "Provide an input whisker protobuf to score with the if= argument.\n");
    exit ( 1 );
  }
  // use test_config to parse config parameters 
  if ( !( test_config.has_mean_on_duration() && test_config.has_mean_off_duration() && test_config.has_delay() && test_config.has_link_ppt() && test_config.has_num_senders()) ) {
    fprintf( stderr, "Please provide a config file with mean_on_duation, mean_off_duration, delay, link_ppt and num_senders.\n");
    exit ( 1 );
  }
  mean_on_duration = test_config.mean_on_duration();
  mean_off_duration = test_config.mean_off_duration();
  link_ppt = test_config.link_ppt();
  delay = test_config.delay();
  num_senders = test_config.num_senders();

  ConfigRange configuration_range;
  configuration_range.link_packets_per_ms = make_pair( link_ppt, link_ppt ); /* 1 Mbps to 10 Mbps */
  configuration_range.rtt_ms = make_pair( delay, delay ); /* ms */
  configuration_range.num_senders = make_pair( num_senders, num_senders );
  configuration_range.mean_on_duration = make_pair( mean_on_duration, mean_on_duration );
  configuration_range.mean_off_duration = make_pair(mean_off_duration, mean_off_duration );
  configuration_range.lo_only = true;

  Evaluator eval( whiskers, configuration_range );
  auto outcome = eval.score( {}, false, 10 );
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
