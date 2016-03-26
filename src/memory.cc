#include <boost/functional/hash.hpp>
#include <vector>
#include <cassert>

#include "memory.hh"

using namespace std;

static const double alpha = 1.0 / 8.0;

//static const double slow_alpha = 1.0 / 256.0;

void Memory::packets_received( const vector< Packet > & packets, const unsigned int flow_id )
{
  for ( const auto &x : packets ) {
    if ( x.flow_id != flow_id ) {
      continue;
    }
    const double rtt = x.tick_received - x.tick_sent;
    // reset the loss indicator if we have passed approx 1 RTT
    if (x.tick_received > _time_at_last_loss + _rtt_at_last_loss) {
       _loss_indicator = 0;
    }

    // Does this packet indicate a loss?
    if (x.seq_num > _largest_ack + 1) {
      _loss_indicator = 1;
      _time_at_last_loss = x.tick_received;
      _rtt_at_last_loss = rtt;
      _num_lost_so_far += x.seq_num - _largest_ack; // update the number lost so far
    }

    // update num packets seen so far, with lost packets
    _num_packets_so_far += x.seq_num - _largest_ack;

    // Update the largest ack
    assert(_largest_ack <= x.seq_num);
    _largest_ack = x.seq_num;

    // update percent_loss variable
    _percent_loss = _num_lost_so_far/_num_packets_so_far;
    if ( _last_tick_sent == 0 || _last_tick_received == 0 ) {
      _last_tick_sent = x.tick_sent;
      _last_tick_received = x.tick_received;
      _min_rtt = rtt;
    } else {
      _rec_send_ewma = (1 - alpha) * _rec_send_ewma + alpha * (x.tick_sent - _last_tick_sent);
      _rec_rec_ewma = (1 - alpha) * _rec_rec_ewma + alpha * (x.tick_received - _last_tick_received);
     //_slow_rec_rec_ewma = (1 - slow_alpha) * _slow_rec_rec_ewma + slow_alpha * (x.tick_received - _last_tick_received);

      _last_tick_sent = x.tick_sent;
      _last_tick_received = x.tick_received;

      _min_rtt = min( _min_rtt, rtt );
      _rtt_ratio = double( rtt ) / double( _min_rtt );
      assert( _rtt_ratio >= 1.0 );
    }
  }
}

string Memory::str( void ) const
{
  char tmp[ 256 ];
  snprintf( tmp, 256, "sewma=%f, rewma=%f, rttr=%f, percent_loss=%f", _rec_send_ewma, _rec_rec_ewma, _rtt_ratio, _percent_loss);
  return tmp;
}

const Memory & MAX_MEMORY( void )
{
  static const Memory max_memory( { 163840, 163840, 163840, 163840} );
  return max_memory;
}

RemyBuffers::Memory Memory::DNA( void ) const
{
  RemyBuffers::Memory ret;
  ret.set_rec_send_ewma( _rec_send_ewma );
  ret.set_rec_rec_ewma( _rec_rec_ewma );
  ret.set_rtt_ratio( _rtt_ratio );
  ret.set_percent_loss( _percent_loss );
  return ret;
}

/* If fields are missing in the DNA, we want to wildcard the resulting rule to match anything */
#define get_val_or_default( protobuf, field, limit ) \
  ( (protobuf).has_ ## field() ? (protobuf).field() : (limit) ? 0 : 163840 )

Memory::Memory( const bool is_lower_limit, const RemyBuffers::Memory & dna )
  : _rec_send_ewma( get_val_or_default( dna, rec_send_ewma, is_lower_limit ) ),
    _rec_rec_ewma( get_val_or_default( dna, rec_rec_ewma, is_lower_limit ) ),
    _rtt_ratio( get_val_or_default( dna, rtt_ratio, is_lower_limit ) ),
    _percent_loss( get_val_or_default( dna, percent_loss, is_lower_limit ) ),
    _last_tick_sent( 0 ),
    _last_tick_received( 0 ),
    _min_rtt( 0 ),
    _num_packets_so_far( 0 ),
    _num_lost_so_far( 0 ),
    _loss_indicator( 0 ),
    _largest_ack( -1 ),
    _time_at_last_loss( 0 ),
    _rtt_at_last_loss( 0 )
{
}

size_t hash_value( const Memory & mem )
{
  size_t seed = 0;
  boost::hash_combine( seed, mem._rec_send_ewma );
  boost::hash_combine( seed, mem._rec_rec_ewma );
  boost::hash_combine( seed, mem._rtt_ratio );
  boost::hash_combine( seed, mem._percent_loss );
  return seed;
}
