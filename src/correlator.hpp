/// Definition of correlator classes
#ifndef __correlator_h
#define __correlator_h

#include <vector>

////////////////////////////////////////////////////
/// Standard Scalar Correlator result(tau)=<A(step)A(step+tau)>
class Correlator {

public:
	/** Constructor */
	Correlator ();
	Correlator (const uint32_t numcorrin, const uint32_t pin, const uint32_t min);
	~Correlator() = default;

	/** Add a scalar to the correlator number k */
	void add(const double w, const unsigned int k = 0);

	/** Evaluate the current state of the correlator */
	void evaluate(const bool norm = false);

	/** Initialize all values (current and average) to zero */
	// void initialize();

public:
	std::vector<double> step, result;
	uint32_t num_points_out = 0;

private:
	/** Set size of correlator */
	void init ();
private:
	/** points_per_corr: Points per correlator */
	uint32_t num_correlators = 32;
	uint32_t points_per_corr = 16;
	uint32_t npoints_to_avg = 2;
	uint32_t min_dist_bt_points{0}, vector_size{0}, num_correlators_used{0};
	/** m: Number of points over which to average; RECOMMENDED: points_per_corr mod m = 0 */

	/** Accumulated result of incoming values **/
	double accumulated_values = 0.0;

	/** Where the coming values are stored */
	std::vector<std::vector<double>> shift, correlation;
	/** Array containing the actual calculated correlation function */
	// double **correlation;
	/** Number of values accumulated in corr */
	std::vector<std::vector<uint64_t>> ncorrelation;

	/** Accumulator in each correlator */
	std::vector<double> accumulator;
	/** Index that controls accumulation in each correlator */
	std::vector<uint32_t> naccumulator, insertindex;
	/** Index pointing at the position at which the current value is inserted */
	// unsigned int *insertindex;

	/** Number of Correlators */
	// unsigned int num_correlators;

	/** Minimum distance between points for correlators k>0; min_dist_bt_points = points_per_corr/m */
	// uint32_t min_dist_bt_points, length, num_correlators_used;

	/*  SCHEMATIC VIEW OF EACH CORRELATOR
						        points_per_corr=N
		<----------------------------------------------->
		_________________________________________________
		|0|1|2|3|.|.|.| | | | | | | | | | | | | | | |N-1|
		-------------------------------------------------
		*/

	/** Lenght of result arrays */
	// unsigned int length;
	/** Maximum correlator attained during simulation */
	// unsigned int num_correlators_used;

};

#endif
