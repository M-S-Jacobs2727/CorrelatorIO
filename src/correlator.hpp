#pragma once

#include <cstdint>
#include <vector>

class Correlator
{
public:
	Correlator();
	Correlator(const uint32_t, const uint32_t, const uint32_t);

	void add(const double w, const unsigned int k = 0);
	void evaluate();
	void computeSteps();
public:
	//TODO: change to time, based on dt in input file
	std::vector<uint32_t> step;
	std::vector<double> result;

private:
	void init();
	uint32_t pow(uint32_t, uint32_t);
private:
	uint32_t m_num_correlators{0}, m_points_per_corr{0}, m_npoints_to_avg{0};
	uint32_t m_min_dist_bt_points{0}, m_vector_size{0}, m_num_correlators_used{0};
	bool m_evaluated = false;

	std::vector<std::vector<double>> m_shift_mat, m_correlation_mat;
	std::vector<std::vector<uint64_t>> m_num_vals_mat;

	std::vector<double> m_accumulator_vec;
	std::vector<uint32_t> m_corr_idx_vec, m_corr_insert_idx_vec;
};
