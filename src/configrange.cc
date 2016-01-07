#include "configrange.hh"

using namespace std;

static RemyBuffers::Range pair_to_range( const pair< double, double > & p )
{
  RemyBuffers::Range ret;
  ret.set_low( p.first );
  ret.set_high( p.second );
  return ret;
}

static InputConfigRange::DoubleRange pair_to_double_range( double lo, double hi, double incr ) 
{
  InputConfigRange::DoubleRange ret;
  ret.set_lo(lo);
  ret.set_hi(hi);
  ret.set_incr(incr);
  return ret;
}

static InputConfigRange::IntRange pair_to_int_range( unsigned int lo, unsigned int hi, unsigned int incr ) 
{
  InputConfigRange::IntRange ret;
  ret.set_lo(lo);
  ret.set_hi(hi);
  ret.set_incr(incr);
  return ret;
}
RemyBuffers::ConfigRange ConfigRange::DNA( void ) const

{
  RemyBuffers::ConfigRange ret;
  ret.mutable_link_packets_per_ms()->CopyFrom( pair_to_range( link_packets_per_ms ) );
  ret.mutable_rtt()->CopyFrom( pair_to_range( rtt_ms ) );
  ret.mutable_num_senders()->CopyFrom( pair_to_range( num_senders ) );
  ret.mutable_bdp_multiplier()->CopyFrom( pair_to_range( bdp_multiplier ) );
  ret.mutable_mean_off_duration()->CopyFrom( pair_to_range( mean_on_duration ) );
  ret.mutable_mean_on_duration()->CopyFrom( pair_to_range( mean_off_duration) );
  ret.set_inf_buffers(inf_buffers);
  return ret;
}

InputConfigRange::ConfigRange ConfigRange::DNA_Config( void ) const

{
  InputConfigRange::ConfigRange ret;
  ret.mutable_link_ppt()->CopyFrom( pair_to_double_range( link_packets_per_ms.first, link_packets_per_ms.second, link_ppt_incr ) );
  ret.mutable_delay()->CopyFrom( pair_to_double_range( rtt_ms.first, rtt_ms.second, rtt_incr ) );
  ret.mutable_num_senders()->CopyFrom( pair_to_int_range( num_senders.first, num_senders.second, senders_incr ) );
  ret.mutable_mean_on_duration()->CopyFrom( pair_to_double_range( mean_on_duration.first, mean_on_duration.second, on_incr ) );
  ret.mutable_mean_off_duration()->CopyFrom( pair_to_double_range( mean_off_duration.first, mean_off_duration.second, off_incr ) );
  ret.mutable_bdp_multiplier()->CopyFrom( pair_to_int_range( bdp_multiplier.first, bdp_multiplier.second, bdp_incr ) );
  ret.set_inf_buffers(inf_buffers);
  return ret;
}
