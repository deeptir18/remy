#ifndef SMARTBREEDER_HH
#define SMARTBREEDER_HH

#include "breeder.hh"
#include <chrono>
#define INTERSEND 0
#define WINDOW_INCR 1
#define WINDOW_MULT 2
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

// Functor solution.
class SortReplacement
{
private:
    Whisker& _original;
public:
    SortReplacement(Whisker& original) :
        _original( original ) {}

    bool operator()(Whisker const &first, Whisker const &second)
    {
      double distance_one = abs( first.intersend() - _original.intersend() ) + abs( first.window_increment() - _original.window_increment() ) + abs( first.window_multiple() - _original.window_multiple() );
      double distance_two = abs( second.intersend() - _original.intersend() ) + abs( second.window_increment() - _original.window_increment() ) + abs( second.window_multiple() - _original.window_multiple() );
      return (distance_one < distance_two);
    }
};

class Direction
{
private:
  Dir _intersend;
  Dir _window_multiple;
  Dir _window_increment;
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
  Direction( const Dir intersend, const Dir window_multiple, const Dir window_increment )
    : _intersend( intersend ), _window_multiple( window_multiple ), _window_increment( window_increment ) {}
  Direction( const Whisker& original, const Whisker& replacement )
    : _intersend( PLUS ), _window_multiple( PLUS ), _window_increment( PLUS )
    {
      _intersend = get_direction( original.intersend(), replacement.intersend() );
      _window_multiple = get_direction( original.window_multiple(), replacement.intersend() );
      _window_increment = get_direction( double( original.window_increment() ), double(original.window_multiple()) );
    }
  void replace( const int i, const Dir dir )
  {
    if ( i == INTERSEND ) {
      _intersend = dir;
    } else if ( i == WINDOW_MULT ) {
      _window_multiple = dir;
    } else if ( i == WINDOW_INCR ) {
      _window_increment = dir;
    }
  }
  Dir get_index( int i )
  {
    assert( i == INTERSEND || i == WINDOW_INCR || i == WINDOW_MULT );
    if ( i == INTERSEND ) { return _intersend; }
    if ( i == WINDOW_MULT ) { return _window_multiple; }
    return _window_increment;
  }
  bool operator==( const Direction& other ) const { return ( _intersend == other._intersend ) && ( _window_multiple == other._window_multiple ) && ( _window_increment == other._window_increment ); }
  std::string str(void) const
{
  char tmp[256];
  snprintf(tmp, 256, "{%d, %d, %d}: {intersend, mult, incr}", _intersend, _window_multiple, _window_increment);
  return tmp;
}
  friend size_t hash_value( const Direction& direction );
};

class SmartBreeder : public Breeder< WhiskerTree >
{
private:
  WhiskerImproverOptions _whisker_options;
  std::unordered_map< Whisker, double, boost::hash< Whisker > > eval_cache_ {};

  vector< Whisker > get_replacements( Whisker & whisker_to_improve );

  double  improve_whisker( Whisker & whisker_to_improve, WhiskerTree & tree, double score_to_beat);

  bool evaluate_whisker_list( WhiskerTree &tree, double score_to_beat, vector< Whisker > &replacements, vector< pair < const Whisker&, pair< bool, double > > > &scores, Evaluator< WhiskerTree > eval);

  std::unordered_map< Direction, std::pair< vector< Whisker >, vector< Whisker > >, boost:: hash< Direction > > get_direction_bins ( Whisker & whisker_to_improve );
public:
  SmartBreeder( const BreederOptions & s_options, const WhiskerImproverOptions & s_whisker_options )
    : Breeder( s_options ), _whisker_options( s_whisker_options ) {};

  Evaluator< WhiskerTree >::Outcome improve( WhiskerTree & whiskers );
};

#endif
