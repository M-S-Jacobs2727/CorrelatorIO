#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "correlator.hpp"

int main(int argc, char *argv[])
{
	// Get command-line args
	if (argc < 3)
	{
		std::cout << "Usage:\n" << argv[0] << " <input-file> <output-file>\n";
		return 1;
	}

	auto infile = argv[1];
	auto outfile = argv[2];

	std::ifstream fin;
	double val;
	fin.open(infile);

	if(!fin.is_open() || fin.bad())
	{
		std::cerr << "Error opening input file " << infile << '\n';
		return 1;
	}

	// Get number of columns for rows after comment(s)
	std::string line;
	uint32_t num_cols = 0;
	auto linestart = fin.tellg();
	while (fin.good())
	{
		linestart = fin.tellg();
		std::string word;
		fin >> word;
		if (word[0] == '#')
		{
			fin.ignore(512, '\n');
		}
		else
		{
			fin.seekg(linestart);
			std::getline(fin, line);
			std::istringstream iss(line);

			while (iss.good())
			{
				iss >> word;
				if (word.size())
					++num_cols;
			}
			break;
		}
	}
    if (num_cols == 0)
    {
        std::cerr << "ERROR: No data columns found.\n";
        return 1;
    }

	std::vector<Correlator> correlators(num_cols - 1);

	fin.seekg(linestart);
	
	while(!fin.eof())
	{
        std::string word;
		fin >> word; // time
		for (auto& c : correlators)
		{
			fin >> val;
			c.add(val);
		}
	}
	
	fin.close();

	for (auto& c : correlators)
		c.evaluate();

	std::ofstream fout;
	fout.open(outfile);
	
	if(!fout.is_open() || fout.bad())
	{
		std::cerr << "Error opening output file " << outfile << '\n';
		return 1;
	}

	correlators[0].computeSteps();
	auto& step = correlators[0].step;
	
	for (uint32_t i = 0; i < step.size(); ++i)
	{
		fout << step[i];
		for (const auto& c : correlators)
			fout << ' ' << c.result[i];
		fout << '\n';
	}

	fout.close();

	return 0;
}
