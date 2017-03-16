#include <cassert>
#include <utility>

#include "lerpsender.hh"

using namespace std;

  template <class NextHop>
void LerpSender::send( const unsigned int id, NextHop & next, const double & tickno )
{
  assert( int(_packets_sent) >= _largest_ack + 1 );
	if ( _the_window == 0 ) {
		update_actions( _memory );
	}
  if ( ( int(_packets_sent) < _largest_ack + 1 + _the_window ) and ( _last_send_time + _intersend_time <= tickno ) ) {
    Packet p( id, _flow_id, tickno, _packets_sent );
    _packets_sent++;
    next.accept( p, tickno );
    _last_send_time = tickno;
  }
}
