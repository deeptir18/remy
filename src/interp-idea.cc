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
int main( int argc, char *argv[] )
{

  RemyBuffers::WhiskerTree tree;
  WhiskerTree whiskers;
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
    }
  }

  if ( !(included_whisker) ) {
    exit( 1 );
  }

  vector< Whisker > leaves;
  leaves = whiskers.get_leaves( leaves );

  for ( const Whisker & leaf: leaves ) {
    Memory middle = leaf.get_median_point();
    double send_ewma = (double)(middle.field( 0 ) );
    double rec_ewma = (double)(middle.field( 1 ) );
    double rtt_ratio = (double)(middle.field( 2 ) );
    double slow_rec_ewma = (double)(middle.field( 3 ) );
    NewSignalTuple signal = make_tuple(send_ewma, rec_ewma, rtt_ratio, slow_rec_ewma);
    ActionTuple action = make_tuple( leaf.window_increment(), leaf.window_multiple(), leaf.intersend() );
    printf("%s->%s\n", _stuple_str( signal ).c_str(), _atuple_str( action ).c_str() );
  }
  return 0;
}


