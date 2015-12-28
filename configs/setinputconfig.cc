#include "setinputconfig.hh"
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
  double min_link_ppt = .1;
  double max_link_ppt = 100;
  double min_rtt = 150;
  double max_rtt = 150;
  uint32_t min_senders = 2;
  uint32_t max_senders = 2;
  double mean_on_duration = 1000;
  double mean_off_duration = 1000;
  bool lo_only = false;

  // Creates config range object 
  InputConfigRange::ConfigRange inputconfig;
  inputconfig.set_min_link_ppt(min_link_ppt);
  inputconfig.set_max_link_ppt(max_link_ppt);
  inputconfig.set_min_rtt(min_rtt);
  inputconfig.set_max_rtt(max_rtt);
  inputconfig.set_min_senders(min_senders);
  inputconfig.set_max_senders(max_senders);
  inputconfig.set_mean_on_duration(mean_on_duration);
  inputconfig.set_mean_off_duration(mean_off_duration);
  inputconfig.set_lo_only(lo_only);

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
  

  if ( not inputconfig.SerializeToFileDescriptor( fd ) ) {
    fprintf( stderr, "Could not serialize InputConfig parameters.\n" );
    exit( 1 );
  }

  if ( close( fd ) < 0 ) {
    perror( "close" );
    exit( 1 );
  }
  printf( "Wrote config range protobuf to \"%s\"\n", output_filename.c_str() );
  return 0;
}
