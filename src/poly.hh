#ifndef POLYNOMIAL_HH
#define POLYNOMIAL_HH
#include <vector>
#include "memory.hh"
#include <cmath>
using namespace std;
/*Define a 'PolynomialMapping' per memory signal in question*/
class PolynomialMapping {
public:
  class Polynomial {
  public:
    int _degree;
    vector< double > _terms;
    Polynomial( const int degree ) : _degree ( degree ), _terms()
    {
      // for now just place constants
      _terms.emplace_back(.01);
      _terms.emplace_back(.01);
    }
    double get_value( const double x )
    {
      double ret = 0;
      for ( int i = 0; i < _degree + 1; i++ ) {
        ret += ( _terms.at(i)*pow((float)x, (float)i) );
      }
      return ret;
    }

    void place_val( const int degree, const double val )
    {
      _terms[degree] = val;
    }
  };

  private:
    // polynomial for the window increment
    Polynomial _window_increment_poly;

    // polynomial for the intersend rate
    Polynomial _intersend_rate_poly;

    // polynomial for the window multiplier
    Polynomial _window_multiplier_poly;


  public:
    // constructor
    PolynomialMapping():  _window_increment_poly( Polynomial( 1 ) ), _intersend_rate_poly( Polynomial( 1 ) ), _window_multiplier_poly( Polynomial( 1 ) ) {}
    double get_window_increment( const double x ) { return _window_increment_poly.get_value( x ); }
    double get_window_multiplier( const double x ) { return _window_multiplier_poly.get_value( x ); }
    double get_intersend( const double x ) { return _intersend_rate_poly.get_value( x ); }

    void initialize_vals(const double incr, double intersend, double multiplier)
    {
      _window_increment_poly.place_val( 1, incr );
      _intersend_rate_poly.place_val( 1, intersend );
      _window_multiplier_poly.place_val( 1, multiplier );
    }

};

#endif /* POLYNOMIAL_HH */
