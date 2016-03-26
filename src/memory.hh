#ifndef MEMORY_HH
#define MEMORY_HH

#include <vector>
#include <string>

#include "packet.hh"
#include "dna.pb.h"

class Memory {
public:
  typedef double DataType;

private:
  DataType _rec_send_ewma;
  DataType _rec_rec_ewma;
  DataType _rtt_ratio;
  DataType _percent_loss;

  double _last_tick_sent;
  double _last_tick_received;
  double _min_rtt;
  double _num_packets_so_far;
  double _num_lost_so_far;
  double _loss_indicator;
  double _largest_ack;
  double _time_at_last_loss;
  double _rtt_at_last_loss;

public:
  Memory( const std::vector< DataType > & s_data )
    : _rec_send_ewma( s_data.at( 0 ) ),
      _rec_rec_ewma( s_data.at( 1 ) ),
      _rtt_ratio( s_data.at( 2 ) ),
      _percent_loss( s_data.at( 3 ) ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _min_rtt( 0 ),
      _num_packets_so_far( 0 ),
      _num_lost_so_far( 0 ),
      _loss_indicator( 0 ),
      _largest_ack( -1 ),
      _time_at_last_loss( 0 ),
      _rtt_at_last_loss( 0 )
  {}

  Memory()
    : _rec_send_ewma( 0 ),
      _rec_rec_ewma( 0 ),
      _rtt_ratio( 0.0 ),
      _percent_loss( 0.0 ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _min_rtt( 0 ),
      _num_packets_so_far( 0 ),
      _num_lost_so_far( 0 ),
      _loss_indicator( 0 ),
      _largest_ack( -1 ),
      _time_at_last_loss( 0 ),
      _rtt_at_last_loss( 0 )
  {}

  void reset( void ) {
    _rec_send_ewma = _rec_rec_ewma = _rtt_ratio = _percent_loss = _last_tick_sent = _last_tick_received = _min_rtt  = _num_packets_so_far = 0;
    _time_at_last_loss = _rtt_at_last_loss = _loss_indicator = _num_lost_so_far = 0;
    _largest_ack = -1;
  }

  static const unsigned int datasize = 4;

  const DataType & field( unsigned int num ) const { return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : num == 2 ? _rtt_ratio: _percent_loss; }
  DataType & mutable_field( unsigned int num )     { return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : num == 2 ? _rtt_ratio: _percent_loss;  }

  void packet_sent( const Packet & packet __attribute((unused)) ) {}
  void packets_received( const std::vector< Packet > & packets, const unsigned int flow_id );
  void advance_to( const unsigned int tickno __attribute((unused)) ) {}

  std::string str( void ) const;

  bool operator>=( const Memory & other ) const { return (_rec_send_ewma >= other._rec_send_ewma) && (_rec_rec_ewma >= other._rec_rec_ewma) && (_rtt_ratio >= other._rtt_ratio) && (_percent_loss >= other._percent_loss); }
  bool operator<( const Memory & other ) const { return (_rec_send_ewma < other._rec_send_ewma) && (_rec_rec_ewma < other._rec_rec_ewma) && (_rtt_ratio < other._rtt_ratio) && ( _percent_loss < other._percent_loss); }
  bool operator==( const Memory & other ) const { return (_rec_send_ewma == other._rec_send_ewma) && (_rec_rec_ewma == _rec_rec_ewma) && (_rtt_ratio == other._rtt_ratio) && ( _percent_loss == other._percent_loss); }

  RemyBuffers::Memory DNA( void ) const;
  Memory( const bool is_lower_limit, const RemyBuffers::Memory & dna );

  friend size_t hash_value( const Memory & mem );
};

extern const Memory & MAX_MEMORY( void );

#endif
