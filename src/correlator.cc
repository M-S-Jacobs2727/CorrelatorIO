#include "correlator.h"
#include <math.h>

/////////////////////////////////////////
// Correlator class
/////////////////////////////////////////
Correlator::Correlator(const unsigned int numcorrin, const unsigned int pin, const unsigned int min) {
	setsize(numcorrin, pin, min);
}


Correlator::~Correlator() {

	if (numcorrelators ==0) return;

	delete [] shift;
	delete [] correlation;
	delete [] ncorrelation;
	delete [] accumulator;
	delete [] naccumulator;
	delete [] insertindex;

	delete [] t;
	delete [] f;
}


void Correlator::setsize (const unsigned int numcorrin, const unsigned int pin, const unsigned int min) {
	numcorrelators = numcorrin; // 32
	p = pin;  // 16
	m = min;  //2
	dmin = p/m;  // 8

	length = numcorrelators*p; // 512

	shift = new double* [numcorrelators];  //32*16
	correlation = new double* [numcorrelators];  
	ncorrelation = new unsigned long int* [numcorrelators];
	accumulator = new double[numcorrelators];
	naccumulator = new unsigned int [numcorrelators];
	insertindex = new unsigned int [numcorrelators];

	for (unsigned int j=0; j<numcorrelators;++j) {
		shift[j] = new double [p];

		/* It can be optimized: Apart from correlator 0, correlation and ncorrelation arrays only use p/2 values */
		correlation[j] = new double [p];
		ncorrelation[j] = new unsigned long int [p];
	}

	t = new double[length];
	f = new double[length];
}


void Correlator::initialize() {

	for (unsigned int j=0;j<numcorrelators;++j) {  //32
		for (unsigned int i=0;i<p;++i) {  //16
			shift[j][i] = -2E10;
			correlation[j][i] = 0;
			ncorrelation[j][i] = 0;
		}
		accumulator[j] = 0.0;
		naccumulator[j] = 0;
		insertindex[j] = 0;
	}

	for (unsigned int i=0;i<length;++i) {
		t[i] = 0;
		f[i] = 0;
	}

	npcorr =0;
	kmax=0;
	accval=0;
}

void Correlator::add(const double w, const unsigned int k) {

	/// If we exceed the correlator side, the value is discarded
	if (k == numcorrelators) return;
	if (k > kmax) kmax=k;

	/// Insert new value in shift array
	shift[k][insertindex[k]] = w;

	/// Add to average value
	if (k==0)
		accval += w;

	/// Add to accumulator and, if needed, add to next correlator
	accumulator[k] += w;
	++naccumulator[k];
	if (naccumulator[k]==m) {
		add(accumulator[k]/m, k+1);
		accumulator[k]=0;
		naccumulator[k]=0;
	}

	/// Calculate correlation function
	unsigned int ind1=insertindex[k];
	if (k==0) { /// First correlator is different
		int ind2=ind1;
		for (unsigned int j=0;j<p;++j) {  //16
			if (shift[k][ind2] > -1e10) {
				correlation[k][j]+= shift[k][ind1]*shift[k][ind2];
				++ncorrelation[k][j];
			}
			--ind2;
			if (ind2<0) ind2+=p;				
		}
	}
	else {
		int ind2=ind1-dmin;
		for (unsigned int j=dmin;j<p;++j) {  //[8, 16]
			if (ind2<0) ind2+=p;				
			if (shift[k][ind2] > -1e10) {
				correlation[k][j]+= shift[k][ind1]*shift[k][ind2];
				++ncorrelation[k][j];				
			}
			--ind2;
		}
	}

	++insertindex[k];
	if (insertindex[k]==p) insertindex[k]=0;
}

void Correlator::evaluate(const bool norm) {
	unsigned int im=0;

	double aux=0;
	if (norm)
		aux = (accval/ncorrelation[0][0])*(accval/ncorrelation[0][0]);

	// First correlator
	for (unsigned int i=0;i<p;++i) {
		if (ncorrelation[0][i] > 0) {
			t[im] = i;
			f[im] = correlation[0][i]/ncorrelation[0][i] - aux;
			++im;
		}
	}

	// Subsequent correlators
	for (int k=1;k<kmax;++k) {
		for (int i=dmin;i<p;++i) {
			if (ncorrelation[k][i]>0) {
				t[im] = i * pow((double)m, k);
				f[im] = correlation[k][i] / ncorrelation[k][i] - aux;
				++im;
			}
		}
	}

	npcorr = im;
}


