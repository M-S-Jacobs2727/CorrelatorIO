#include "correlator.h"
#include <math.h>

/////////////////////////////////////////
// Correlator class
/////////////////////////////////////////
Correlator::Correlator ()
{
	init();
}

Correlator::Correlator(const uint32_t numcorrin, const uint32_t pin, const uint32_t min) : 
		num_correlators(numcorrin), points_per_corr(pin), npoints_to_avg(min) 
{
	init();
}

void Correlator::init()
{
	min_dist_bt_points = points_per_corr / npoints_to_avg;  // 8

	vector_size = num_correlators*points_per_corr; // 512

	shift.resize(num_correlators);
	correlation.resize(num_correlators);
	ncorrelation.resize(num_correlators);
	accumulator.resize(num_correlators);
	naccumulator.resize(num_correlators);
	insertindex.resize(num_correlators);

	for (auto s : shift)
		s.resize(points_per_corr);

	for (auto corr : correlation)
		corr.resize(points_per_corr);

	for (auto ncorr : ncorrelation)
		ncorr.resize(points_per_corr);

	step.resize(vector_size);
	result.resize(vector_size);
}


// void Correlator::initialize()
// {

// 	for (unsigned int j=0;j<num_correlators;++j) {  //32
// 		for (unsigned int i=0;i<points_per_corr;++i) {  //16
// 			shift[j][i] = -2E10;
// 			correlation[j][i] = 0;
// 			ncorrelation[j][i] = 0;
// 		}
// 		accumulator[j] = 0.0;
// 		naccumulator[j] = 0;
// 		insertindex[j] = 0;
// 	}

// 	for (unsigned int i=0;i<vector_size;++i) {
// 		step[i] = 0;
// 		result[i] = 0;
// 	}

// 	num_points_out =0;
// 	num_correlators_used=0;
// 	accumulated_values=0;
// }

void Correlator::add(const double w, const unsigned int k)
{

	/// If we exceed the correlator side, the value is discarded
	if (k == num_correlators) return;
	if (k > num_correlators_used) num_correlators_used=k;

	/// Insert new value in shift array
	shift[k][insertindex[k]] = w;

	/// Add to average value
	if (k==0)
		accumulated_values += w;

	/// Add to accumulator and, if needed, add to next correlator
	accumulator[k] += w;
	++naccumulator[k];
	if (naccumulator[k]==npoints_to_avg) {
		add(accumulator[k]/npoints_to_avg, k+1);
		accumulator[k]=0;
		naccumulator[k]=0;
	}

	/// Calculate correlation function
	unsigned int ind1=insertindex[k];
	if (k==0) { /// First correlator is different
		int ind2=ind1;
		for (unsigned int j=0;j<points_per_corr;++j) {  //16
			if (shift[k][ind2] > -1e10) {
				correlation[k][j]+= shift[k][ind1]*shift[k][ind2];
				++ncorrelation[k][j];
			}
			--ind2;
			if (ind2<0) ind2+=points_per_corr;				
		}
	}
	else {
		int ind2=ind1-min_dist_bt_points;
		for (unsigned int j=min_dist_bt_points;j<points_per_corr;++j) {  //[8, 16]
			if (ind2<0) ind2+=points_per_corr;				
			if (shift[k][ind2] > -1e10) {
				correlation[k][j]+= shift[k][ind1]*shift[k][ind2];
				++ncorrelation[k][j];				
			}
			--ind2;
		}
	}

	++insertindex[k];
	if (insertindex[k]==points_per_corr) insertindex[k]=0;
}

void Correlator::evaluate(const bool norm)
{
	unsigned int im=0;

	double aux=0;
	if (norm)
		aux = (accumulated_values/ncorrelation[0][0])*(accumulated_values/ncorrelation[0][0]);

	// First correlator
	for (unsigned int i=0;i<points_per_corr;++i) {
		if (ncorrelation[0][i] > 0) {
			step[im] = i;
			result[im] = correlation[0][i]/ncorrelation[0][i] - aux;
			++im;
		}
	}

	// Subsequent correlators
	for (int k=1;k<num_correlators_used;++k) {
		for (int i=min_dist_bt_points;i<points_per_corr;++i) {
			if (ncorrelation[k][i]>0) {
				step[im] = i * pow((double)npoints_to_avg, k);
				result[im] = correlation[k][i] / ncorrelation[k][i] - aux;
				++im;
			}
		}
	}

	num_points_out = im;
}
