#include <limits>
#include <algorithm>

#include "polysender.hh"

using namespace std;

static constexpr double INITIAL_WINDOW = 1.0; /* INITIAL WINDOW OF 1 */

PolynomialSender::PolynomialSender()
  :  _packets_sent( 0 ),
     _packets_received( 0 ),
     _the_window( INITIAL_WINDOW ),
     _flow_id( 0 ),
     _largest_ack( -1 )
{
}

void PolynomialSender::packets_received( const vector< Packet > & packets ) {
  for ( auto & packet : packets ) {
    _largest_ack = max( _largest_ack, packet.seq_num );
    _packets_received++;
    if ( packet.flow_id != _flow_id ) {
      /* This was from the previous flow, ignore it for congestion control */
      continue;
    }
    _the_window = max( INITIAL_WINDOW, _the_window );
  }
}

void PolynomialSender::reset( const double & )
{
  _the_window = INITIAL_WINDOW;
  _flow_id++;
  /* Give up on everything sent so far that hasn't been acked,
     Fixes the problem of tail losses */
  _largest_ack = _packets_sent - 1;
  assert( _flow_id != 0 );
}

double PolynomialSender::next_event_time( const double & tickno ) const
{
  if ( _packets_sent < _largest_ack + 1 + _the_window ) {
    return tickno;
  } else {
    return std::numeric_limits<double>::max();
  }
}
