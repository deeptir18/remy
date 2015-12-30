#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../protobufs/configrange.pb.h"

using namespace std;

int main(int argc, char *argv[]) {
  string output_filename;
  for (int i = 1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "of=") {
      output_filename = string( arg.substr( 3 ) ) + ".pb";
    }
  }

  // Network Config Range parameters
  // Modify the value of these variables
  // To make a protobuf that supports this config range
  double link_ppt = 3.162;
  double delay = 150;
  uint32_t num_senders = 2;
  double mean_on_duration = 1000;
  double mean_off_duration = 1000;
  
  InputConfigRange::NetConfig test_config;

  test_config.set_link_ppt(link_ppt);
  test_config.set_delay(delay);
  test_config.set_num_senders(num_senders);
  test_config.set_mean_on_duration(mean_on_duration);
  test_config.set_mean_off_duration(mean_off_duration);

  if (output_filename.empty()) {
    fprintf( stderr, "Provide of=file_name argument.\n" );
    exit ( 1 );
  }
  char of[ 128 ];
  snprintf( of, 128, "%s", output_filename.c_str());
  int fd = open( of, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR );
  if ( fd < 0 ) {
    perror( "open" );
    exit( 1 );
  }


  if ( not test_config.SerializeToFileDescriptor( fd ) ) {
    fprintf( stderr, "Could not serialize TestConfig parameters.\n" );
    exit( 1 );
  }

  if ( close( fd ) < 0 ) {
    perror( "close" );
    exit( 1 );
  }
  printf( "Wrote config range protobuf to \"%s\"\n", output_filename.c_str() );


  return 0;
}

