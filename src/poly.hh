#ifndef POLYNOMIAL_HH
#define POLYNOMIAL_HH
#include <vector>
#include "memory.hh"
#include <cmath>
using namespace std;
/*Define a 'PolynomialMapping' per memory signal in question*/
//class PolynomialMapping {
//public:
class Polynomial {
  public:
    int _degree;
    int _num_signals;
    vector< vector< double > > _terms;
    double _constant;
    Polynomial( const int degree, const int signals ) : _degree ( degree ), _num_signals( signals ), _terms( _num_signals ), _constant( 0 )
    {
      // STRUCTURE: each vector has the coefficients ( in order of degree ) for each of the signals
      for ( int i = 0; i < _num_signals; i++ ) {
        vector< double > coeffs( _degree );
        for ( int j = 0; j < _degree; j++ ) {
          coeffs[j] = .01;
        }
        _terms[i] = coeffs;
      }
    }
    double get_value( const vector< double > signals )
    {
      // order of signals is the same order as the vector of coefficients
      double ret = 0;
      assert( (int)signals.size() == _num_signals );
      for ( int i = 0; i < _num_signals; i++ ) {
        double x = signals[i];
        for ( int j = 0; j < _degree; j++ ) {
          vector< double > coeffs = _terms[i];
          //printf("The coeff is %f, and the x is %f\n", coeffs[j], x);
          ret += coeffs[j]*pow((float)x, (float)(j+1));
        }
      }
      ret += _constant;
      return ret;
    }

    void place_val( const int degree, const int signal, const double val )
    {
      if ( degree == 0 ) {
        _constant = val;
        return;
      }
      vector< double > coeffs = _terms.at(signal);
      coeffs[degree - 1] = val;
    }

    void initialize_vals( const vector< double > vals )
    {
      assert ( (int)vals.size() == _num_signals*_degree );
      int cur_signal = 0;
      for ( int i = 0; i < (int)vals.size(); i ++ ) {
        //vector < double > coeffs = _terms[cur_signal];
        int place = i % ( _degree );
        _terms[cur_signal][place] = vals[i];
        if ( ( (i+1) % _degree ) == 0 ) {
          cur_signal += 1;
        }
      }
    }
};

#endif /* POLYNOMIAL_HH */
