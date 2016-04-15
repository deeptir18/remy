#ifndef UTILITY_HH
#define UTILITY_HH

#include <cmath>

class Utility
{
private:
  double _tick_share_sending;
  unsigned int _packets_received;
  double _total_delay;
  double _last_seqnum;
  double _lost_so_far;
  double _total_so_far;

public:
  Utility( void ) : _tick_share_sending( 0 ), _packets_received( 0 ), _total_delay( 0 ), _last_seqnum( -1 ), _lost_so_far( 0 ), _total_so_far( 0 ) {}

  void sending_duration( const double & duration, const unsigned int num_sending ) { _tick_share_sending += duration / double( num_sending ); }
  void packets_received( const std::vector< Packet > & packets ) {
    _packets_received += packets.size();

    for ( auto &x : packets ) {
      assert( x.tick_received >= x.tick_sent );
      double outstanding_pkts = 1;
      if ( x.seq_num > _last_seqnum ) {
        outstanding_pkts = x.seq_num - _last_seqnum;
      }
      _last_seqnum = x.seq_num;
      _lost_so_far += outstanding_pkts - 1; // add in all lost except one that was just received
      _total_so_far += outstanding_pkts; // add all that should have come
      _total_delay += x.tick_received - x.tick_sent;
    }

   
  }

  double average_throughput( void ) const
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

    const double throughput_utility = log2( average_throughput() );
    const double delay_penalty = log2( average_delay() / 100.0 );

    return throughput_utility - delay_penalty;
  }

  double percent_lost( void ) const
  {
    if ( _packets_received == 0 ) {
      return 0.0;
    }
    if ( _total_so_far == 0 ) {
      return 0.0;
    }
    return double( _lost_so_far ) / double ( _total_so_far );
  }

};

#endif
