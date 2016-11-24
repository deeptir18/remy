#ifndef POLYSENDER_HH
#define POLYSENDER_HH

#include <cassert>
#include <vector>
#include <string>

#include "packet.hh"

class PolynomialSender
{
private:
  unsigned int _packets_sent, _packets_received;

  /* _the_window is the congestion window */
  double _the_window;
  unsigned int _flow_id;

  /* Largest ACK so far */
  int _largest_ack;


public:
  PolynomialSender();

  void packets_received( const std::vector< Packet > & packets );
  void reset( const double & tickno ); /* start new flow */

  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );

  PolynomialSender & operator=( const PolynomialSender & ) { assert( false ); return *this; }

  double next_event_time( const double & tickno ) const;
};

#endif
