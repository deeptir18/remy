#include <fcntl.h>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>
#include <boost/functional/hash.hpp>

#include <iostream>
#include "configrange.hh"
#include "evaluator.hh"
#include "network.cc"
#include "rat-templates.cc"
#include "fish-templates.cc"
#include "aimd-templates.cc"

template <typename T>
Evaluator< T >::Evaluator( const ConfigRange & range )
  : _prng_seed( global_PRNG()() ), /* freeze the PRNG seed for the life of this Evaluator */
    _tick_count( range.simulation_ticks ),
    _configs()
{
  // add configs from every point in the cube of configs
  double tcp_senders_arr[5] = { 0, 1, 5, 10, 100 };
  double bdp_arr[4] = { .5, 1, 2, 4 };
  int count = 0;
  // add configs from every point in the cube of configs
  for (double link_ppt = range.link_ppt.low; link_ppt <= range.link_ppt.high; link_ppt += range.link_ppt.incr) {
    for (double rtt = range.rtt.low; rtt <= range.rtt.high; rtt += range.rtt.incr) {
      for (unsigned int senders = range.num_senders.low; senders <= range.num_senders.high; senders += range.num_senders.incr) {
        for (double on = range.mean_on_duration.low; on <= range.mean_on_duration.high; on += range.mean_on_duration.incr) {
          for (double off = range.mean_off_duration.low; off <= range.mean_off_duration.high; off += range.mean_off_duration.incr) {
            for ( double num_tcp_senders: tcp_senders_arr ) {
              for ( double loss_rate = range.stochastic_loss_rate.low; loss_rate <= range.stochastic_loss_rate.high; loss_rate += range.stochastic_loss_rate.incr) {
                // now iterate over .5, 1 2 4 BDP
                vector< double > buffer_sizes;
                double one_bdp = link_ppt * rtt;
                for ( double bdp: bdp_arr ) {
                  buffer_sizes.emplace_back( one_bdp * bdp );
                }
                for ( double buffer_size : buffer_sizes ) {
                  _configs.push_back( NetConfig().set_link_ppt( link_ppt ).set_delay( rtt ).set_num_senders( senders ).set_on_duration( on ).set_off_duration(off).set_buffer_size( buffer_size ).set_stochastic_loss_rate( loss_rate ).set_num_tcp_senders( num_tcp_senders ) );
                  std::cout << "Config # " << count << ": " << _configs.at(_configs.size() - 1).str().c_str() << std::endl;
                }
                if ( range.stochastic_loss_rate.isOne() ) { break; }
              }
            }
            if ( range.mean_off_duration.isOne() ) { break; }
          }
          if ( range.mean_on_duration.isOne() ) { break; }
        }
        if ( range.num_senders.isOne() ) { break; }
      }
      if ( range.rtt.isOne() ) { break; }
    }
    if ( range.link_ppt.isOne() ) { break; }
  }

  std::cout << "Num configs is: " << _configs.size() << std::endl;
}

template <typename T>
ProblemBuffers::Problem Evaluator< T >::_ProblemSettings_DNA( void ) const
{
  ProblemBuffers::Problem ret;

  ProblemBuffers::ProblemSettings settings;
  settings.set_prng_seed( _prng_seed );
  settings.set_tick_count( _tick_count );

  ret.mutable_settings()->CopyFrom( settings );

  for ( auto &x : _configs ) {
    RemyBuffers::NetConfig *config = ret.add_configs();
    *config = x.DNA();
  }

  return ret;
}

template <>
ProblemBuffers::Problem Evaluator< WhiskerTree >::DNA( const WhiskerTree & whiskers ) const
{
  ProblemBuffers::Problem ret = _ProblemSettings_DNA();
  ret.mutable_whiskers()->CopyFrom( whiskers.DNA() );

  return ret;
}

template <>
ProblemBuffers::Problem Evaluator< FinTree >::DNA( const FinTree & fins ) const
{
  ProblemBuffers::Problem ret = _ProblemSettings_DNA();
  ret.mutable_fins()->CopyFrom( fins.DNA() );

  return ret;
}

template <>
Evaluator< WhiskerTree >::Outcome Evaluator< WhiskerTree >::score( WhiskerTree & run_whiskers,
             const unsigned int prng_seed,
             const vector<NetConfig> & configs,
             const bool trace,
             const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );

  run_whiskers.reset_counts();

  /* run tests */
  Evaluator::Outcome the_outcome;
  for ( auto &x : configs ) {
    /* run once */
    Network<SenderGang<Rat, TimeSwitchedSender<Rat>>,
      SenderGang<Rat, TimeSwitchedSender<Rat>>> network1( Rat( run_whiskers, trace ), run_prng, x );
    Network<SenderGang<Rat, TimeSwitchedSender<Rat>>,
      SenderGang<Aimd, TimeSwitchedSender<Aimd>>> network2( Rat( run_whiskers, trace ), Aimd(), run_prng, x);
    if( x.num_tcp_senders == 0 ) {
      network1.run_simulation( ticks_to_run );
      the_outcome.score += network1.senders().utility();
      the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
      //std::cout << "Score from network " << x.str().c_str() << ": " << the_outcome.score << std::endl;
    } else {
      network2.run_simulation( ticks_to_run);
      the_outcome.score += network2.senders().utility();
      the_outcome.throughputs_delays.emplace_back( x, network2.senders().throughputs_delays() );
      //std::cout << "Score from network " << x.str().c_str() << ": " << the_outcome.score << std::endl;
    }
    
  }

  the_outcome.used_actions = run_whiskers;

  return the_outcome;
}

template <>
Evaluator< FinTree >::Outcome Evaluator< FinTree >::score( FinTree & run_fins,
             const unsigned int prng_seed,
             const vector<NetConfig> & configs,
             const bool trace,
             const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  unsigned int fish_prng_seed( run_prng() );

  run_fins.reset_counts();

  /* run tests */
  Evaluator::Outcome the_outcome;
  for ( auto &x : configs ) {
    /* run once */
    Network<SenderGang<Fish, TimeSwitchedSender<Fish>>,
      SenderGang<Fish, TimeSwitchedSender<Fish>>> network1( Fish( run_fins, fish_prng_seed, trace ), run_prng, x );
    network1.run_simulation( ticks_to_run );
    
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }

  the_outcome.used_actions = run_fins;

  return the_outcome;
}

pair< double, vector < pair< double, double > > >
single_simulation ( const NetConfig& config,
                 const WhiskerTree & t,
                 PRNG seed,
                 const bool trace_whisker,
                 const unsigned int ticks)
{
  WhiskerTree tree_copy( t );
  Network<SenderGang<Rat, TimeSwitchedSender<Rat>>,
  SenderGang<Rat, TimeSwitchedSender<Rat>>> network1( Rat( tree_copy, trace_whisker ), seed, config );
  network1.run_simulation( ticks );
  double score = network1.senders().utility();
  vector< pair< double, double > > throughput_delay = network1.senders().throughputs_delays();
  pair < double, vector < pair < double, double > > > summary = make_pair(score, throughput_delay);
  return summary;
}

template <>
Evaluator< WhiskerTree >::Outcome Evaluator< WhiskerTree >::score_in_parallel( WhiskerTree & run_whiskers,
             const unsigned int prng_seed,
             const vector<NetConfig> & configs,
             const bool trace,
             const unsigned int ticks_to_run )
{

  PRNG run_prng( prng_seed );

  run_whiskers.reset_counts();

  // vector that maps the config to the score from the simulation as well as the throughput delay object
  vector< pair < const NetConfig&, future< pair < double, vector < pair< double, double > > > > > > config_scores;
  for ( const auto &x : configs ) {
    config_scores.emplace_back( x, async( launch::async, single_simulation, x, run_whiskers, run_prng, trace, ticks_to_run ));
  }
  Evaluator::Outcome the_outcome;
  // iterate through the config score and update the outcome object
  for ( auto &x: config_scores ) {
    // later include stuff to modify the whisker to have all the modifications
    NetConfig config = x.first;
    pair< double, vector < pair < double, double > > > result = x.second.get();
    double score = result.first;
    the_outcome.score += score;
    the_outcome.throughputs_delays.emplace_back( config, result.second );
  }

  the_outcome.used_actions = run_whiskers;
  return the_outcome;
}

template <>
Evaluator< FinTree >::Outcome Evaluator< FinTree >::score_in_parallel( FinTree & run_fins,
             const unsigned int prng_seed,
             const vector<NetConfig> & configs,
             const bool trace,
             const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  unsigned int fish_prng_seed( run_prng() );

  run_fins.reset_counts();

  /* run tests */
  Evaluator::Outcome the_outcome;
  for ( auto &x : configs ) {
    /* run once */
    Network<SenderGang<Fish, TimeSwitchedSender<Fish>>,
      SenderGang<Fish, TimeSwitchedSender<Fish>>> network1( Fish( run_fins, fish_prng_seed, trace ), run_prng, x );
    network1.run_simulation( ticks_to_run );
    
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }

  the_outcome.used_actions = run_fins;

  return the_outcome;
}

template <>
typename Evaluator< WhiskerTree >::Outcome Evaluator< WhiskerTree >::parse_problem_and_evaluate( const ProblemBuffers::Problem & problem )
{
  vector<NetConfig> configs;
  for ( const auto &x : problem.configs() ) {
    configs.emplace_back( x );
  }

  WhiskerTree run_whiskers = WhiskerTree( problem.whiskers() );

  return Evaluator< WhiskerTree >::score( run_whiskers, problem.settings().prng_seed(),
			   configs, false, problem.settings().tick_count() );
}

template <>
typename Evaluator< FinTree >::Outcome Evaluator< FinTree >::parse_problem_and_evaluate( const ProblemBuffers::Problem & problem )
{
  vector<NetConfig> configs;
  for ( const auto &x : problem.configs() ) {
    configs.emplace_back( x );
  }

  FinTree run_fins = FinTree( problem.fins() );

  return Evaluator< FinTree >::score( run_fins, problem.settings().prng_seed(),
         configs, false, problem.settings().tick_count() );
}

template <typename T>
AnswerBuffers::Outcome Evaluator< T >::Outcome::DNA( void ) const
{
  AnswerBuffers::Outcome ret;

  for ( const auto & run : throughputs_delays ) {
    AnswerBuffers::ThroughputsDelays *tp_del = ret.add_throughputs_delays();
    tp_del->mutable_config()->CopyFrom( run.first.DNA() );

    for ( const auto & x : run.second ) {
      AnswerBuffers::SenderResults *results = tp_del->add_results();
      results->set_throughput( x.first ); 
      results->set_delay( x.second );
    }
  }

  ret.set_score( score );

  return ret;
}

template <typename T>
Evaluator< T >::Outcome::Outcome( const AnswerBuffers::Outcome & dna )
  : score( dna.score() ), throughputs_delays(), used_actions() {
  for ( const auto &x : dna.throughputs_delays() ) {
    vector< pair< double, double > > tp_del;
    for ( const auto &result : x.results() ) {
      tp_del.emplace_back( result.throughput(), result.delay() );
    }

    throughputs_delays.emplace_back( NetConfig( x.config() ), tp_del );
  }
}

template <typename T>
typename Evaluator< T >::Outcome Evaluator< T >::score( T & run_actions,
				     const bool trace, const double carefulness ) const
{
  return score( run_actions, _prng_seed, _configs, trace, _tick_count * carefulness );
}


template <typename T>
typename Evaluator< T >::Outcome Evaluator< T >::score_in_parallel( T & run_actions,
				     const bool trace, const double carefulness ) const
{
  return score_in_parallel( run_actions, _prng_seed, _configs, trace, _tick_count * carefulness );
}

template class Evaluator< WhiskerTree>;
template class Evaluator< FinTree >;
