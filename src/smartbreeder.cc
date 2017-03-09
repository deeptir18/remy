#include <iostream>

#include "smartbreeder.hh"
#include <ctime>
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
		printf("-----In Generation While loop with gen: %d-----------\n", generation);
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
      	printf("--------------------------------------\n");
	      break;
      } else {
	      cerr << "Score jumps from " << score_to_beat << " to " << new_score << endl;
        printf("Took %f milliseconds for score to jump from %f to %f\n", (double)time_ct, score_to_beat, new_score);
	      score_to_beat = new_score;
      }
      chrono::time_point<chrono::system_clock> time_point;
      time_point = chrono::system_clock::now();

      time_t ttp = chrono::system_clock::to_time_t(time_point);
      cout << "time: " << ctime(&ttp);
      printf("--------------------------------------\n");
    }

    whisker_to_improve.demote( generation + 1 );

    const auto result __attribute((unused)) = whiskers.replace( whisker_to_improve );
    assert( result );
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

vector< Whisker > SmartBreeder::get_initial_replacements( Whisker & whisker_to_improve )
{
  // first get increment alternates, then add the other two actions	
  vector< Whisker > ret =  whisker_to_improve.next_generation( true, false, false );
  vector< Whisker > mult = whisker_to_improve.next_generation( false, true, false );
  vector< Whisker > intersend = whisker_to_improve.next_generation( false, false, true );
  ret.insert( ret.end(), mult.begin(), mult.end() );
  ret.insert( ret.end(), intersend.begin(), intersend.end() );
  return ret;
}

double
SmartBreeder::improve_whisker( Whisker & whisker_to_improve, WhiskerTree & tree, double score_to_beat )
{
  // evaluates replacements in sequence in a smart way
  vector< pair < const Whisker&, pair< bool, double > > > scores;

  const Evaluator< WhiskerTree > eval( _options.config_range );

  unordered_map< Direction, vector< Whisker >, boost:: hash< Direction > > bin = get_direction_bins( whisker_to_improve );
  vector< Direction > coordinates;

  for ( int i=0; i < 3; i++ ) {
    Direction plus = Direction(EQUALS, EQUALS, EQUALS);
    Direction minus = Direction(EQUALS, EQUALS, EQUALS);

    plus.replace( i, PLUS );
    minus.replace( i, MINUS );

    coordinates.emplace_back( plus );
    coordinates.emplace_back( minus );
  }
	unordered_map< Direction, double, boost:: hash< Direction > > direction_results;
  // first evaluate initial 6 directions
  Direction best_dir = Direction( EQUALS, EQUALS, EQUALS );
  double best_score_change = 0;
  for ( Direction& dir: coordinates ) {
    if ( bin.find( dir ) != bin.end() ) {
      double change = ( evaluate_whisker_list( tree, score_to_beat, bin.at( dir ), scores, eval ) );
      if ( change > best_score_change ) {
        best_dir = dir;
				best_score_change = change;
      }
			direction_results.insert( make_pair( dir, change ) );
    }
  }
	vector< Whisker > further_replacements =  get_further_replacements( whisker_to_improve, direction_results );
  // iterate to find the best replacement of what we have so far
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
	// print where the whisker currently is
	printf("After evaluating initial directions, whisker is: %s -> score: %f\n", whisker_to_improve.str().c_str(), score_to_beat );

	// only probe a specific direction if one of the original replacements showed improvement
  Direction equals = Direction( EQUALS, EQUALS, EQUALS );
  if ( best_dir != equals ) {
    // iterate to find the best replacement in this direction
    int change_index = 0;
    for ( int i = 0; i < 3; i++ ) {
      if ( best_dir.get_index( i ) != EQUALS )
        change_index = i;
    }

		// calculate which direction to probe until score stops improving
    double change = ( change_index == WINDOW_INCR ) ? WINDOW_INCR_CHANGE : ( change_index == WINDOW_MULT ) ? WINDOW_MULT_CHANGE : INTERSEND_CHANGE;
    double cur_change = change;
    double cur_value = ( change_index == WINDOW_INCR ) ? whisker_to_improve.window_increment() : ( change_index == WINDOW_MULT ) ? whisker_to_improve.window_multiple() : whisker_to_improve.intersend();
    bool neg = ( best_dir.get_index( change_index ) == MINUS);
    bool optimize_increment = ( change_index == WINDOW_INCR );
    bool optimize_multiplier = ( change_index == WINDOW_MULT );
    bool optimize_intersend = ( change_index == INTERSEND );

		double current_best_score = score_to_beat;
    Whisker best_replacement = whisker_to_improve;
    while ( true ) {
      Whisker next_action = whisker_to_improve.next_action( optimize_increment, optimize_multiplier, optimize_intersend, cur_value, cur_change, neg );
      // score this action and see if it's better
      double score = evaluate_whisker( tree, next_action, eval );
      printf("Tried whisker %s, score is: %f\n", next_action.str().c_str(), score );

			if ( score > current_best_score ) {
        current_best_score = score;
        best_replacement = next_action;
        cur_change += change;
      } else {
        break;
      }
    }
    whisker_to_improve = best_replacement;
    score_to_beat = current_best_score;
  } else {
    printf("From the initial direction, no one direction gave a good improvement\n");
  }
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

vector< Whisker >
SmartBreeder::get_further_replacements( Whisker & whisker_to_improve, unordered_map< Direction, double, boost::hash< Direction > > direction_results )
{
	// parse through direction results to calculate the best direction to try
	vector< Whisker > replacements;

	Direction best_dir = Direction( EQUALS, EQUALS, EQUALS );
	for ( int i = 0; i < 3; i ++ ) {
		Direction plus = Direction( EQUALS, EQUALS, EQUALS );
		Direction minus = Direction( EQUALS, EQUALS, EQUALS );
		plus.replace( i, PLUS );
		minus.replace( i, MINUS );

		double best_change = 0;
		if ( direction_results.find( plus ) != direction_results.end() ) {
			if ( direction_results.at( plus ) > best_change ) {
				best_dir.replace( i, PLUS );
				best_change = direction_results.at( plus );
			}
		}
		if ( direction_results.find( minus ) != direction_results.end() ) {
			if ( direction_results.at( minus ) > best_change ) {
				best_dir.replace( i, MINUS );
				best_change = direction_results.at( minus );
			}
		}
	}

	if ( best_dir == Direction( EQUALS, EQUALS, EQUALS ) ) {
		return replacements;
	}

	// generate a list of replacements to try based on the direction that was best
	// construct an "ActionChange" Map -> look in whisker.hh
	ActionChange actions;
	actions[WINDOW_INCR] = ( best_dir.get_index( WINDOW_INCR ) == PLUS ) ? 1 : ( best_dir.get_index( WINDOW_INCR ) == EQUALS ) ? 0: -1;
	actions[WINDOW_MULT] = ( best_dir.get_index( WINDOW_MULT ) == PLUS ) ? 1 : ( best_dir.get_index( WINDOW_MULT ) == EQUALS ) ? 0: -1;
	actions[INTERSEND] = ( best_dir.get_index( INTERSEND ) == PLUS ) ? 1 : ( best_dir.get_index( INTERSEND ) == EQUALS ) ? 0: -1;
	return whisker_to_improve.next_in_direction( actions );

}
double
SmartBreeder::evaluate_whisker( WhiskerTree &tree, Whisker replacement, Evaluator< WhiskerTree > eval)
{
  // returns the score from evaluating  single whisker
	bool trace = false;
  int carefulness = 1;
 	if ( eval_cache_.find( replacement ) == eval_cache_.end() ) {
  	// replace whisker
    WhiskerTree replaced_tree( tree );
    const bool found_replacement __attribute((unused)) = replaced_tree.replace( replacement );
    assert (found_replacement);
    Evaluator<WhiskerTree>::Outcome outcome = eval.score_in_parallel( replaced_tree, trace, carefulness );
    double score = outcome.score;
    eval_cache_.insert( make_pair( replacement, score ) ); // add this to eval cache
		printf(" Tried whisker: %s, got score: %f\n", replacement.str().c_str(), score );
    return score;
  } else {
    double cached_score = eval_cache_.at( replacement );
		printf(" Tried whisker: %s, got score: %f\n", replacement.str().c_str(), cached_score );
    return cached_score;
  }
}
double
SmartBreeder::evaluate_whisker_list( WhiskerTree &tree, double score_to_beat, vector< Whisker > &replacements, vector< pair < const Whisker&, pair< bool, double > > > &scores, Evaluator< WhiskerTree > eval)
{
  bool trace = false;
  int carefulness = 1;
  double best_change = 0;
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
          double change = ( score - score_to_beat );
          if ( change > best_change )
            best_change = change;
      } else {
        double cached_score = eval_cache_.at( x );
        double change = ( cached_score - score_to_beat );
        if ( change > best_change )
          change = best_change;
        scores.emplace_back( x, make_pair( false, cached_score ) );
      }
  }
  return best_change; // return best change in any direction
}


unordered_map< Direction, vector< Whisker >, boost:: hash< Direction > >
SmartBreeder::get_direction_bins( Whisker & whisker_to_improve )
{
	// sorts all whiskers in list into a map based on direction
  vector< Whisker > replacements = get_initial_replacements( whisker_to_improve );
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
  return map;
}






