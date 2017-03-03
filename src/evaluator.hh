#ifndef EVALUATOR_HH
#define EVALUATOR_HH

#include <string>
#include <vector>

#include "random.hh"
#include "whiskertree.hh"
#include "fintree.hh"
#include "network.hh"
#include "problem.pb.h"
#include "answer.pb.h"
#include "lerpsender.hh"

template <typename T>
class Evaluator
{
public:
  class Outcome
  {
  public:
    double score;
    std::vector< std::pair< NetConfig, std::vector< std::pair< double, double > > > > throughputs_delays;
    T used_actions;

    Outcome() : score( 0 ), throughputs_delays(), used_actions() {}

    Outcome( const AnswerBuffers::Outcome & dna );

    AnswerBuffers::Outcome DNA( void ) const;
  };

private:
  const unsigned int _prng_seed;
  unsigned int _tick_count;

  std::vector< NetConfig > _configs;

  ProblemBuffers::Problem _ProblemSettings_DNA ( void ) const;

public:
  Evaluator( const ConfigRange & range );
  
  ProblemBuffers::Problem DNA( const T & actions ) const;

  Outcome score( T & run_actions,
		const bool trace = false,
		const double carefulness = 1) const;

  Outcome score_polynomial(
      const double carefulness = 1) const;

  Outcome score_lerp(
			PointGrid & grid,
      const double carefulness = 1) const;

  static Evaluator::Outcome parse_problem_and_evaluate( const ProblemBuffers::Problem & problem );

  static Outcome score( T & run_actions,
			const unsigned int prng_seed,
			const std::vector<NetConfig> & configs,
			const bool trace,
			const unsigned int ticks_to_run );

  static Outcome score_polynomial(
      const unsigned int prng_seed,
      const std::vector<NetConfig> & configs,
      const unsigned int ticks_to_run );

  static Outcome score_lerp(
			PointGrid & grid,
      const unsigned int prng_seed,
      const std::vector<NetConfig> & configs,
      const unsigned int ticks_to_run );

};

#endif
