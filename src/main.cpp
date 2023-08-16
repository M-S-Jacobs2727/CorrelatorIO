#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "correlator.h"

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
	uint32_t num_cols;
	auto linestart = fin.tell();
	while (fin.good())
	{
		linestart = fin.tell();
		std::string word;
		fin >> word;
		if (word[0] == '#')
		{
			fin.ignore(512, '\n');
		}
		else
		{
			fin.seek(linestart);
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

	std::vector<Correlator> correlators(num_cols - 1);

	fin.seek(linestart);
	
	while(!fin.eof())
	{
		fin >> word; // time
		for (uint32_t i = 0; i < num_cols - 1; ++i)
		{
			fin >> val;
			correlators[i].add(val);
		}
	}
	
	fin.close();

	for (auto c : correlators)
		c.evaluate();

	std::ofstream fout;
	fout.open(outfile);
	
	if(!fout.is_open() || fout.bad())
	{
		std::cerr << "Error opening output file " << outfile << '\n';
		return 1;
	}
	
	for (uint32_t i = 0; i < correlators[0].num_points_out; ++i)
	{
		fout << correlators[0].step[i]
		for (auto c : correlators)
			fout << ' ' << c.result[i] << '\n';
	}

	fout.close();

	return 0;
}
