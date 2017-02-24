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
      printf("--------------------------------------\n");
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

  unordered_map< Direction, vector< Whisker >, boost:: hash< Direction > > bin = get_direction_bins( whisker_to_improve );
  vector< Direction > coordinates;
  unordered_map< Direction, bool, boost:: hash< Direction > > coordinate_map;
  for ( int i=0; i < 3; i++ ) {
    Direction plus = Direction(EQUALS, EQUALS, EQUALS);
    Direction minus = Direction(EQUALS, EQUALS, EQUALS);

    plus.replace( i, PLUS );
    minus.replace( i, MINUS );

    coordinates.emplace_back( plus );
    coordinates.emplace_back( minus );
  }
  // first evaluate initial 6 directions
  for ( Direction& dir: coordinates ) {
    if ( bin.find( dir ) != bin.end() ) {
      printf("Evaluating direction %s\n", dir.str().c_str() );
      bool dir_bad = ( evaluate_whisker_list( tree, score_to_beat, bin.at( dir ), scores, eval ) );
      coordinate_map.insert( make_pair ( dir, dir_bad ) );
      string bad = "good";
      if ( dir_bad )
        bad = "bad";
      printf("Direction %s is %s\n", dir.str().c_str(), bad.c_str());
    }
  }
  // try rest -> based on info from first 6 directions
  int not_evaluated = 0;
  for ( auto it = bin.begin(); it != bin.end(); ++it ) {
    if (!( evaluate_direction( it->first, coordinate_map ))) {
     printf("Evaluating direction %s\n", (it->first).str().c_str() );
      evaluate_whisker_list( tree, score_to_beat, it->second, scores, eval );
    } else {
      not_evaluated += ( it->second ).size();
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
  bool all_bad = true;
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
          if ( score > score_to_beat )
            all_bad = true;
      } else {
        double cached_score = eval_cache_.at( x );
        scores.emplace_back( x, make_pair( false, cached_score ) );
      }
  }
  return all_bad;
}


unordered_map< Direction, vector< Whisker >, boost:: hash< Direction > >
SmartBreeder::get_direction_bins( Whisker & whisker_to_improve )
{
  vector< Whisker > replacements = get_replacements( whisker_to_improve );
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

/*Function evaluates if we should evaluate a direction based on coordinate descent information*/
bool
SmartBreeder::evaluate_direction( Direction direction, unordered_map< Direction, bool, boost:: hash< Direction > > coordinate_map)
{
  // if Direction has been evaluated -> don't evaluate
  if ( coordinate_map.find( direction ) != coordinate_map.end() ) {
    return false;
  }

  // for each change in the direction -> checks if changing that direction alone is bad
  for ( int i = 0; i < 3; i ++ ) {
    if ( direction.get_index( i ) == EQUALS )
      continue; // don't check if equals at that index
    Direction dir = Direction( EQUALS, EQUALS, EQUALS );
    dir.replace( i, direction.get_index( i ) );
    if ( coordinate_map.find( dir ) != coordinate_map.end() ) {
      if ( !( coordinate_map.at( dir ) ) ) { // if this direction is BAD -> don't evaluate
        return false;
      }
    }
  }
  return true;
}





