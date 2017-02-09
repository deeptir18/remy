#include <iostream>

#include "smartbreeder.hh"

using namespace std;

Evaluator< WhiskerTree >::Outcome SmartBreeder::improve( WhiskerTree & whiskers )
{
  /* back up the original whiskertree */
  /* this is to ensure we don't regress */
  WhiskerTree input_whiskertree( whiskers );

  /* evaluate the whiskers we have */
  whiskers.reset_generation();
  unsigned int generation = 0;

  while ( generation < 5 ) {
    const Evaluator< WhiskerTree > eval( _options.config_range );

    auto outcome( eval.score( whiskers ) );

    /* is there a whisker at this generation that we can improve? */
    auto most_used_whisker_ptr = outcome.used_actions.most_used( generation );

    /* if not, increase generation and promote all whiskers */
    if ( !most_used_whisker_ptr ) {
      generation++;
      whiskers.promote( generation );

      continue;
    }

    Whisker whisker_to_improve = *most_used_whisker_ptr;

    double score_to_beat = outcome.score;

    while ( 1 ) {

      double new_score = improve_whisker( whisker_to_improve, whiskers, score_to_beat );
      assert( new_score >= score_to_beat );
      if ( new_score == score_to_beat ) {
	cerr << "Ending search." << endl;
	break;
      } else {
	      cerr << "Score jumps from " << score_to_beat << " to " << new_score << endl;
	      score_to_beat = new_score;
      }
    }

    whisker_to_improve.demote( generation + 1 );

    const auto result __attribute((unused)) = whiskers.replace( whisker_to_improve );
    assert( result );
  }

  /* Split most used whisker */
  apply_best_split( whiskers, generation );

  /* carefully evaluate what we have vs. the previous best */
  const Evaluator< WhiskerTree > eval2( _options.config_range );
  const auto new_score = eval2.score( whiskers, false, 10 );
  const auto old_score = eval2.score( input_whiskertree, false, 10 );

  if ( old_score.score >= new_score.score ) {
    fprintf( stderr, "Regression, old=%f, new=%f\n", old_score.score, new_score.score );
    whiskers = input_whiskertree;
    return old_score;
  }

  return new_score;
}

vector< Whisker > SmartBreeder::get_replacements( Whisker & whisker_to_improve ) 
{
  return whisker_to_improve.next_generation( _whisker_options.optimize_window_increment,
                                             _whisker_options.optimize_window_multiple,
                                             _whisker_options.optimize_intersend );
}

double
SmartBreeder::improve_whisker( Whisker & whisker_to_improve, WhiskerTree & tree, double score_to_beat )
{
  // evaluates replacements in sequence in a smart way
  vector< Whisker > replacements = get_replacements ( whisker_to_improve );
  vector< pair < const Whisker&, pair< bool, double > > > scores;
  double carefulness = 1;
  bool trace = false;
  const Evaluator< WhiskerTree > eval( _options.config_range );

  // replacement mapped to pair of ( evaluated, score )
  for ( Whisker& x: replacements ) {
    // function to decide if we even need to try this replacement at all
    if ( evaluate_replacement( x ) ) {
      // check if in eval cache
      if ( eval_cache_.find( x ) == eval_cache_.end() ) {
          // replace whisker
          WhiskerTree replaced_tree( tree );
          const bool found_replacement __attribute((unused)) = replaced_tree.replace( x );
          assert (found_replacement);
          Evaluator<WhiskerTree>::Outcome outcome = eval.score_in_parallel( replaced_tree, trace, carefulness );
          double score = outcome.score;
          scores.emplace_back( x, make_pair( true, score ) );
      } else {
        double cached_score = eval_cache_.at( x );
        scores.emplace_back( x, make_pair( false, cached_score ) );
      }
    }
  }
  // iterate to find the best replacement
  for ( auto & x: scores ) {
    const Whisker& replacement( x.first );
    const auto result( x.second );
    const bool was_new_evaluation( result.first );
    const double score( result.second );

    /* should we cache this result?*/
    if ( was_new_evaluation ) {
      eval_cache_.insert( make_pair( replacement, score ) );
    }
    if ( score > score_to_beat ) {
      score_to_beat = score;
      whisker_to_improve = replacement; // replaces object in memory
    }

  }
    printf("With score %f, chose %s\n", score_to_beat, whisker_to_improve.str().c_str() );
  return score_to_beat;
}

bool SmartBreeder::evaluate_replacement( Whisker& replacement )
{
  if ( replacement.intersend() > 0 ) {
    return true;
  }
  return true;
}
