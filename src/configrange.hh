#ifndef CONFIG_RANGE_HH
#define CONFIG_RANGE_HH

#include "dna.pb.h"
#include "configrange.pb.h"
#include <limits>
class ConfigRange
{
public:
  std::pair< double, double > link_packets_per_ms { 1, 1 };
  std::pair< double, double > rtt_ms { 150, 150 };
  std::pair< unsigned int, unsigned int > num_senders { 1, 16 };
  std::pair< double, double > mean_on_duration { 1000, 1000 };
  std::pair< double, double > mean_off_duration { 1000, 1000 };
  std::pair< unsigned int, unsigned int > bdp_multiplier { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
  bool inf_buffers { true };
  double link_ppt_incr { 0 };
  unsigned int senders_incr { 0 };
  double rtt_incr { 0 };
  double on_incr { 0 };
  double off_incr { 0 };
  unsigned int bdp_incr { 0 };
  InputConfigRange::ConfigRange DNA_Config( void ) const;
  RemyBuffers::ConfigRange DNA( void ) const;
};

#endif  // CONFIG_RANGE_HH
