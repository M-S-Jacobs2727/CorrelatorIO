#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "correlator.hpp"

constexpr auto FileError = 1;
constexpr auto CommandLineError = 2;
constexpr auto RangeStringError = 3;

void help()
{
	std::cerr
		<< "Usage: CorrelatorIO [-h] -i <infile> -o <outfile> [-t <timestep>] [-f <fluct_cols>] [-s <skip_cols>] [-c <corr_values>]\n"
		<< "\t-c <corr_values>	Must be three comma-separated values: number of correlators,\n"
		<< "\t					length of each correlator, number of values to average before\n"
		<< "\t					adding to the correlator. Default values: 32,16,2\n";
}

struct Options
{
	std::string infile;
	std::string outfile;

	std::vector<uint32_t> fluct_columns;
	std::vector<uint32_t> cols_to_skip = { 0 };

	double timestep = 1.0;
	uint32_t num_correlators = 0;
	uint32_t correlator_length = 0;
	uint32_t num_to_average = 0;
};

std::vector<uint32_t> parseRangeString(const std::string& rangeString)
{
	std::vector<uint32_t> values;
	values.reserve(8);

	std::istringstream rss(rangeString);
	std::string item;

	while (std::getline(rss, item, ','))
	{
		uint32_t hyphenIdx = 0;

		for (uint32_t i = 0; i < item.size(); ++i)
		{
			if (item[i] == '-' && (i == 0 || i == item.size() - 1))
			{
				std::cerr << "Invalid range string: " << rangeString << '\n';
				exit(RangeStringError);
			}
			else if (item[i] < '0' || item[i] > '9')
			{
				std::cerr << "Invalid range string: " << rangeString << '\n';
				exit(RangeStringError);
			}
			else if (item[i] == '-')
			{
				hyphenIdx = i;
				break;
			}
		}

		if (hyphenIdx)
		{
			uint32_t rmin = std::stoi(item.substr(0, hyphenIdx));
			uint32_t rmax = std::stoi(item.substr(hyphenIdx + 1));

			if (rmax <= rmin)
			{
				std::cerr << "Invalid range string: " << rangeString << '\n';
				exit(RangeStringError);
			}
			
			for (uint32_t r = rmin; r <= rmax; ++r)
				values.push_back(r);
		}
		else
		{
			values.push_back(std::stoi(item));
		}
	}

	return values;
}

Options parseArgs(int argc, char* argv[])
{
	Options options;
	std::vector<uint32_t> corr_options;

	for (uint32_t i = 0; i < argc; ++i)
	{
		auto flag = argv[i];
		if (flag[0] != '-')
		{
			std::cerr << "Invalid option flag: " << flag << '\n';
			exit(CommandLineError);
		}

		char opt = flag[1];
		if (opt == 'h')
		{
			help();
			exit(0);
		}

		if (i + 1 == argc)
		{
			std::cerr << "Option flag has no argument: -" << opt << '\n';
			exit(CommandLineError);
		}

		std::string arg = argv[++i];

		switch (opt)
		{
		case 'i':
			options.infile = arg;
			break;
		case 'o':
			options.outfile = arg;
			break;
		case 'f':
			options.fluct_columns = parseRangeString(arg);
			break;
		case 's':
			options.cols_to_skip = parseRangeString(arg);
			break;
		case 't':
			options.timestep = std::stod(arg);
			break;
		case 'c':
			corr_options = parseRangeString(arg);
			if (corr_options.size() != 3)
			{
				std::cerr << "Invalid correlator options: -c " << arg << '\n';
				exit(CommandLineError);
			}
			options.num_correlators = corr_options[0];
			options.correlator_length = corr_options[1];
			options.num_to_average= corr_options[2];
			break;
		default:
			std::cerr << "Unrecognized option: -" << opt << '\n';
			exit(CommandLineError);
		}
	}
	return options;
}

uint32_t getNumCols(std::ifstream& fin)
{
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

	fin.seekg(linestart);

	if (num_cols == 0)
	{
		std::cerr << "ERROR: No data columns found.\n";
		exit(FileError);
	}

	return num_cols;
}

int main(int argc, char* argv[])
{
	Options options = parseArgs(argc, argv);

	std::ifstream fin(options.infile);

	if(!fin.good())
	{
		std::cerr << "Error opening input file " << options.infile << '\n';
		exit(FileError);
	}
	uint32_t num_cols = getNumCols(fin);


	std::vector<Correlator> correlators(num_cols - options.cols_to_skip.size());
	while(!fin.eof())
	{
		uint32_t col = options.cols_to_skip[0];
		for (uint32_t i = 0; i < num_cols - 1; ++i)
		{
			if (i == col)
			{
				std::string trash;
				fin >> trash;
			}
			else
			{
				double val;
				fin >> val;
				correlators[i].add(val);
			}
		}
	}

	fin.close();

	for (auto& c : correlators)
		c.evaluate();

	std::ofstream fout(options.outfile);

	if (fout.bad())
	{
		std::cerr << "Error opening output file " << options.outfile << '\n';
		exit(FileError);
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
