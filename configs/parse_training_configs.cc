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
// Parses protobuf with training network parameters
// Prints them out
int main(int argc, char *argv[]) {
  string input_filename;
  for (int i = 1; i < argc; i++) {
    string arg( argv[ i ] );
    if ( arg.substr( 0, 3 ) == "if=") {
      input_filename = string( arg.substr( 3 ) );
    }
  }
  if ( input_filename.empty() ) {
    fprintf( stderr, "Provide if=file_name argument to parse.\n" );
  }
  InputConfigRange::ConfigVector training_vector;

  int fd = open( input_filename.c_str(), O_RDONLY );
     
  if ( !training_vector.ParseFromFileDescriptor( fd ) ) {
   fprintf( stderr, "Could not parse %s.\n", input_filename.c_str() );
   exit( 1 );
  }

  if ( close( fd ) < 0 ) {
    perror( "close" );
    exit( 1 );
  }
  
  cout << "size: " << training_vector.config_size();
  // parse and print the training config vectors
  for (int i = 0; i < training_vector.config_size(); i++) {
    const InputConfigRange::NetConfig& config = training_vector.config(i);
    cout << " Link ppt: " << config.link_ppt() << "\n";
    cout << " Delay: " << config.delay() << "\n";
    cout << " Mean on duration: " << config.mean_on_duration() << "\n";
    cout << " Mean off duration: " << config.mean_off_duration() << "\n";
    cout << " Num senders: " << config.num_senders() << "\n";
  }  
  return 0;

}
