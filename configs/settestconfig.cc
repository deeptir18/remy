#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits>
#include "../protobufs/configrange.pb.h"

using namespace std;

int main(int argc, char *argv[]) {
  string output_filename;

  // Network Config Range parameters
  // Modify the value of these variables
  // To make a protobuf that supports this config range
  double link_ppt = 0;
  double delay = 0;
  uint32_t num_senders = 0;
  double mean_on_duration = 0;
  double mean_off_duration = 0;
  uint32_t bdp_multiplier = numeric_limits<int>::max();
  for (int i = 1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "of=") {
      output_filename = string( arg.substr( 3 ) ) + ".pb";
    } else if ( arg.substr( 0, 5 ) == "link=") {
      link_ppt = atof( arg.substr( 5 ).c_str() );
      fprintf( stderr, "Setting min_link packets per ms to %f\n", link_ppt );
    } else if ( arg.substr( 0, 4 ) == "del=" ) {
      delay = atof( arg.substr( 4 ).c_str() );
      fprintf( stderr, "Setting delay to %f ms\n", delay );
    } else if ( arg.substr( 0, 3 ) == "on=" ) {
      mean_on_duration = atof( arg.substr( 3 ).c_str() );
      fprintf( stderr, "Setting mean_on_duration to %f ms\n", mean_on_duration );
    } else if ( arg.substr( 0, 4 ) == "off=" ) {
      mean_off_duration = atof( arg.substr( 4 ).c_str() );
      fprintf( stderr, "Setting mean_off_duration to %f ms\n", mean_off_duration );
    } else if ( arg.substr( 0, 5 ) == "nsrc=" ) {
      num_senders =  atoi( arg.substr( 5 ).c_str() );
      fprintf( stderr, "Setting num_senders to %d\n", num_senders );
    } else if ( arg.substr( 0, 4 ) == "bdp=" ) {
      bdp_multiplier = atoi( arg.substr( 4 ).c_str() );
      fprintf( stderr, "Setting bdp_multiplier to %d\n", bdp_multiplier);
    }
  }
  if ( ( link_ppt == 0 ) || ( delay == 0 ) || ( num_senders == 0 ) || ( mean_on_duration == 0 ) || ( mean_off_duration == 0 ) ) {
    fprintf( stderr, "Provide link=, del=, nsrc=, on=, and off=  arguments.\n" );
    exit ( 1 );
  } 
  InputConfigRange::NetConfig test_config;

  test_config.set_link_ppt(link_ppt);
  test_config.set_delay(delay);
  test_config.set_num_senders(num_senders);
  test_config.set_mean_on_duration(mean_on_duration);
  test_config.set_mean_off_duration(mean_off_duration);
  test_config.set_bdp_multiplier(bdp_multiplier);
  if ( (bdp_multiplier == std::numeric_limits<int>::max() ) ) {
    fprintf(stderr, "Using infinite buffers. To provide a buffer size a multiple of the bdp, provide a bdp= argument.\n");
  }
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

