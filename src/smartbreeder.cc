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
      auto start_time = chrono::high_resolution_clock::now();
      double new_score = improve_whisker( whisker_to_improve, whiskers, score_to_beat );
      auto end_time = chrono::high_resolution_clock::now();
      chrono::milliseconds time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
      double time_ct = time.count();
      assert( new_score >= score_to_beat );
      if ( new_score == score_to_beat ) {
	      cerr << "Ending search." << endl;
        printf("Took %f milliseconds for score to converge to %f\n", (double)time_ct, score_to_beat);
	      break;
      } else {
	      cerr << "Score jumps from " << score_to_beat << " to " << new_score << endl;
        printf("Took %f milliseconds for score to jump from %f to %f\n", (double)time_ct, score_to_beat, new_score);
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
  vector< Whisker > replacements = get_sorted_replacements ( whisker_to_improve );
  vector< pair < const Whisker&, pair< bool, double > > > scores;
  double carefulness = 1;
  bool trace = false;
  const Evaluator< WhiskerTree > eval( _options.config_range );

  // choosing algorithm: direction map
  unordered_map< Direction, double, boost::hash< Direction > > direction_map {};
  // replacement mapped to pair of ( evaluated, score )
  for ( Whisker& x: replacements ) {
    // function to decide if we even need to try this replacement at all
    if ( evaluate_replacement( x, whisker_to_improve, direction_map ) ) {
      // check if in eval cache
      if ( eval_cache_.find( x ) == eval_cache_.end() ) {
          // replace whisker
          WhiskerTree replaced_tree( tree );
          const bool found_replacement __attribute((unused)) = replaced_tree.replace( x );
          assert (found_replacement);
          Evaluator<WhiskerTree>::Outcome outcome = eval.score_in_parallel( replaced_tree, trace, carefulness );
          double score = outcome.score;
          scores.emplace_back( x, make_pair( true, score ) );
          // if score < score to beat, add to the direction map
          if ( score < score_to_beat ) {
            Direction direction = Direction( whisker_to_improve, x );
            if ( direction_map.find( direction ) == direction_map.end() ) {
              direction_map.insert(make_pair(direction, 1));
            } else {
              double val = direction_map.at(direction);
              direction_map[direction] = val + 1;
            }
          }
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

bool SmartBreeder::evaluate_replacement( Whisker& replacement, Whisker& original, std::unordered_map< Direction, double, boost::hash< Direction > > direction_map )
{
  Direction direction = Direction( original, replacement );
  if ( direction_map.find( direction ) == direction_map.end() ) {
    return true;
  }
  double times_bad = direction_map.at( direction );
  if ( times_bad >= 2 ) {
    return false;
  }
  return true;
}


size_t hash_value( const Direction& direction ) {
    size_t seed = 0;
    boost::hash_combine( seed, direction._intersend );
    boost::hash_combine( seed, direction._window_multiple );
    boost::hash_combine( seed, direction._window_increment );
    return seed;
}

// function to return a sorted list of replacements
vector< Whisker > SmartBreeder::get_sorted_replacements( Whisker& whisker_to_improve )
{
  vector< Whisker > replacements = get_replacements ( whisker_to_improve );
  vector< Whisker > sorted_replacements;
  // now sort them according to direction
  unordered_map< Direction, vector< Whisker >, boost::hash< Direction > > map{};

  for ( Whisker& x: replacements ) {
    Direction direction = Direction( whisker_to_improve, x );
    if ( map.find( direction ) == map.end() ) {
      vector< Whisker > list;
      list.emplace_back( x );
      map.insert(make_pair(direction, list));
    } else {
      vector< Whisker > list = map.at( direction);
      list.emplace_back( x );
      map[direction] = list;
    }
  }
  for ( auto it = map.begin(); it != map.end(); ++it ) {
    vector< Whisker > list =  it->second;
    for ( Whisker& x: list ) {
      sorted_replacements.emplace_back( x );
    }
  }

  return sorted_replacements;
}

