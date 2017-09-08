#ifndef UTILITY_HH
#define UTILITY_HH

#include <cmath>
#include <cassert>
#include <climits>
#include "simulationresults.pb.h"
static const double alpha = 1.0/8.0;
class Utility
{
private:
  double _tick_share_sending;
  unsigned int _packets_received;
  double _total_delay;
  double _last_seqnum;

public:
  Utility( void ) : _tick_share_sending( 0 ), _packets_received( 0 ), _total_delay( 0 ), _last_seqnum( - 1 ) {}

  void sending_duration( const double & duration, const unsigned int num_sending ) { _tick_share_sending += duration / double( num_sending ); }
  void packets_received( const std::vector< Packet > & packets ) {
    _packets_received += packets.size();
    // maintain an srtt estimate
    double srtt = 0;
    for ( auto &x : packets ) {
      assert( x.tick_received >= x.tick_sent );
      double rtt = x.tick_received - x.tick_sent;
      double unpenalized_rtt = rtt;
      srtt = ( srtt == 0 ) ? rtt : ( 1 - alpha ) * srtt + alpha * unpenalized_rtt; // maintain smoothed rtt estimate
      double pkt_outstanding = ( x.seq_num > _last_seqnum ) ? ( x.seq_num - _last_seqnum) : 1; // how many packets lost?
      _last_seqnum = x.seq_num; // assuming no reordering
      _total_delay += ( rtt ) + srtt * ( pkt_outstanding - 1 ); // add additional srtt if lost
    }
  }

  /* returns throughput normalized to equal share of link */
  double average_throughput_normalized_to_equal_share( void ) const
  {
    if ( _tick_share_sending == 0 ) {
      return 0.0;
    }
    return double( _packets_received ) / _tick_share_sending;
  }

  double average_delay( void ) const
  {
    if ( _packets_received == 0 ) {
      return 0.0;
    }
    return double( _total_delay ) / double( _packets_received );
  }

  double utility( void ) const
  {
    if ( _tick_share_sending == 0 ) {
      return 0.0;
    }

    if ( _packets_received == 0 ) {
      return -INT_MAX;
    }

    const double throughput_utility = log2( average_throughput_normalized_to_equal_share() );
    const double delay_penalty = log2( average_delay() / 100.0 );

    return throughput_utility - delay_penalty;
  }

  SimulationResultBuffers::UtilityData DNA() const {
    SimulationResultBuffers::UtilityData ret;
    ret.set_sending_duration( _tick_share_sending );
    ret.set_packets_received( _packets_received );
    ret.set_total_delay( _total_delay );
    return ret;
  }

};

#endif
