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
    //double old_val = score_to_beat;
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
    //if ((( score_to_beat - old_val ) / old_val) < .05 )
    //  break;
  }

  /* Split most used whisker */
  apply_best_split( whiskers, generation );

  /* carefully evaluate what we have vs. the previous best */
  const Evaluator< WhiskerTree > eval2( _options.config_range );
  const auto new_score = eval2.score_in_parallel( whiskers, false, 10 );
  const auto old_score = eval2.score_in_parallel( input_whiskertree, false, 10 );

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
  vector< pair < const Whisker&, pair< bool, double > > > scores;

  const Evaluator< WhiskerTree > eval( _options.config_range );

  std::unordered_map< Direction, std::pair< vector< Whisker >, vector< Whisker > >, boost:: hash< Direction > > bin = get_direction_bins( whisker_to_improve );

  // iterate through direction bins
  int not_evaluated = 0;
  for ( auto it = bin.begin(); it != bin.end(); ++it ) {
    if ( evaluate_whisker_list( tree, score_to_beat, it->second.first, scores, eval ) ) {
      evaluate_whisker_list( tree, score_to_beat, it->second.second, scores, eval );
    } else {
      not_evaluated += int( it->second.second.size() );
    }
  }
  double original_score = score_to_beat;
  int len_evaluated = int( scores.size() );
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
    printf("Did not evaluate %d out of %d whiskers for jump from %f to %f\n", not_evaluated, not_evaluated + len_evaluated, original_score, score_to_beat );
    printf("With score %f, chose %s\n", score_to_beat, whisker_to_improve.str().c_str() );
  return score_to_beat;
}


size_t hash_value( const Direction& direction ) {
    size_t seed = 0;
    boost::hash_combine( seed, direction._intersend );
    boost::hash_combine( seed, direction._window_multiple );
    boost::hash_combine( seed, direction._window_increment );
    return seed;
}

bool
SmartBreeder::evaluate_whisker_list( WhiskerTree &tree, double score_to_beat, vector< Whisker > &replacements, vector< pair < const Whisker&, pair< bool, double > > > &scores, Evaluator< WhiskerTree > eval)
{
  bool ret = true;
  bool trace = false;
  int carefulness = 1;
  for ( Whisker& x: replacements ) {
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
          if ( score < score_to_beat )
            ret = false;
      } else {
        double cached_score = eval_cache_.at( x );
        scores.emplace_back( x, make_pair( false, cached_score ) );
      }
  }
  return ret;
}


unordered_map< Direction, pair< vector< Whisker >, vector< Whisker > >, boost:: hash< Direction > >
SmartBreeder::get_direction_bins( Whisker & whisker_to_improve )
{
  unordered_map< Direction, pair< vector< Whisker >, vector< Whisker > >, boost::hash< Direction > > bin {};
  //vector< Whisker > replacements = get_replacements( whisker_to_improve );
  vector< Whisker  > replacements = whisker_to_improve.next_generation( _whisker_options.optimize_window_increment,
                                             _whisker_options.optimize_window_multiple,
                                             _whisker_options.optimize_intersend );
  unordered_map< Direction, vector< Whisker >, boost::hash< Direction >> map {};

  for ( Whisker& x: replacements ) {
    Direction direction = Direction( whisker_to_improve, x );
    if ( map.find( direction ) == map.end() ) {
      // insert a new vector
      vector< Whisker > list;
      list.emplace_back( x );
      map.insert( make_pair( direction, list ) );
    } else {
      vector< Whisker > list = map.at( direction );
      list.emplace_back( x );
      map[direction] = list;
    }
  }

  // now go through and place into two lists
  for ( auto it = map.begin(); it != map.end(); ++ it ) {
    Direction direction = it->first;
    vector< Whisker > list = it->second;
    sort( list.begin(), list.end(), SortReplacement( whisker_to_improve ) );

    vector< Whisker > first;
    unsigned int count = 0;
    unsigned int len_list = list.size();
    vector< int > indices;
    for ( Whisker& x : list ) {
      if ( ( count == 0 ) || ( count == 1 ) || ( count == len_list - 2) || ( count == len_list - 1 ) ) {
        first.emplace_back( x );
        indices.emplace_back( count );
      }
      count++;
    }

    for ( int x: indices ){
      vector< Whisker >::iterator it = list.begin();
      advance( it, x );
      list.erase( it );
    }

    assert( ( first.size() + list.size() == len_list ) );
    bin.insert( make_pair( direction, make_pair( first, list ) ) );
  }
  return bin;
}




