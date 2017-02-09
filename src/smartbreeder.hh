#ifndef SMARTBREEDER_HH
#define SMARTBREEDER_HH

#include "breeder.hh"
struct WhiskerImproverOptions
{
  bool optimize_window_increment = true;
  bool optimize_window_multiple = true;
  bool optimize_intersend = true;
};

struct ReplacementInfo
{
  Whisker* whisker_to_improve;
  bool evaluate = true; // initially true, change to false
  double score;
  bool finished = false; // finished set to true either either if evaluate is false, or evaulate is true and scoring is done
};


class SmartBreeder : public Breeder< WhiskerTree >
{
private:
  WhiskerImproverOptions _whisker_options;
  vector< Whisker > get_replacements( Whisker & whisker_to_improve );
  double  improve_whisker( Whisker & whisker_to_improve, WhiskerTree & tree, double score_to_beat);
  bool evaluate_replacement( Whisker& replacement );
  std::unordered_map< Whisker, double, boost::hash< Whisker > > eval_cache_ {};
public:
  SmartBreeder( const BreederOptions & s_options, const WhiskerImproverOptions & s_whisker_options )
    : Breeder( s_options ), _whisker_options( s_whisker_options ) {};

  Evaluator< WhiskerTree >::Outcome improve( WhiskerTree & whiskers );
};

#endif
