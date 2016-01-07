#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include "../protobufs/configrange.pb.h"
using namespace std;

// Parses input arguments, create double range protobuf
InputConfigRange::DoubleRange set_double_range_protobuf(int argc, char *argv[], string arg_name) {
  string min_string = "min_" + arg_name + "=";
  string max_string = "max_" + arg_name + "=";
  string incr_string = arg_name + "_incr=";
  int min_size = min_string.size();
  int max_size = max_string.size();
  int incr_size = incr_string.size();
  double min = -1;
  double max = -1;
  double incr = 0;
  for (int i=1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, min_size ) == min_string ) {
     min = atof( arg.substr( min_size ).c_str() );
     fprintf( stderr, "Setting min of %s to %f\n", arg_name.c_str(), min );
    } else if ( arg.substr( 0, max_size ) == max_string ) {
     max = atof( arg.substr( max_size ).c_str() );
     fprintf( stderr, "Setting max of %s to %f\n", arg_name.c_str(), max );
    } else if ( arg.substr( 0, incr_size ) == incr_string ) {
     incr = atof ( arg.substr( incr_size ).c_str() );
     fprintf( stderr, "Setting incr of %s to %f\n", arg_name.c_str(), incr );
    }
  }    

  if ( (min == -1) || (max == -1) ) {
    fprintf( stderr, "Please provide min, max and incr for %s with %s, %s, %s\n", arg_name.c_str(), min_string.c_str(), max_string.c_str(), incr_string.c_str());
    exit(1);
  }
    
  if ( ( min != max ) && ( incr == 0 ) ) {
    fprintf( stderr, "Provide non-zero incr for %s\n", arg_name.c_str());
    exit(1);
  }
    
  if ( (incr > (max-min)) || (min>max) ) {
    fprintf( stderr, "Provide valid min and max and incr for %s\n.", arg_name.c_str());
    exit(1);
  }

  InputConfigRange::DoubleRange range;
  range.set_lo(min);
  range.set_hi(max);
  range.set_incr(incr);
  return range;
}

// Parses command line arguments, creates int range protobuf
InputConfigRange::IntRange set_int_range_protobuf(int argc, char *argv[], string arg_name) {
  string min_string = "min_" + arg_name + "=";
  string max_string = "max_" + arg_name + "=";
  string incr_string = arg_name + "_incr=";
  uint32_t min = 0;
  uint32_t max = 0;
  uint32_t incr = 0;
  int min_size = min_string.size();
  int max_size = max_string.size();
  int incr_size = incr_string.size();
  for (int i=1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, min_size ) == min_string ) {
     min = atof( arg.substr( min_size ).c_str() );
     fprintf( stderr, "Setting min of %s to %d\n", arg_name.c_str(), min );
    } else if ( arg.substr( 0, max_size ) == max_string ) {
     max = atof( arg.substr( max_size ).c_str() );
     fprintf( stderr, "Setting max of %s to %d\n", arg_name.c_str(), max );
    } else if ( arg.substr( 0, incr_size ) == incr_string ) {
     incr = atof ( arg.substr( incr_size ).c_str() );
     fprintf( stderr, "Setting incr of %s to %d\n", arg_name.c_str(), incr );
    }
  }

  if ( (min == 0) || (max == 0) ) {
    fprintf( stderr, "Please provide min, max and incr for %s with %s, %s, %s\n", arg_name.c_str(), min_string.c_str(), max_string.c_str(), incr_string.c_str());
    exit(1);
  }
    
  if ( ( min != max ) && ( incr == 0 ) ) {
    fprintf( stderr, "Provide non-zero incr for %s\n", arg_name.c_str());
    exit(1);
  }
    
  if ( (incr > (max-min)) || (min>max) ) {
    fprintf( stderr, "Provide valid min and max and incr for %s\n", arg_name.c_str());
    exit(1);
  }

  InputConfigRange::IntRange range;
  range.set_lo(min);
  range.set_hi(max);
  range.set_incr(incr);
  return range;
}


int main(int argc, char *argv[]) {
  string output_filename;  
  bool infinite_buffers = false;
  for (int i = 1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "of=") {
      output_filename = string( arg.substr( 3 ) ) + ".pb";
    }
    if ( arg == "inf_buffers") {
      infinite_buffers = true;
    } 
  }
  
  if (output_filename.empty()) {
    fprintf( stderr, "Provide of=file_name argument\n" );
    exit ( 1 );
  }
  // parse all other config range parameters
  InputConfigRange::DoubleRange link_ppt = set_double_range_protobuf(argc, argv, "link_ppt");
  InputConfigRange::DoubleRange delay = set_double_range_protobuf(argc, argv, "rtt");
  InputConfigRange::DoubleRange mean_on_duration = set_double_range_protobuf(argc, argv, "on");
  InputConfigRange::DoubleRange mean_off_duration = set_double_range_protobuf(argc, argv, "off");
  InputConfigRange::IntRange num_senders = set_int_range_protobuf(argc, argv, "nsrc");
  

  // set input config object
  InputConfigRange::ConfigRange input_config;
  input_config.mutable_link_ppt()->CopyFrom(link_ppt);
  input_config.mutable_delay()->CopyFrom(delay);
  input_config.mutable_mean_on_duration()->CopyFrom(mean_on_duration);
  input_config.mutable_mean_off_duration()->CopyFrom(mean_off_duration);
  input_config.mutable_num_senders()->CopyFrom(num_senders);

  InputConfigRange::IntRange bdp_multiplier;
  // if not infinite buffers, check for buffer multiplier
  if ( !(infinite_buffers) ) {
    bdp_multiplier = set_int_range_protobuf(argc, argv, "bdp");
    input_config.mutable_bdp_multiplier()->CopyFrom(bdp_multiplier);
  } else {
    // set default as infinite buffers
    bdp_multiplier.set_lo(std::numeric_limits<unsigned int>::max());
    bdp_multiplier.set_hi(std::numeric_limits<unsigned int>::max());
    bdp_multiplier.set_incr(0);
    input_config.mutable_bdp_multiplier()->CopyFrom(bdp_multiplier);
  }

  char of[ 128 ];
  snprintf( of, 128, "%s", output_filename.c_str());
  int fd = open( of, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR );
  if ( fd < 0 ) {
    perror( "open" );
    exit( 1 );
  }
  

  if ( not input_config.SerializeToFileDescriptor( fd ) ) {
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
