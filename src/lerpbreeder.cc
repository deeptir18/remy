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
	  eval.score_lerp( test_grid, _carefulness );
		SignalTuple median = test_grid.get_median_signal();
		printf("New median is %s\n", _stuple_str(median).c_str());
		double new_score = optimize_new_median( median, grid, score_to_beat );
		printf("New score after adding the new median is %f\n", new_score );
	}
  Evaluator<WhiskerTree>::Outcome tmp;
  tmp.score = score_to_beat;
  return tmp;
}

double
LerpBreeder::optimize_new_median( SignalTuple median, PointGrid & grid, double current_score )
{
	LerpSender sender = LerpSender( grid );
	ActionTuple interpolated_action = sender.interpolate( median );
	Point median_pt = make_pair( median, interpolated_action );

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
LerpBreeder::get_replacements( Point point )
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
	vector< ActionTuple > fake_ret;
	fake_ret.push_back( make_tuple( 60, .8, .22 ) );
	//fake_ret.push_back( make_tuple(17, .9, .004 ) );
	//fake_ret.push_back( make_tuple( 70, .8, .15 ) );
	//fake_ret.push_back( make_tuple( 50, .85, .35 ) );
	return fake_ret;
	//return ret;
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
		Point point = make_pair( signal, best_action );
		vector< ActionTuple > replacements = get_replacements( point );
		for ( ActionTuple & a: replacements ) {
			if ( eval_cache.find( a ) == eval_cache.end() ) {
				printf("Trying action %s\n", _atuple_str( a ).c_str() );
				PointGrid test_grid( grid, false );
				test_grid._points[signal] = a;
				printf("REPLACED test grid to have %s->%s\n", _stuple_str( signal ).c_str(), _atuple_str( test_grid._points[signal] ).c_str() );
				// test to see if it's in the eval cache
				double new_score = ( eval.score_lerp_parallel( test_grid, _carefulness ) ).score;
				printf("Evaluated signal->action: %s -> %s; score: %f\n", _stuple_str( signal).c_str(), _atuple_str( test_grid._points[signal] ).c_str(), new_score);
				printf("\n");
				if ( new_score > score ) {
					score = new_score;
					best_action = a;
				}
				eval_cache.insert( make_pair( a, score ) );
			} else {
				double new_score = eval_cache.at( a );
				if ( new_score > score ) {
					score = new_score;
					best_action = a;
				}
			}
		}
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

ActionScore
LerpBreeder::optimize_point_parallel( SignalTuple signal, PointGrid & grid, double current_score )
{
	vector< pair< const ActionTuple, future< double > > > replacement_scores;
	ActionTuple original_action = grid._points[signal];
	ActionTuple best_action = original_action;
	Point point = make_pair( signal, original_action );
  printf("OPTIMIZING SIGNAL %s\n", _stuple_str( signal ).c_str() );
	vector< ActionTuple > replacements = get_replacements( point );
	int index = 0;
	for ( ActionTuple & a: replacements ) {
		PointGrid test_grid( grid, false);
		test_grid._points[signal] = a;
		Evaluator< WhiskerTree > eval2( _config_range );
		replacement_scores.emplace_back( a, async(launch::async, single_simulation, test_grid, eval2, _carefulness ) );
		index++;
	}

	for ( auto & x: replacement_scores ) {
		const ActionTuple  replacement( x.first );
		double score( x.second.get() );
    printf("Replacement %s -> score %f\n", _atuple_str( replacement).c_str(), score );
		if ( score > current_score ) {
			current_score = score;
			best_action = replacement;
		}
	}
	return make_pair( best_action, current_score );
}

