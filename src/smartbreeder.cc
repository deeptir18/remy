#include <iostream>

#include "smartbreeder.hh"

using namespace std;

Evaluator< WhiskerTree >::Outcome SmartBreeder::improve( WhiskerTree & whiskers, int num_generations )
{
  /* back up the original whiskertree */
  /* this is to ensure we don't regress */
  WhiskerTree input_whiskertree( whiskers );

  /* evaluate the whiskers we have */
  whiskers.reset_generation();
  unsigned int generation = 0;
  while ( int(generation) < num_generations ) {
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

vector< Whisker > SmartBreeder::get_replacements( Whisker & whisker_to_improve, bool wide )
{
  return whisker_to_improve.next_generation( _whisker_options.optimize_window_increment,
                                             _whisker_options.optimize_window_multiple,
                                             _whisker_options.optimize_intersend, wide );
}
// returns the change index in this particular direction (or first index where something is different)
pair< int, Dir > get_change_index( Direction dir ) {
  for ( int i = 0; i < 3; i++ ) {
    if ( dir.get_index( i ) != EQUALS ) {
      return make_pair( i, dir.get_index( i ));
    }
  }
  return make_pair( 3, EQUALS ); // returns 3 if everything is equals
}

double
SmartBreeder::get_score_to_beat( double current_score, Whisker& whisker_to_improve, vector< pair< const Whisker&, pair< bool, double > > > scores )
{
  for ( auto & x : scores ) {
    const Whisker & replacement( x.first );
    const auto outcome( x.second );
    bool was_new_evaluation( outcome.first );
    const double score( outcome.second );

    if ( was_new_evaluation) {
      eval_cache_.insert( make_pair( replacement, score ) ); 
    }
    if ( score > current_score ) {
      current_score = score;
      whisker_to_improve = replacement;
    }
  }

  return current_score;
}
double
SmartBreeder::improve_whisker( Whisker & whisker_to_improve, WhiskerTree & tree, double score_to_beat )
{
  std::cout << "In improve whisker function for smartbreeder" << std::endl;
  // evaluates replacements in sequence in a smart way
  vector< pair < const Whisker&, pair< bool, double > > > scores;
  vector< pair< double, Dir > > initial_best_improvements;
  for ( int i = 0; i < 3; i++ ) {
    initial_best_improvements.emplace_back( make_pair( 0, EQUALS ));
  }
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
  // first evaluate initial 6 directions
  // figure out for each initial direction -> if it's worth it to change it
  for ( Direction& dir: coordinates ) {
    if ( bin.find( dir ) != bin.end() ) {
      double avg_change = evaluate_initial_whisker_list( tree, bin.at( dir ), scores, eval, score_to_beat ) ;
      // which variable changed?
      pair< int, Dir> change_index = get_change_index( dir );
	  cout << "Trying dir: " << dir.str().c_str() << std::endl;
      assert( change_index.first >= 0 && change_index.first < 3 );
      if ( avg_change > initial_best_improvements.at( change_index.first).first ) {
        initial_best_improvements[change_index.first] = make_pair( avg_change, change_index.second );
      }
    }
  }
  // go through the replacements evaluated so far and replace the whisker if something in the scores is better
  score_to_beat = get_score_to_beat( score_to_beat, whisker_to_improve, scores );
  printf("With score %f, chose %s\n", score_to_beat, whisker_to_improve.str().c_str() );

  // now take the "best" direction, and keep probing it until it stops improving
  double optimize_window_increment = ( initial_best_improvements[WINDOW_INCR].second == PLUS ) ? 1 : ( initial_best_improvements[WINDOW_INCR].second == MINUS ) ? -1 : 0;
  double optimize_window_multiple = ( initial_best_improvements[WINDOW_MULT].second == PLUS ) ? 1 : ( initial_best_improvements[WINDOW_MULT].second == MINUS ) ? -1 : 0;
  double optimize_intersend = ( initial_best_improvements[INTERSEND].second == PLUS ) ? 1 : ( initial_best_improvements[INTERSEND].second == MINUS ) ? -1 : 0;
  double old_score = score_to_beat;
  while (1) {
    vector < Whisker > next_whiskers = whisker_to_improve.next_in_direction( optimize_window_increment, optimize_window_multiple, optimize_intersend );
    if ( next_whiskers.size() == 0 ) {
	  std::cout << "No next whiskers" << std::endl;
      break; // no more improvements after initial - nothing is better
    }
    vector< pair < const Whisker&, pair< bool, double > > > next_scores;
    // evaluate this whisker list and update the RemyCC
    evaluate_whisker_list( tree, score_to_beat, next_whiskers, next_scores, eval );
    score_to_beat = get_score_to_beat( score_to_beat, whisker_to_improve, next_scores );
    if ( score_to_beat == old_score ) { // stopped improving in this direction -> return
      break;
    }
    old_score = score_to_beat;
  }
  printf("With score %f, chose %s\n", score_to_beat, whisker_to_improve.str().c_str());
  return score_to_beat;
}


size_t hash_value( const Direction& direction ) {
    size_t seed = 0;
    boost::hash_combine( seed, direction._intersend );
    boost::hash_combine( seed, direction._window_multiple );
    boost::hash_combine( seed, direction._window_increment );
    return seed;
}

// is this direction worth evaluating?
double
SmartBreeder::evaluate_initial_whisker_list( WhiskerTree &tree, vector< Whisker > &replacements, vector< pair < const Whisker&, pair< bool, double > > > &scores, Evaluator< WhiskerTree > eval, double score_to_beat)
{
  bool trace = false;
  int carefulness = 1;
  double avg_score_change = 0;
  double total_score_change = 0;
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
          total_score_change += ( score - score_to_beat );
      } else {
        double cached_score = eval_cache_.at( x );
        scores.emplace_back( x, make_pair( false, cached_score ) );
        total_score_change += ( cached_score - score_to_beat );
      }
  }
  // TODO: maybe change this to not do the average
  avg_score_change = total_score_change/( double( replacements.size() ) );
  return avg_score_change;
}

double
SmartBreeder::evaluate_whisker_list( WhiskerTree &tree, double score_to_beat, vector< Whisker > &replacements, vector< pair < const Whisker&, pair< bool, double > > > &scores, Evaluator< WhiskerTree > eval)
{
  std::cout << "In evaluate whisker list function" << std::endl;
  double ret = score_to_beat;
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
	cout << "Tried whisker: " << x.str().c_str() << "-> score: " << score << std::endl;
      scores.emplace_back( x, make_pair( true, score ) );
      if ( score > score_to_beat ) {
        printf("Tried Whisker: %s, and score improved to %f\n", x.str().c_str(), score);
        ret = score;
      } else {
        printf("Tried Whisker: %s, and score did not improve to %f\n", x.str().c_str(), score);
      }
    } else {
      double cached_score = eval_cache_.at( x );
      scores.emplace_back( x, make_pair( false, cached_score ) );
    }
  }
  return ret;
}

unordered_map< Direction, vector< Whisker >, boost:: hash< Direction > >
SmartBreeder::get_direction_bins( Whisker & whisker_to_improve )
{
  vector< Whisker > replacements = get_replacements( whisker_to_improve, false );
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

