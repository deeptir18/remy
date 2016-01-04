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
  // Network Config Range parameters
  // Modify the value of these variables
  // To make a protobuf that supports this config range
  double min_link_ppt = 0;
  double max_link_ppt = 0;
  double min_rtt = 0;
  double max_rtt = 0;
  uint32_t min_senders = 0;
  uint32_t max_senders = 0;
  double mean_on_duration = 0;
  double mean_off_duration = 0;
  bool lo_only = false; 
  uint32_t min_bdp_mult = 0;
  uint32_t max_bdp_mult = 0;
  for (int i = 1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "of=") {
      output_filename = string( arg.substr( 3 ) ) + ".pb";
    } else if ( arg.substr( 0, 9 ) == "min_nsrc=" ) {
      min_senders = atoi( arg.substr( 9 ).c_str() );
      fprintf( stderr, "Setting min_senders to %d\n", min_senders );
    } else if ( arg.substr( 0, 9 ) == "max_nsrc=" ) {
      max_senders = atoi( arg.substr( 9 ).c_str() );
      fprintf( stderr, "Setting max_senders to %d\n", max_senders );
    } else if ( arg.substr( 0, 9 ) == "min_link=" ) {
      min_link_ppt = atof( arg.substr( 9 ).c_str() );
      fprintf( stderr, "Setting min_link packets per ms to %f\n", min_link_ppt );
    } else if ( arg.substr( 0, 9 ) == "max_link=" ) {
      max_link_ppt = atof( arg.substr( 9 ).c_str() );
      fprintf( stderr, "Setting max_link packets per ms to %f\n", max_link_ppt );
    } else if ( arg.substr( 0, 8 ) == "min_rtt=" ) {
      min_rtt = atof( arg.substr( 8 ).c_str() );
      fprintf( stderr, "Setting delay to %f ms\n", min_rtt );
    } else if ( arg.substr( 0, 8 ) == "max_rtt=" ) {
      max_rtt = atof( arg.substr( 8 ).c_str() );
      fprintf( stderr, "Setting delay to %f ms\n", max_rtt );
    } else if ( arg.substr( 0, 3 ) == "on=" ) {
      mean_on_duration = atof( arg.substr( 3 ).c_str() );
      fprintf( stderr, "Setting mean_on_duration to %f ms\n", mean_on_duration );
    } else if ( arg.substr( 0, 4 ) == "off=" ) {
      mean_off_duration = atof( arg.substr( 4 ).c_str() );
      fprintf( stderr, "Setting mean_off_duration to %f ms\n", mean_off_duration );
    } else if ( arg.substr( 0, 8 ) == "min_bdp=" ) {
      min_bdp_mult = atoi( arg.substr( 8 ).c_str() );
      fprintf( stderr, "Setting min_bdp_mult to %d\n", min_bdp_mult );
    } else if ( arg.substr( 0, 8 ) == "max_bdp=" ) {
      max_bdp_mult = atoi( arg.substr( 8 ).c_str() ) ;
      fprintf( stderr, "Setting max_bdp_mult to %d\n", max_bdp_mult );
    }
  }
  
  if ( (min_link_ppt == 0 ) || ( max_link_ppt == 0 ) || ( min_rtt == 0 )  || ( max_rtt == 0 ) || ( min_senders == 0 )||( max_senders == 0 ) || ( mean_off_duration == 0 )|| ( mean_on_duration == 0 ) || ( min_bdp_mult == 0 ) || ( max_bdp_mult == 0 ) ) {
  fprintf( stderr, "Provide min_link=, max_link=, min_rtt=, max_rtt=, min_nsrc=, max_nsrc=, on=, off=, min_bdp=, and max_bdp= arguments.. \n");
  exit( 1 );
}

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
  inputconfig.set_min_bdp_multiplier(min_bdp_mult);
  inputconfig.set_max_bdp_multiplier(max_bdp_mult);
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
