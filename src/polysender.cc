#include <limits>
#include <algorithm>

#include "polysender.hh"

using namespace std;

static constexpr double INITIAL_WINDOW = 100; /* INITIAL WINDOW OF 1 */

PolynomialSender::PolynomialSender()
  :  _packets_sent( 0 ),
     _packets_received( 0 ),
     _last_send_time( 0 ),
     _the_window( INITIAL_WINDOW ),
     _intersend_time( 0 ),
     _flow_id( 0 ),
     _memory(),
     _largest_ack( -1 ),
     _send_ewma_poly( PolynomialMapping() ),
     _rec_ewma_poly( PolynomialMapping() ),
     _rtt_ratio_poly( PolynomialMapping() )
{
  // initial values for each polynomial
  // ordererd: increment, intersend, multiplier
  _send_ewma_poly.initialize_vals(20, 1, .02);
  _rec_ewma_poly.initialize_vals(30, 1, .03);
  _rtt_ratio_poly.initialize_vals(40, 1, .04);
}

void PolynomialSender::packets_received( const vector< Packet > & packets ) {
  _packets_received += packets.size();
  _largest_ack = max( packets.at( packets.size() - 1 ).seq_num, _largest_ack );
  _the_window = max( INITIAL_WINDOW, _the_window );
  _memory.packets_received( packets, _flow_id, _largest_ack );
  // update the window and intersend time based on a polynomial function
  update_window( _memory );
  update_intersend( _memory );
  //printf("Updated window to be %f and intersend to be %f\n", _the_window, _intersend_time );
}

void PolynomialSender::reset( const double & )
{
  _the_window = INITIAL_WINDOW;
  _last_send_time = 0;
  _flow_id++;
  _intersend_time = 0;
  _largest_ack = _packets_sent - 1;
   _memory.reset();
  assert( _flow_id != 0 );

  // reset polynomial values
  _send_ewma_poly.initialize_vals(20, .5, .02);
  _rec_ewma_poly.initialize_vals(30, .5, .03);
  _rtt_ratio_poly.initialize_vals(40, .5, .04);
}

double PolynomialSender::next_event_time( const double & tickno ) const
{
  if ( _packets_sent < _largest_ack + 1 + _the_window ) {
    if ( _last_send_time + _intersend_time <= tickno ) {
      return tickno;
    } else {
      return _last_send_time + _intersend_time;
    }
    return tickno;
  } else {
    return std::numeric_limits<double>::max();
  }
}

void PolynomialSender::update_window( const Memory memory )
{
  // calculate window multiplier and increment
  double rec_ewma = (double)(memory.field( 1 ) );
  double send_ewma = (double)(memory.field( 0 ) );
  double rtt_ratio = (double)(memory.field( 2 ) );
  double window_increment = _send_ewma_poly.get_window_increment( send_ewma ) + _rec_ewma_poly.get_window_increment( rec_ewma ) + _rtt_ratio_poly.get_window_increment( rtt_ratio );

  double window_multiplier = _send_ewma_poly.get_window_multiplier( send_ewma ) + _rec_ewma_poly.get_window_multiplier( rec_ewma ) + _rtt_ratio_poly.get_window_multiplier( rtt_ratio );

  _the_window = min( max( 0, int( _the_window * window_multiplier + window_increment ) ), 1000000 );
}

void PolynomialSender::update_intersend( const Memory memory )
{
  double rec_ewma = (double)(memory.field( 1 ) );
  double send_ewma = (double)(memory.field( 0 ) );
  double rtt_ratio = (double)(memory.field( 2 ) );

  double intersend = _send_ewma_poly.get_intersend( send_ewma ) + _rec_ewma_poly.get_intersend( rec_ewma ) + _rtt_ratio_poly.get_intersend( rtt_ratio );
  _intersend_time = intersend;

}
