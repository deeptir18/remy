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
#include "configrange.hh"

using namespace std;
template <typename T>
void parse_outcome( T & outcome ) 
{
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

  printf( "Rules: %s\n", outcome.used_actions.str().c_str() );
}


int main( int argc, char *argv[] )
{

  RemyBuffers::WhiskerTree tree;
  WhiskerTree whiskers;
  string config_filename;
  RemyBuffers::ConfigRange input_config;
  
  bool included_whisker = false;
  for ( int i = 1; i < argc; i++ ) {
    string arg( argv[i] );
    if ( arg.substr( 0, 3 ) == "if=" ) {
      string filename( arg.substr( 3 ) );
      int fd = open( filename.c_str(), O_RDONLY );
      if ( fd < 0 ) {
        perror( "open" );
        exit( 1 );
      }

      if ( !tree.ParseFromFileDescriptor( fd ) ) {
        fprintf( stderr, "Could not parse %s. \n", filename.c_str() );
        exit( 1 );
      }

      whiskers = WhiskerTree( tree );
      included_whisker = true;
    } else if ( arg.substr( 0, 3 ) == "cf=" ) {
      config_filename = string( arg.substr( 3 ));
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

  if ( !(included_whisker) ) {
    exit( 1 );
  }

  if ( config_filename.empty() ) {
    fprintf( stderr, "An input configuration protobuf must be provided via the cf= option. \n");
    fprintf( stderr, "You can generate one using './configuration'. \n");
    exit ( 1 );
  }

  ConfigRange configuration_range( input_config );


  vector< Whisker > leaves;
  leaves = whiskers.get_leaves( leaves );
  PointGrid grid;
  for ( const Whisker & leaf: leaves ) {
    Memory middle = leaf.get_median_point();
    double send_ewma = (double)(middle.field( 0 ) );
    double rec_ewma = (double)(middle.field( 1 ) );
    double rtt_ratio = (double)(middle.field( 2 ) );
    double slow_rec_ewma = (double)(middle.field( 3 ) );
    SignalTuple signal = make_tuple(send_ewma, rec_ewma, rtt_ratio, slow_rec_ewma);
    ActionTuple action = make_tuple( leaf.window_increment(), leaf.window_multiple(), leaf.intersend() );
    grid.add_point( make_pair( signal, action ) );
  }

  // simply test the interpolation
  LerpSender sender( grid );
  SignalTuple signal = make_tuple(0,0,0,0);
  ActionTuple a = sender.interpolate( signal );
  printf("Maps %s -> %s\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str() );

  Evaluator< WhiskerTree > eval( configuration_range );
  auto outcome = eval.score_lerp( grid, 10 );
  parse_outcome< Evaluator< WhiskerTree >::Outcome > ( outcome );

  return 0;
}


