#ifndef POLYSENDER_HH
#define POLYSENDER_HH

#include <cassert>
#include <vector>
#include <string>
#include "memory.hh"
#include "packet.hh"
#include "poly.hh"

class PolynomialSender
{
private:
  unsigned int _packets_sent, _packets_received;
  double _last_send_time;
  /* _the_window is the congestion window */
  double _the_window;
  double _intersend_time;
  unsigned int _flow_id;
  Memory _memory;

  /* Largest ACK so far */
  int _largest_ack;

  // hardcode a polynomial mapping for each memory congestion signal we care about
  Polynomial _window_increment_poly;
  Polynomial _window_multiplier_poly;
  Polynomial _intersend_rate_poly;
  /*PolynomialMapping _send_ewma_poly;
  PolynomialMapping _rec_ewma_poly;
  PolynomialMapping _rtt_ratio_poly;*/

public:
  PolynomialSender();
  void update_window( const Memory memory );
  void update_intersend( const Memory memory );
  void packets_received( const std::vector< Packet > & packets );
  void reset( const double & tickno ); /* start new flow */

  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );

  PolynomialSender & operator=( const PolynomialSender & ) { assert( false ); return *this; }

  double next_event_time( const double & tickno ) const;
};

#endif
