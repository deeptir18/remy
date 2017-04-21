#include <iostream>

#include "lerpbreeder.hh"

Evaluator< WhiskerTree >::Outcome LerpBreeder::improve( PointGrid & grid )
{
	PointGrid input_grid( grid, false ); // make sure we don't regress
	Evaluator< WhiskerTree > eval ( _config_range );

	double score_to_beat = (eval.score_lerp_parallel( grid, _carefulness )).score;
	printf("Initial score to beat is %f\n", score_to_beat );
	if ( (check_bootstrap( grid )) ) {
		printf("Trying to optimize the 8 initial points\n");
		// make a tuple with the signal value tuples to optimize
		vector< SignalTuple > signals;
		for ( double send_ewma = 0; send_ewma <= MAX_SEND_EWMA; send_ewma += MAX_SEND_EWMA ) {
			for ( double rec_ewma = 0; rec_ewma <= MAX_REC_EWMA; rec_ewma += MAX_REC_EWMA ) {
				for ( double rtt_ratio = 0; rtt_ratio <= MAX_RTT_RATIO; rtt_ratio += MAX_RTT_RATIO ) {
					SignalTuple signal = make_tuple( send_ewma, rec_ewma, rtt_ratio );
					signals.emplace_back( signal );
				}
			}
		}
		// for ( SignalTuple signal: signals) {
		  SignalTuple signal = signals[0];
			ActionScore opt_action = optimize_point( signal, grid, eval, score_to_beat );
			grid._points[signal] = opt_action.first; // replace action
			score_to_beat = opt_action.second;
	  // }

	} else {
		PointGrid test_grid( grid, true );
    printf("Looking for a new median\n");
    SignalTuple median = eval.grid_get_median_signal( test_grid, _carefulness);
		printf("New median is %s\n", _stuple_str(median).c_str());
		double new_score = optimize_new_median( median, grid, score_to_beat );
		printf("New score after adding the new median is %f\n", new_score );
	}
  Evaluator<WhiskerTree>::Outcome tmp;
  tmp.score = score_to_beat;
  return tmp;
}

size_t hash_value( const DirectionObj& direction)
{
    size_t seed = 0;
    boost::hash_combine( seed, direction._intersend );
    boost::hash_combine( seed, direction._window_multiple );
    boost::hash_combine( seed, direction._window_increment );
    return seed;
}

double
LerpBreeder::optimize_new_median( SignalTuple median, PointGrid & grid, double current_score )
{
	LerpSender sender = LerpSender( grid );
	ActionTuple interpolated_action = sender.interpolate( median );
	PointObj median_pt = make_pair( median, interpolated_action );

	// add the median point, with the interpolated action, to the grid
	sender.add_inner_point( median_pt, grid );
	printf("GRID AFTER ADDING PT: \n%s\n", grid.str().c_str() );
	Evaluator< WhiskerTree > eval( _config_range );
	ActionScore best_new_action = optimize_point( median, grid, eval, current_score );
	assert ( best_new_action.second >= current_score );

	// modify the grid to contain the new median point, and return the score
	grid._points[ median ] = best_new_action.first;
	return best_new_action.second;
}
bool
LerpBreeder::check_bootstrap( PointGrid & grid )
{
	// TODO: change to be just  checking the first point
	if ( grid.size() != 8 ) {
		return false;
	}
	for ( SignalActionMap::iterator it = grid.begin(); it != grid.end(); ++it ) {
		ActionTuple a = it->second;
		if ( ( CWND_INC( a ) != DEFAULT_INCR ) || ( CWND_MULT( a ) != DEFAULT_MULT ) || ( MIN_SEND( a ) != DEFAULT_SEND ) ) {
			return false;
		}
	}
	return true;
}

vector< ActionTuple >
LerpBreeder::get_replacements( PointObj point )
{
	vector< ActionTuple > ret;
	double window_increment = CWND_INC( point.second );
	double window_multiple = CWND_MULT( point.second );
	double intersend = MIN_SEND( point.second );

	auto window_increment_alternatives( get_optimizer().window_increment.alternatives( window_increment, true ));
	auto window_multiple_alternatives( get_optimizer().window_multiple.alternatives( window_multiple, true ));
	auto intersend_alternatives( get_optimizer().intersend.alternatives( intersend, true ));

	for ( const auto & alt_window: window_increment_alternatives ) {
		for ( const auto & alt_multiple: window_multiple_alternatives ) {
			for ( const auto & alt_intersend: intersend_alternatives ) {
				ActionTuple a = make_tuple( alt_window, alt_multiple, alt_intersend );
				ret.push_back( a ); // add a point to try out
			}
		}
	}
	return ret;
}

vector< ActionTuple >
LerpBreeder::get_initial_replacements( PointObj point )
{
	vector< ActionTuple > ret;
	double window_increment = CWND_INC( point.second );
	double window_multiple = CWND_MULT( point.second );
	double intersend = MIN_SEND( point.second );

	auto window_increment_alternatives( get_optimizer().window_increment.alternatives( window_increment, true ));
	auto window_multiple_alternatives( get_optimizer().window_multiple.alternatives( window_multiple, true ));
	auto intersend_alternatives( get_optimizer().intersend.alternatives( intersend, true ));

	for ( const auto & alt_window: window_increment_alternatives ) {
    printf("Initial alt window is %f\n", alt_window);
		ActionTuple a = make_tuple( alt_window, window_multiple, intersend );
		ret.push_back( a ); // add a point to try out
  }
  for ( const auto & alt_multiple: window_multiple_alternatives ) {
    printf("Initial alt multiple is %f\n", alt_multiple);
		ActionTuple a = make_tuple( window_increment, alt_multiple, intersend );
		ret.push_back( a ); // add a point to try out
  }
	for ( const auto & alt_intersend: intersend_alternatives ) {
    printf("Initial alt intersend is %f\n", alt_intersend);
		ActionTuple a = make_tuple( window_increment, window_multiple, alt_intersend );
		ret.push_back( a ); // add a point to try out
  }
	return ret;
}
ActionTuple
LerpBreeder::next_action_dir( DirectionObj dir, ActionTuple current_action, int step )
{
	double num = double(step)* double( _carefulness );
	double incr_change = ( dir.get_index( 0 ) == EQUALS ) ? 0 : ( dir.get_index( 0 ) == PLUS ) ? 1: -1;
	double mult_change = ( dir.get_index( 1 ) == EQUALS ) ? 0 : ( dir.get_index( 1 ) == PLUS ) ? 1: -1;
	double send_change = ( dir.get_index( 2 ) == EQUALS ) ? 0 : ( dir.get_index( 2 ) == PLUS ) ? 1: -1;
  printf("The step is %d, incr_change: %f, mult_change: %f, send_change: %f\n", step, incr_change, mult_change, send_change );
  printf("The current action is %s\n", _atuple_str( current_action ).c_str() );
	double new_incr = CWND_INC( current_action ) + incr_change *num*WINDOW_INCR_CHANGE;
	double new_mult = CWND_MULT( current_action ) + mult_change * num * WINDOW_MULT_CHANGE;
	double new_send = MIN_SEND( current_action ) + send_change * num * INTERSEND_CHANGE;
	return make_tuple( new_incr, new_mult, new_send );
}


pair< ActionScore, unordered_map< ActionTuple, double, HashAction > >
LerpBreeder::internal_optimize_point( SignalTuple signal, PointGrid & grid, Evaluator< WhiskerTree > eval, double current_score, unordered_map< ActionTuple, double, HashAction >  eval_cache, PointObj point )
{
  printf("Entering internal optimize point function -> score to beat is %f\n", current_score);
  printf("Current best action tuple is %s\n", _atuple_str( point.second ).c_str() );
  // generates replacements in a "smart" way
	double score = current_score;
  double original_score = current_score;
	ActionTuple best_action = point.second;
	// get the initial directions
	unordered_map< DirectionObj, vector< ActionTuple >, boost::hash< DirectionObj > > bin = get_direction_bins( point );

  vector< DirectionObj > coordinates;
  vector< pair< Dir, double > > best_directions(3);
  best_directions[0] = make_pair( EQUALS, 0 );
  best_directions[1] = make_pair( EQUALS, 0 );
  best_directions[2] = make_pair( EQUALS, 0 );
  for ( int i=0; i < 3; i++ ) {
    DirectionObj plus = DirectionObj(EQUALS, EQUALS, EQUALS);
    DirectionObj minus = DirectionObj(EQUALS, EQUALS, EQUALS);

    plus.replace( i, PLUS );
    minus.replace( i, MINUS );

    coordinates.emplace_back( plus );
    coordinates.emplace_back( minus );
  }
  // first evaluate initial 6 directions
 	for ( DirectionObj & dir: coordinates ) {
		if ( bin.find( dir ) != bin.end() ) {
      int change_index = -1;
      Dir change_dir = EQUALS;
      for ( int i = 0; i < 3; i++ ) {
        if ( dir.get_index( i ) != EQUALS )
          change_index = i;
      }
      if ( change_index != -1 ) {
        change_dir = dir.get_index( change_index );
      }
      printf("Change index: %d,change_dir: %d\n", change_index, change_dir );
      printf("Exploring direction %s\n", dir.str().c_str() );
			// iterate through this direction and grab the best change to the point
      // also find avg score change this change makes
      double total_score_change = 0;
      int count = 0;
			for ( ActionTuple & a : bin.at( dir ) ) {
				if ( eval_cache.find( a ) == eval_cache.end() ) {
				PointGrid test_grid( grid, false );
				test_grid._points[signal] = a;
				double new_score = ( eval.score_lerp_parallel( test_grid, _carefulness ) ).score;
        printf("Replaced %s to have action %s, score is %f\n", _stuple_str( signal ).c_str(), _atuple_str( a ).c_str(), new_score );
        total_score_change += ( new_score - original_score );
        count += 1;
				if ( new_score > score ) {
					score = new_score;
					best_action = a;
				}
				eval_cache.insert( make_pair( a , score ) );
				} else {
					double new_score = eval_cache.at( a );
          total_score_change += ( new_score - original_score );
          count += 1;

					if ( new_score > score ) {
						score = new_score;
						best_action = a;
					}
				}
			}
      double avg_score_change = total_score_change/double(count);
      printf("Avg score change is %f\n", avg_score_change );
      if ( avg_score_change > 0 && change_index >= 0) {
        best_directions[change_index] = make_pair( change_dir, avg_score_change );
	      DirectionObj changed_dir = DirectionObj( best_directions[0].first, best_directions[1].first, best_directions[2].first );
        printf("The bes directions is now %s\n", changed_dir.str().c_str() );
      }
		}
	}

	// now figure out what direction the best change was in
	DirectionObj equals = DirectionObj( EQUALS, EQUALS, EQUALS );
	DirectionObj change_dir = DirectionObj( best_directions[0].first, best_directions[1].first, best_directions[2].first );
  printf("The change dir is %s\n", change_dir.str().c_str() );
	if ( change_dir == equals ) { // do not continue in a single direction
		return make_pair( make_pair( best_action, score ), eval_cache );
	}
	int step = 1;
  ActionTuple next_action = best_action;
	while ( true ) {
    printf("Entering while true\n");
		next_action = next_action_dir( change_dir, next_action, step  );
    printf("Calculated next action is %s\n", _atuple_str( next_action ).c_str() );
    // check if the next action is invalid in someway
    if ((  CWND_INC( next_action ) < MIN_WINDOW_INCR ) || ( CWND_INC( next_action ) > MAX_WINDOW_INCR ) ) { break; }
    if ((  CWND_MULT( next_action ) < MIN_WINDOW_MULT ) || ( CWND_MULT( next_action ) > MAX_WINDOW_MULT ) ) { break; }
    if ((  MIN_SEND( next_action ) < MIN_INTERSEND ) || ( CWND_INC( next_action ) > MAX_INTERSEND ) ) { break; }

		double new_score = score;
		if ( eval_cache.find( next_action ) == eval_cache.end() ) {
			PointGrid test_grid( grid, false );
			test_grid._points[signal] = next_action;
			new_score = eval.score_lerp_parallel( test_grid, _carefulness ).score;
        printf("Replaced %s to have action %s, score is %f\n", _atuple_str( next_action ).c_str(), _stuple_str( signal ).c_str(), new_score );
		} else {
			new_score = eval_cache.at( next_action );
		}
		if ( new_score > score ) {
			score = new_score;
			best_action = next_action;
			step += 1;
		} else {
			break;
		}
	}
	return make_pair( make_pair( best_action, score ), eval_cache );

}

ActionScore
LerpBreeder::optimize_point( SignalTuple signal, PointGrid & grid, Evaluator< WhiskerTree > eval, double current_score )
{
	double score = current_score, 
				 last_score = current_score;
	unordered_map< ActionTuple, double, HashAction > eval_cache {};
	ActionTuple original_action = grid._points[signal];
	ActionTuple best_action = original_action;
	while ( true ) {
		last_score = score;
		PointObj point = make_pair( signal, best_action );
		// run internal optimize point -> actual interesting work
		pair< ActionScore, unordered_map< ActionTuple, double, HashAction > > ret = internal_optimize_point( signal, grid, eval, score, eval_cache, point );
		eval_cache = ret.second;
		best_action = ret.first.first;
		score = ret.first.second;
    printf("After one more round of internal optimize point, best action is %s and score is %f\n", _atuple_str( best_action ).c_str(), score );
		if ( score == last_score ) {
			break;
		}
  }
	return make_pair( best_action, score );
}

double
single_simulation( PointGrid grid, Evaluator< WhiskerTree > eval, int carefulness )
{
	double score = ( eval.score_lerp( grid, carefulness ).score );
  printf("SCORE AFTER SINGLE SIMULATION IS %f\n", score );
  return score;
}

unordered_map< DirectionObj, vector< ActionTuple >, boost:: hash< DirectionObj > >
LerpBreeder::get_direction_bins( PointObj point_to_improve )
{
	// sorts all whiskers in list into a map based on direction
  vector< ActionTuple > replacements = get_initial_replacements( point_to_improve );
  unordered_map< DirectionObj, vector< ActionTuple >, boost::hash< DirectionObj >> map {};

  for ( ActionTuple x: replacements ) {
    DirectionObj direction = DirectionObj( point_to_improve.second, x );
    //printf("Replacement %s, calculated direction %s\n", _atuple_str( x ).c_str(), direction.str().c_str() );
    if ( map.find( direction ) == map.end() ) {
      // insert a new vector
      vector< ActionTuple > list;
      list.emplace_back( x );
      map.insert( make_pair( direction, list ) );
    } else {
      vector< ActionTuple > list = map.at( direction );
      list.emplace_back( x );
      map[direction] = list;
    }
  }
  return map;
}



