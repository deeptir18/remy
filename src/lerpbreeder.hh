#ifndef LERPBREEDER_HH
#define LERPBREEDER_HH

using namespace std;

#include "lerpsender.hh"
#include "configrange.hh"
#include "evaluator.hh"
#include "whiskertree.hh"
#include <future>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#define WINDOW_INCR 0
#define WINDOW_MULT 1
#define INTERSEND 2
#define WINDOW_INCR_CHANGE 5
#define WINDOW_MULT_CHANGE .01
#define INTERSEND_CHANGE .2
#define MIN_WINDOW_INCR 1
#define MAX_WINDOW_INCR 400
#define MIN_WINDOW_MULT .01
#define MAX_WINDOW_MULT 1
#define MIN_INTERSEND .002
#define MAX_INTERSEND 30

typedef pair< ActionTuple, double > ActionScore;
/*#####
	TODO: 1. Add in sense of "direction" for point( just a wrapper around the points )
	2. Code in final good algorithm from last time: coordinate descent
	3. See if this version can get to something close to the optimal point for the first generation in reasonable time as well
		-> Add "direction" objects and create direction from two ActionTuples
		-> Generate a bunch of replacements that modify one parameter at a time
		-> Get the best changes from that and keep going in that direction until it stops improving
	4. Probably just use the same optimization parameters as before (?)
	*/

/*########################################################################*/
enum Dir {
  PLUS,
  MINUS,
  EQUALS
};

class DirectionObj
{
private:
  Dir _window_increment;
  Dir _window_multiple;
  Dir _intersend;
  Dir get_direction( double a, double b )
  {
    if ( a == b ) {
      return EQUALS;
    }
    if ( a < b ) {
      return PLUS;
    }
    return MINUS;
}
public:
  DirectionObj( const Dir window_increment, const Dir window_multiple, const Dir intersend )
    : _window_increment( window_increment ), _window_multiple( window_multiple ), _intersend( intersend ) {}
  DirectionObj( const ActionTuple original, const ActionTuple replacement )
    : _window_increment( PLUS ), _window_multiple( PLUS ), _intersend( PLUS )
    {
      _window_increment = get_direction( CWND_INC( original ), CWND_INC( replacement ) );
      _window_multiple = get_direction( CWND_MULT( original ), CWND_MULT( replacement ) );
      _intersend = get_direction( MIN_SEND( original ), MIN_SEND( replacement ) );
    }
  void replace( const int i, const Dir dir )
  {
    if ( i == WINDOW_INCR ) {
      _window_increment = dir;
    } else if ( i == WINDOW_MULT ) {
      _window_multiple = dir;
    } else if ( i == INTERSEND ) {
      _intersend = dir;
    }
  }
  Dir get_index( int i )
  {
    assert( i == WINDOW_INCR || i == WINDOW_MULT || i == INTERSEND );
    if ( i == WINDOW_INCR ) { return _window_increment; }
    if ( i == WINDOW_MULT ) { return _window_multiple; }
		return _intersend;
  }

  bool operator==( const DirectionObj& other ) const { return
		( _window_increment == other._window_increment ) &&
		( _window_multiple == other._window_multiple ) &&
		( _intersend == other._intersend ); }

		bool operator!=( const DirectionObj& other ) const { return
		( _window_increment != other._window_increment ) ||
		( _window_multiple != other._window_multiple ) ||
		( _intersend != other._intersend ); }

		std::string str(void) const
  {
    // map
    typedef map< Dir, std::string > DirPrint;
    DirPrint print_map;
    print_map.insert( DirPrint::value_type( PLUS, "plus" ) );
    print_map.insert( DirPrint::value_type( MINUS, "minus" ) );
    print_map.insert( DirPrint::value_type( EQUALS, "equals" ) );
    char tmp[256];
		snprintf(tmp, 256, "{%s, %s, %s}: {incr, mult, intersend}\n", print_map[_window_increment].c_str(), print_map[_window_multiple].c_str(), print_map[_intersend].c_str() );
    return tmp;
}
  friend size_t hash_value( const DirectionObj& direction );
};

/*########################################################################*/
struct OptimizationSetting
{
    double min_value; /* the smallest the value can be */
    double max_value; /* the biggest */

    double min_change; /* the smallest change to the value in an optimization exploration step */
    double max_change; /* the biggest change */

    double multiplier; /* we will explore multiples of the min_change until we hit the max_change */
    /* the multiplier defines which multiple (e.g. 1, 2, 4, 8... or 1, 3, 9, 27... ) */

    double default_value;

    bool eligible_value( const double & value ) const { return value >= min_value and value <= max_value; }

    vector< double > alternatives( const double value, bool active ) const 
    {
      if ( !eligible_value( value ) ) {
        printf("Ineligible value: %s is not between %s and %s\n", to_string( value ).c_str(), to_string( min_value ).c_str(), to_string( max_value ).c_str());
        assert(false);
      }

      vector< double > ret( 1, value );

      /* If this axis isn't active, return only the current value. */
      if (!active) return ret;

      for ( double proposed_change = min_change;
            proposed_change <= max_change;
            proposed_change *= multiplier ) {
        /* explore positive change */
        const double proposed_value_up = value + proposed_change;
        const double proposed_value_down = value - proposed_change;

        if ( eligible_value( proposed_value_up ) ) {
          ret.push_back( proposed_value_up );
        }

        if ( eligible_value( proposed_value_down ) ) {
          ret.push_back( proposed_value_down );
        }
      }

      return ret;
    }
};
/*########################################################################*/
struct OptimizationSettings
{
	OptimizationSetting window_increment;
  OptimizationSetting window_multiple;
  OptimizationSetting intersend;
};
/*########################################################################*/

class LerpBreeder
{
	private:
		ConfigRange _config_range;
		int _carefulness;
		vector< ActionTuple > get_replacements( PointObj point_to_improve );
		vector< ActionTuple > get_initial_replacements( PointObj point_to_improve );
		bool check_bootstrap( PointGrid & grid );
		pair< ActionScore, unordered_map< ActionTuple, double, HashAction > > internal_optimize_point( SignalTuple signal, PointGrid & grid, Evaluator< WhiskerTree > eval, double current_score, std::unordered_map< ActionTuple, double, HashAction > eval_cache, PointObj point );
		ActionScore optimize_point( SignalTuple signal, PointGrid & grid, Evaluator< WhiskerTree > eval, double current_score );
		unordered_map< DirectionObj, vector< ActionTuple >, boost:: hash< DirectionObj > > get_direction_bins( PointObj point_to_improve );
		ActionTuple next_action_dir( DirectionObj dir, ActionTuple current_action, int step );
	public:
		LerpBreeder( ConfigRange range ): _config_range( range ), _carefulness( 1 ) {}
		Evaluator< WhiskerTree >::Outcome improve( PointGrid & grid);
		static const OptimizationSettings & get_optimizer( void )
		{
   		static OptimizationSettings default_settings {
      	{ 1,    256, 1,    32,  4, 1 }, /* window increment */
      	{ .01,    1,   0.01, 0.5, 4, 1 }, /* window multiple */
      	{ 0.002, 3,   0.05, 1,   4, 3 } /* intersend */
    	};
    	return default_settings;
		}
};
/*########################################################################*/
#endif
