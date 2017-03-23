#ifndef LERPBREEDER_HH
#define LERPBREEDER_HH

using namespace std;

#include "lerpsender.hh"
#include "configrange.hh"
#include "evaluator.hh"
#include "whiskertree.hh"
#include <future>
#include <unordered_map>
typedef pair< ActionTuple, double > ActionScore;
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
		vector< ActionTuple > get_replacements( Point point_to_improve );
		bool check_bootstrap( PointGrid & grid );
		ActionScore optimize_point( SignalTuple signal, PointGrid & grid, Evaluator< WhiskerTree > eval, double current_score );
		ActionScore optimize_point_parallel( SignalTuple signal, PointGrid & grid, double current_score );
		double optimize_new_median( SignalTuple median, PointGrid & original_grid, double current_score );
		//double single_simulation( PointGrid & grid );
	public:
		LerpBreeder( ConfigRange range ): _config_range( range ), _carefulness( 1 ) {}
		Evaluator< WhiskerTree >::Outcome improve( PointGrid & grid);
		static const OptimizationSettings & get_optimizer( void )
		{
   		static OptimizationSettings default_settings {
      	{ 0,    256, 1,    32,  4, 1 }, /* window increment */
      	{ 0,    1,   0.01, 0.5, 4, 1 }, /* window multiple */
      	{ 0.002, 3,   0.05, 1,   4, 3 } /* intersend */
    	};
    	return default_settings;
		}
};
/*########################################################################*/
#endif
