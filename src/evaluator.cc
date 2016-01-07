#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "configrange.hh"
#include "evaluator.hh"
#include "network.cc"
#include "rat-templates.cc"

Evaluator::Evaluator( const WhiskerTree & s_whiskers, const ConfigRange & range )
  : _prng( global_PRNG()() ), /* freeze the PRNG seed for the life of this Evaluator */
    _whiskers( s_whiskers ),
    _configs()
{
  // add configs from every point in the cube of configs
  for (double link_ppt = range.link_packets_per_ms.first; link_ppt <= range.link_packets_per_ms.second; link_ppt += range.link_ppt_incr) {
    for (double rtt = range.rtt_ms.first; rtt <= range.rtt_ms.second; rtt += range.rtt_incr) {
      for (unsigned int senders = range.num_senders.first; senders <= range.num_senders.second; senders += range.senders_incr) {
        for (double on = range.mean_on_duration.first; on <= range.mean_on_duration.second; on += range.on_incr) {
          for (double off = range.mean_off_duration.first; off <= range.mean_off_duration.second; off += range.off_incr) {
            for (unsigned int bdp = range.bdp_multiplier.first; bdp <= range.bdp_multiplier.second; bdp += range.bdp_incr) {
              _configs.push_back( NetConfig().set_link_ppt( link_ppt ).set_delay( rtt ).set_num_senders( senders ).set_on_duration( on ).set_off_duration(off).set_bdp_multiplier( bdp ) );
              if ( ( range.bdp_multiplier.first == range.bdp_multiplier.second) || (range.bdp_incr == 0) ) { break; }
            }
            if ( ( range.mean_off_duration.first == range.mean_off_duration.second ) || ( range.off_incr == 0 ) ) { break; }
          }
          if ( ( range.mean_on_duration.first == range.mean_on_duration.second ) || ( range.on_incr == 0 ) ) { break; }
        }
        if ( ( range.num_senders.first == range.num_senders.second ) || ( range.senders_incr == 0 ) ) { break; }
      }
      if ( ( range.rtt_ms.first == range.rtt_ms.second ) || ( range.rtt_incr == 0 ) ) { break; }
    }
    if ( ( range.link_packets_per_ms.first == range.link_packets_per_ms.second ) || ( range.link_ppt_incr == 0 ) ) { break; }
  }
  
  
}

Evaluator::Outcome Evaluator::score( WhiskerTree & run_whiskers,
				     const bool trace, const unsigned int carefulness ) const
{
  PRNG run_prng( _prng );

  run_whiskers.reset_counts();

  /* run tests */
  Outcome the_outcome;
  for ( auto &x : _configs ) {
    const double dynamic_tick_count = 1000000.0 / x.link_ppt;

    /* run once */
    Network<Rat> network1( Rat( run_whiskers, trace ), run_prng, x );
    network1.run_simulation( dynamic_tick_count * carefulness );

    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }

  the_outcome.used_whiskers = run_whiskers;

  return the_outcome;
}

Evaluator::Outcome Evaluator::score( const std::vector< Whisker > & replacements,
				     const bool trace, const unsigned int carefulness ) const
{
  PRNG run_prng( _prng );

  WhiskerTree run_whiskers( _whiskers );
  for ( const auto &x : replacements ) {
    assert( run_whiskers.replace( x ) );
  }

  return score( run_whiskers, trace, carefulness );
}

InputConfigRange::ConfigVector Evaluator::WriteConfigs() const {
  InputConfigRange::ConfigVector ret;
  for ( auto &x: _configs ) {
    InputConfigRange::NetConfig *config = ret.add_config();
    *config = x.DNA(); 
  }
  return ret;
}
