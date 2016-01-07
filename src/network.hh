#ifndef NETWORK_HH
#define NETWORK_HH

#include <string>

#include "sendergang.hh"
#include "link.hh"
#include "delay.hh"
#include "receiver.hh"
#include "random.hh"
#include "configrange.pb.h"

class NetConfig
{
public:
  double mean_on_duration, mean_off_duration;
  unsigned int num_senders;
  double link_ppt;
  double delay;
  unsigned int bdp_multiplier;

  NetConfig( void )
    : mean_on_duration( 5000.0 ),
      mean_off_duration( 5000.0 ),
      num_senders( 8 ),
      link_ppt( 1.0 ),
      delay( 150 ),
      bdp_multiplier( std::numeric_limits<unsigned int>::max() )

  {}

  NetConfig & set_link_ppt( const double s_link_ppt ) { link_ppt = s_link_ppt; return *this; }
  NetConfig & set_delay( const double s_delay ) { delay = s_delay; return *this; }
  NetConfig & set_num_senders( const unsigned int n ) { num_senders = n; return *this; }
  NetConfig & set_on_duration( const double & duration ) { mean_on_duration = duration; return *this; }
  NetConfig & set_off_duration( const double & duration ) { mean_off_duration = duration; return *this; }
  NetConfig & set_bdp_multiplier( const unsigned int n ) { bdp_multiplier = n; return *this; }

  InputConfigRange::NetConfig DNA( void ) const
  {
    InputConfigRange::NetConfig ret;
    ret.set_mean_on_duration( mean_on_duration );
    ret.set_mean_off_duration( mean_off_duration );
    ret.set_num_senders( num_senders );
    ret.set_delay( delay );
    ret.set_link_ppt( link_ppt );
    ret.set_bdp_multiplier( bdp_multiplier );   
    return ret;
  }
 
  std::string str( void ) const
  {
    char tmp[ 256 ];
    snprintf( tmp, 256, "mean_on=%f, mean_off=%f, nsrc=%d, link_ppt=%f, delay=%f, bdp_multiplier=%d\n",
	      mean_on_duration, mean_off_duration, num_senders, link_ppt, delay, bdp_multiplier );
    return tmp;
  }
};

template <class SenderType>
class Network
{
private:
  PRNG & _prng;
  SenderGang<SenderType> _senders;
  Link _link;
  Delay _delay;
  Receiver _rec;

  double _tickno;

  void tick( void );

public:
  Network( const SenderType & example_sender, PRNG & s_prng, const NetConfig & config );

  void run_simulation( const double & duration );

  const SenderGang<SenderType> & senders( void ) const { return _senders; }
};

#endif
