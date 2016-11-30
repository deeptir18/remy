#include <limits>
#include <algorithm>

#include "lerpsender.hh"

using namespace std;

static constexpr double INITIAL_WINDOW = 100; /* INITIAL WINDOW OF 1 */

LerpSender::LerpSender()
  :  _packets_sent( 0 ),
     _packets_received( 0 ),
     _last_send_time( 0 ),
     _the_window( INITIAL_WINDOW ),
     _intersend_time( 0 ),
     _flow_id( 0 ),
     _memory(),
     _largest_ack( -1 ),
     _x0s( 2 ),
     _x1s( 2 ),
     _x2s( 2 )
{
  _x0s = {0,5};
  _x1s = {0,5};
  _x2s = {0,5};
}

void LerpSender::packets_received( const vector< Packet > & packets ) {
  _packets_received += packets.size();
  _largest_ack = max( packets.at( packets.size() - 1 ).seq_num, _largest_ack );
  _the_window = max( INITIAL_WINDOW, _the_window );
  _memory.packets_received( packets, _flow_id, _largest_ack );

  update_actions( _memory );
  //printf("Updated window to be %f and intersend to be %f\n", _the_window, _intersend_time );
}

void LerpSender::reset( const double & )
{
  _the_window = INITIAL_WINDOW;
  _last_send_time = 0;
  _flow_id++;
  _intersend_time = 0;
  _largest_ack = _packets_sent - 1;
   _memory.reset();
  assert( _flow_id != 0 );
  _x0s = {0,5};
  _x1s = {0,5};
  _x2s = {0,5};
}

double LerpSender::next_event_time( const double & tickno ) const
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

void LerpSender::update_actions( const Memory memory )
{
  double send_ewma = (double)(memory.field( 0 ) );
  double rec_ewma = (double)(memory.field( 1 ) );
  double rtt_ratio = (double)(memory.field( 2 ) );
  vector < double > signals =  {send_ewma, rec_ewma, rtt_ratio};
  
  unsigned int i,j,k;
  for (i=0; i<_x0s.size() && send_ewma < _x0s[i]; i++);
  for (j=0; j<_x1s.size() && rec_ewma < _x1s[j]; j++);
  for (k=0; k<_x2s.size() && rtt_ratio < _x2s[k]; k++);

  double window_increment = interpolate3d( i, j, k, signals, 0 );
  double window_multiplier = interpolate3d( i, j, k, signals, 1 );
  _the_window = min( max( 0, int( _the_window * window_multiplier + window_increment ) ), 1000000 );
  double intersend = interpolate3d( i, j, k, signals, 2 );
  _intersend_time = intersend;
}

double LerpSender::interpolate3d( int x0_min_index, int x1_min_index, int x2_min_index, 
                                  vector <double> signals, int action ) {
  double x0min = _x0s[x0_min_index], x0max = _x0s[x0_min_index+1],
         x1min = _x1s[x1_min_index], x1max = _x1s[x1_min_index+1],
         x2min = _x2s[x2_min_index], x2max = _x2s[x2_min_index+1];

  double x0_max_t = (x0max - signals[0]),
         x0_t_min = (signals[0] - x0min),
         x1_max_t = (x1max - signals[1]),
         x1_t_min = (signals[1] - x1min),
         x2_max_t = (x2max - signals[2]),
         x2_t_min = (signals[2] - x2min);

  return ((((x0_max_t * ((x1_max_t * x2_max_t * _known_points[x0_min_index][x1_min_index][x2_min_index][action]) + 
                       (x1_max_t * x2_t_min * _known_points[x0_min_index][x1_min_index][x2_min_index+1][action]) +
                       (x1_t_min * x2_max_t * _known_points[x0_min_index][x1_min_index+1][x2_min_index][action]) + 
                       (x1_t_min * x2_t_min * _known_points[x0_min_index][x1_min_index+1][x2_min_index+1][action]))) +
           (x0_t_min * (x1_max_t * x2_max_t * _known_points[x0_min_index+1][x1_min_index][x2_min_index][action]) + 
                       (x1_max_t * x2_t_min * _known_points[x0_min_index+1][x1_min_index][x2_min_index+1][action]) +
                       (x1_t_min * x2_max_t * _known_points[x0_min_index+1][x1_min_index+1][x2_min_index][action]) + 
                       (x1_t_min * x2_t_min * _known_points[x0_min_index+1][x1_min_index+1][x2_min_index+1][action]))))
           / 
           ((x0max - x0min) * (x1max - x1min) * (x2max - x2min))
         );
}
