#ifndef LERPSENDER_HH
#define LERPSENDER_HH

#include <cassert>
#include <vector>
#include <string>
#include "memory.hh"
#include "packet.hh"

using namespace std;

class LerpSender
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
  
  vector<double> _x0s;
  vector<double> _x1s;
  vector<double> _x2s;

  double _known_points[2][2][2][3] = {
    {{{2,1.5,10},  // send_low rec_low rtt_low 
      {0.5,1,1000}}, // send_low rec_low rtt_high 
     {{2,1,100},  // send_low rec_high rtt_low
      {0,0.7,2000}}},// send_low rec_high rtt_high  
    {{{0.5,1,200},  // send_high rec_low rtt_low
      {0,0.6,3000}}, // send_high rec_low rtt_high
     {{-1,1,500},  // send_high rec_high rtt_low 
      {-1,0.5,5000}}} // send_high rec_high rtt_high 
  };

public:
  LerpSender();
  void update_actions( const Memory memory );
  void packets_received( const std::vector< Packet > & packets );
  void reset( const double & tickno ); /* start new flow */

  template <class NextHop>
  void send( const unsigned int id, NextHop & next, const double & tickno );

  LerpSender & operator=( const LerpSender & ) { assert( false ); return *this; }

  double next_event_time( const double & tickno ) const;

  double interpolate3d (int x0_index, int x1_index, int x2_index, std::vector<double> signals, int action);
};

#endif
