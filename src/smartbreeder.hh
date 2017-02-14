#ifndef SMARTBREEDER_HH
#define SMARTBREEDER_HH

#include "breeder.hh"
struct WhiskerImproverOptions
{
  bool optimize_window_increment = true;
  bool optimize_window_multiple = true;
  bool optimize_intersend = true;
};
enum Dir {
  PLUS,
  MINUS,
  EQUALS
};

class Direction
{
private:
  Dir _intersend;
  Dir _window_multiple;
  Dir _window_increment;
  double _intersend_diff;
  double _window_multiple_diff;
  double _window_increment_diff;
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
  Direction( const Whisker& original, const Whisker& replacement )
    : _intersend( PLUS ), _window_multiple( PLUS ), _window_increment( PLUS ), _intersend_diff( 0 ), _window_multiple_diff( 0 ), _window_increment_diff( 0 )
    {
      _intersend = get_direction( original.intersend(), replacement.intersend() );
      _intersend_diff = replacement.intersend() - original.intersend();
      _window_multiple = get_direction( original.window_multiple(), replacement.intersend() );
      _window_multiple_diff = replacement.window_multiple() - original.window_multiple();
      _window_increment = get_direction( double( original.window_increment() ), double(original.window_multiple()) );
      _window_increment_diff = replacement.window_increment() - original.window_increment();
    }
  bool operator==( const Direction& other ) const { return ( _intersend == other._intersend ) && ( _window_multiple == other._window_multiple ) && ( _window_increment == other._window_increment ); }
  std::string str(void) const
{
  char tmp[256];
  snprintf(tmp, 256, "{%f, %f, %f}: {intersend, mult, incr}", _intersend_diff, _window_multiple_diff, _window_increment_diff);
  return tmp;
}
  friend size_t hash_value( const Direction& direction );
};

class SmartBreeder : public Breeder< WhiskerTree >
{
private:
  WhiskerImproverOptions _whisker_options;
  vector< Whisker > get_replacements( Whisker & whisker_to_improve );
  vector< Whisker > get_sorted_replacements( Whisker & whisker_to_improve );
  double  improve_whisker( Whisker & whisker_to_improve, WhiskerTree & tree, double score_to_beat);
  bool evaluate_replacement( Whisker& replacement, Whisker& original, std::unordered_map< Direction, double, boost::hash< Direction >> direction_map );
  std::unordered_map< Whisker, double, boost::hash< Whisker > > eval_cache_ {};
public:
  SmartBreeder( const BreederOptions & s_options, const WhiskerImproverOptions & s_whisker_options )
    : Breeder( s_options ), _whisker_options( s_whisker_options ) {};

  Evaluator< WhiskerTree >::Outcome improve( WhiskerTree & whiskers );
};

#endif
