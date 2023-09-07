#include "correlator.hpp"

Correlator::Correlator ()
{
	m_num_correlators = 32;
	m_points_per_corr = 16;
	m_npoints_to_avg = 2;
	init();
}

Correlator::Correlator(const uint32_t numcorrin, const uint32_t pin, const uint32_t min) : 
		m_num_correlators(numcorrin), m_points_per_corr(pin), m_npoints_to_avg(min) 
{
	init();
}

void Correlator::init()
{
	m_min_dist_bt_points = m_points_per_corr / m_npoints_to_avg;

	m_vector_size = m_num_correlators*m_points_per_corr;

    m_accumulator_vec.resize(m_num_correlators);
	m_corr_idx_vec.resize(m_num_correlators);
	m_corr_insert_idx_vec.resize(m_num_correlators);

	m_shift_mat.resize(m_num_correlators);
	m_correlation_mat.resize(m_num_correlators);
	m_num_vals_mat.resize(m_num_correlators);
	
    for (uint32_t i = 0; i < m_num_correlators; ++i)
    {
        m_shift_mat[i].resize(m_points_per_corr);
        m_correlation_mat[i].resize(m_points_per_corr);
        m_num_vals_mat[i].resize(m_points_per_corr);
    }

	result.resize(m_vector_size);
}

std::vector<double> Correlator::computeTimesteps(double timestep)
{
	std::vector<double> timesteps;

	// TODO: insert return statement here
	if (!m_evaluated)
		return timesteps;

	timesteps.resize(m_vector_size);

	uint32_t k = 0;
	uint32_t start = 0;
	for (uint32_t i = 0; i < m_num_correlators_used; ++i)
	{
		for (uint32_t j = start; j < m_points_per_corr; ++j)
		{
			if (m_num_vals_mat[i][j] != 0)
			{
				timesteps[k] = j * pow(m_npoints_to_avg, i) * timestep;
				++k;
			}
		}
		start = m_min_dist_bt_points;
	}

	timesteps.resize(k);
	return timesteps;
}

uint32_t Correlator::pow(uint32_t base, uint32_t power)
{
	if (power == 0)
		return 1;
	
	if (power == 1)
		return base;
	
	uint32_t result = 1;
	uint32_t power1, power2;
	power1 = power / 2;
	power2 = power - power1;

	return pow(base, power1) * pow(base, power2);
}

void Correlator::add(const double w, const unsigned int k)
{
	if (k == m_num_correlators)
		return;
	if (k > m_num_correlators_used)
		m_num_correlators_used = k;

	m_shift_mat[k][ m_corr_insert_idx_vec[k] ] = w;

	m_accumulator_vec[k] += w;
	++m_corr_idx_vec[k];
	if (m_corr_idx_vec[k] == m_npoints_to_avg)
	{
		add(m_accumulator_vec[k] / m_npoints_to_avg, k + 1);
		m_accumulator_vec[k] = 0;
		m_corr_idx_vec[k] = 0;
	}

	uint32_t ind1 = m_corr_insert_idx_vec[k];
	if (k == 0) 
	{
		int ind2 = ind1;
		for (uint32_t j = 0; j < m_points_per_corr; ++j)
		{
			if (m_shift_mat[k][ind2] > -1e10) {
				m_correlation_mat[k][j] += m_shift_mat[k][ind1] * m_shift_mat[k][ind2];
				++m_num_vals_mat[k][j];
			}
			--ind2;
			if (ind2 < 0)
				ind2 += m_points_per_corr;
		}
	}
	else
	{
		int ind2 = ind1 - m_min_dist_bt_points;
		for (uint32_t j = m_min_dist_bt_points; j < m_points_per_corr; ++j)
		{
			if (ind2 < 0)
				ind2 += m_points_per_corr;
			if (m_shift_mat[k][ind2] > -1e10)
			{
				m_correlation_mat[k][j] += m_shift_mat[k][ind1] * m_shift_mat[k][ind2];
				++m_num_vals_mat[k][j];
			}
			--ind2;
		}
	}

	++m_corr_insert_idx_vec[k];
	if (m_corr_insert_idx_vec[k] == m_points_per_corr)
		m_corr_insert_idx_vec[k] = 0;
}

void Correlator::evaluate()
{
	uint32_t k = 0;

	uint32_t start = 0;
	for (uint32_t i = 0; i < m_num_correlators_used; ++i)
	{
		for (uint32_t j = start; j < m_points_per_corr; ++j)
		{
			if (m_num_vals_mat[i][j] != 0)
			{
				result[k] = m_correlation_mat[i][j] / m_num_vals_mat[i][j];
				++k;
			}
		}
		start = m_min_dist_bt_points;
	}
	result.resize(k);
	m_evaluated = true;
}
