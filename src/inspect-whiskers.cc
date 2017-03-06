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
	RemyBuffers::ConfigRange input_config;
	bool whisker_input = false;
	for ( int i = 1; i < argc; i ++ ) {
		string arg( argv[i] );
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
			whisker_input = true;

		}
	}

	if ( whisker_input ) {
    printf( "whiskers: %s\n", whiskers.str().c_str() );
	}
}
