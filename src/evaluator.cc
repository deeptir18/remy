#include <fcntl.h>

#include "configrange.hh"
#include "evaluator.hh"
#include "network.cc"
#include "rat-templates.cc"
#include "fish-templates.cc"
#include "aimd-templates.cc"
#include "polysender-templates.cc"
#include "lerpsender-templates.cc"
template <typename T>
Evaluator< T >::Evaluator( const ConfigRange & range )
  : _prng_seed( global_PRNG()() ), /* freeze the PRNG seed for the life of this Evaluator */
    _tick_count( range.simulation_ticks ),
    _configs()
{
  // specific changes to be able to compare to 100x branch from 2013 paper
  const double steps = 100.0;
  const double link_speed_dynamic_range = range.link_ppt.high / range.link_ppt.low;
  const double multiplier = pow(link_speed_dynamic_range, 1.0 / steps);


  // add configs from every point in the cube of configs
  for (double link_ppt = range.link_ppt.low; link_ppt <= (range.link_ppt.high * (1 + (multiplier-1)/2)); link_ppt *= multiplier) {
    for (double rtt = range.rtt.high; rtt <= range.rtt.high; rtt += range.rtt.incr) {
      for (unsigned int senders = range.num_senders.low; senders <= range.num_senders.high; senders += range.num_senders.incr) {
        for (double on = range.mean_on_duration.low; on <= range.mean_on_duration.high; on += range.mean_on_duration.incr) {
          for (double off = range.mean_off_duration.low; off <= range.mean_off_duration.high; off += range.mean_off_duration.incr) {
            for ( double buffer_size = range.buffer_size.low; buffer_size <= range.buffer_size.high; buffer_size += range.buffer_size.incr) {
              for ( double loss_rate = range.stochastic_loss_rate.low; loss_rate <= range.stochastic_loss_rate.high; loss_rate += range.stochastic_loss_rate.incr) {
                _configs.push_back( NetConfig().set_link_ppt( link_ppt ).set_delay( rtt ).set_num_senders( senders ).set_on_duration( on ).set_off_duration(off).set_buffer_size( buffer_size ).set_stochastic_loss_rate( loss_rate ) );
                if ( range.stochastic_loss_rate.isOne() ) { break; }
              }
              if ( range.buffer_size.isOne() ) { break; }
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
SignalTuple
get_median ( const NetConfig& config,
                 PointGrid grid,
                 PRNG seed,
                 const unsigned int ticks)
{
  Network<SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>,
  SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>> network1( LerpSender( grid ), seed, config );
  network1.run_simulation( ticks );
  SignalTuple median = grid.get_median_signal();
  return median;
}

pair< double, vector < pair< double, double > > >
single_simulation ( const NetConfig& config,
                 PointGrid grid,
                 PRNG seed,
                 const unsigned int ticks)
{
  Network<SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>,
  SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>> network1( LerpSender( grid ), seed, config );
  network1.run_simulation( ticks );
  double score = network1.senders().utility();
  vector< pair< double, double > > throughput_delay = network1.senders().throughputs_delays();
  pair < double, vector < pair < double, double > > > summary = make_pair(score, throughput_delay);
  return summary;
}
template <>
SignalTuple Evaluator< WhiskerTree >::grid_get_median_signal(
						PointGrid & grid,
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;
	// vector for all threads to place their respective median signals
  vector< pair < const NetConfig&, future< SignalTuple > > > median_signals;
  for ( auto &x : configs ) {
		PointGrid test_grid( grid, true); // copies the test grid
    median_signals.emplace_back( x,  async( launch::async, get_median, x, test_grid, run_prng, ticks_to_run ) );
  }
  // now take the avg of the median signal tuples
  double total_send = 0;
  double total_rec = 0;
  double total_ratio = 0;
  int count = 0;
  for ( auto & x: median_signals ) {
    SignalTuple s = x.second.get();
    total_send += CWND_INC( s );
    total_rec += CWND_MULT( s );
    total_ratio += MIN_SEND( s );
    count += 1;
  }
  total_send = total_send/double( count );
  total_rec = total_rec/double(count);
  total_ratio = total_ratio/double(count);
  return make_tuple(total_send, total_rec, total_ratio);
}

template <>
SignalTuple Evaluator< FinTree >::grid_get_median_signal(
						PointGrid & grid,
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;
	// vector for all threads to place their respective config scores
  vector< pair < const NetConfig&, future< pair < double, vector < pair< double, double > > > > > > config_scores;
  /* run simulation */
  for ( auto &x : configs ) {
		PointGrid test_grid( grid, false);
		config_scores.emplace_back( x, async( launch::async, single_simulation, x, test_grid, run_prng, ticks_to_run ));
  }
  for ( auto &x: config_scores ) {
    // later include stuff to modify the whisker to have all the modifications
    NetConfig config = x.first;
    pair< double, vector < pair < double, double > > > result = x.second.get();
    double score = result.first;
    the_outcome.score += score;
    the_outcome.throughputs_delays.emplace_back( config, result.second );
  }
  return make_tuple(1,1,3);
}

template <>
Evaluator< WhiskerTree >::Outcome Evaluator< WhiskerTree >::score_lerp_parallel(
						PointGrid & grid,
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;
	// vector for all threads to place their respective config scores
  vector< pair < const NetConfig&, future< pair < double, vector < pair< double, double > > > > > > config_scores;
  /* run simulation */
  for ( auto &x : configs ) {
		PointGrid test_grid( grid, false);
		//printf("Spawning thread for config %s\n", x.str().c_str() );
		config_scores.emplace_back( x, async( launch::async, single_simulation, x, test_grid, run_prng, ticks_to_run ));
  }
  for ( auto &x: config_scores ) {
    // later include stuff to modify the whisker to have all the modifications
    NetConfig config = x.first;
    pair< double, vector < pair < double, double > > > result = x.second.get();
    double score = result.first;
		//printf("Score from config %s is %f\n", config.str().c_str(), score );
    the_outcome.score += score;
    the_outcome.throughputs_delays.emplace_back( config, result.second );
  }
  return the_outcome;
}

template <>
Evaluator< FinTree >::Outcome Evaluator< FinTree >::score_lerp_parallel(
						PointGrid & grid,
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;
	// vector for all threads to place their respective config scores
  vector< pair < const NetConfig&, future< pair < double, vector < pair< double, double > > > > > > config_scores;
  /* run simulation */
  for ( auto &x : configs ) {
		PointGrid test_grid( grid, false);
		config_scores.emplace_back( x, async( launch::async, single_simulation, x, test_grid, run_prng, ticks_to_run ));
  }
  for ( auto &x: config_scores ) {
    // later include stuff to modify the whisker to have all the modifications
    NetConfig config = x.first;
    pair< double, vector < pair < double, double > > > result = x.second.get();
    double score = result.first;
    the_outcome.score += score;
    the_outcome.throughputs_delays.emplace_back( config, result.second );
  }
  return the_outcome;
}

template <>
Evaluator< WhiskerTree >::Outcome Evaluator< WhiskerTree >::score_lerp(
						PointGrid & grid,
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;

  /* run simulation */
  for ( auto &x : configs ) {
    Network<SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>,
    SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>> network1( LerpSender( grid ), run_prng, x );
    network1.run_simulation( ticks_to_run );
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }
  return the_outcome;
}

template <>
Evaluator< FinTree >::Outcome Evaluator< FinTree >::score_lerp(
						PointGrid & grid,
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;

  /* run simulation */
  for ( auto &x : configs ) {
    Network<SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>,
    SenderGang<LerpSender, TimeSwitchedSender<LerpSender>>> network1( LerpSender( grid ), run_prng, x );
    network1.run_simulation( ticks_to_run );
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }
  return the_outcome;
}

template <>
Evaluator< WhiskerTree >::Outcome Evaluator< WhiskerTree >::score_polynomial(
            const unsigned int prng_seed,
            const vector<NetConfig> & configs,
            const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;

  /* run simulation */
  for ( auto &x : configs ) {
    Network<SenderGang<PolynomialSender, TimeSwitchedSender<PolynomialSender>>,
    SenderGang<PolynomialSender, TimeSwitchedSender<PolynomialSender>>> network1( PolynomialSender(), run_prng, x );
    network1.run_simulation( ticks_to_run );
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }
  return the_outcome;
}

template<>
Evaluator< FinTree >::Outcome Evaluator< FinTree >::score_polynomial(
          const unsigned int prng_seed,
          const vector<NetConfig> & configs,
          const unsigned int ticks_to_run )
{
  PRNG run_prng( prng_seed );
  Evaluator::Outcome the_outcome;

  /*run simulation*/
  for ( auto &x : configs ) {
    Network<SenderGang<PolynomialSender, TimeSwitchedSender<PolynomialSender>>,
      SenderGang<PolynomialSender, TimeSwitchedSender<PolynomialSender>>> network1( PolynomialSender(), run_prng, x );
    network1.run_simulation( ticks_to_run );
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
  }
  return the_outcome;
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
    network1.run_simulation( ticks_to_run );
    
    the_outcome.score += network1.senders().utility();
    the_outcome.throughputs_delays.emplace_back( x, network1.senders().throughputs_delays() );
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
typename Evaluator< T >::Outcome Evaluator< T >::score_polynomial( const double carefulness ) const
{
  return score_polynomial( _prng_seed, _configs, _tick_count * carefulness );
}

template <typename T>
typename Evaluator< T >::Outcome Evaluator< T >::score_lerp( PointGrid & grid, const double carefulness ) const
{
  return score_lerp( grid, _prng_seed, _configs, _tick_count * carefulness );
}

template <typename T>
typename Evaluator< T >::Outcome Evaluator< T >::score_lerp_parallel( PointGrid & grid, const double carefulness ) const
{
  return score_lerp_parallel( grid, _prng_seed, _configs, _tick_count * carefulness );
}

template <typename T>
SignalTuple Evaluator< T >::grid_get_median_signal( PointGrid & grid, const double carefulness ) const
{
  return grid_get_median_signal( grid, _prng_seed, _configs, _tick_count * carefulness );
}

template class Evaluator< WhiskerTree>;
template class Evaluator< FinTree >;
