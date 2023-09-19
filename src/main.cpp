#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "correlator.hpp"
// TODO: refactoring ingestFile for reading, then OMP helps with interpreting text to doubles (custom algo?)
constexpr auto FileError = 1;
constexpr auto CommandLineError = 2;
constexpr auto RangeStringError = 3;

class Timer
{
public:
    void start()
    {
        m_start = std::chrono::steady_clock::now();
    }
    void end()
    {
        const auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double> num_seconds = end - m_start;
        std::cerr << num_seconds.count() << '\n';
    }
private:
    std::chrono::time_point<std::chrono::steady_clock> m_start;
};

void help()
{
    std::cerr
        << "Usage: CorrelatorIO [-h] -i <infile> -o <outfile> [-t <timestep>] [-f <fluct_cols>] [-s <skip_cols>] [-c <corr_values>]\n"
        << "\t-c <corr_values>  Must be three comma-separated values: number of correlators,\n"
        << "\t                  length of each correlator, number of values to average before\n"
        << "\t                  adding to the correlator. Default values: 32,16,2\n";
}

struct Options
{
    std::string infile;
    std::string outfile;

    std::vector<uint32_t> fluct_columns;
    std::vector<uint32_t> cols_to_skip = { 0 };

    double timestep = 1.0;
    uint32_t num_correlators = 32;
    uint32_t correlator_length = 16;
    uint32_t num_to_average = 2;
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
            else if (item[i] == '-')
            {
                hyphenIdx = i;
                break;
            }
            else if (item[i] < '0' || item[i] > '9')
            {
                std::cerr << "Invalid range string: " << rangeString << '\n';
                exit(RangeStringError);
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

void sortAndUnique(std::vector<uint32_t>& values)
{
    if (!std::is_sorted(values.begin(), values.end()))
        std::sort(values.begin(), values.end());

    std::unique(values.begin(), values.end());
}

Options parseArgs(int argc, char* argv[])
{
    Options options;
    std::vector<uint32_t> corr_options;

    for (int i = 1; i < argc; ++i)
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
            sortAndUnique(options.fluct_columns);
            break;
        case 's':
            options.cols_to_skip = parseRangeString(arg);
            sortAndUnique(options.cols_to_skip);
            break;
        case 't':
            options.timestep = std::stod(arg);
            if (options.timestep <= 0.0)
            {
                std::cerr << "Invalid timestep value: " << arg << '\n';
                exit(CommandLineError);
            }
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
            options.num_to_average = corr_options[2];
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

std::vector<Correlator> ingestFile(Options options)
{
    Timer t;
    t.start();

    std::ifstream fin(options.infile);

    if (!fin.good())
    {
        std::cerr << "Error opening input file " << options.infile << '\n';
        exit(FileError);
    }
    uint32_t total_num_cols = getNumCols(fin);
    uint32_t num_cols = total_num_cols - options.cols_to_skip.size();

    std::vector<std::vector<std::string>> text;

    text.resize(num_cols);
    for (auto& t : text)
        t.reserve(10000);

    auto skip_begin = options.cols_to_skip.begin();
    auto skip_end = options.cols_to_skip.end();

    while (!fin.eof())
    {
        auto skip = skip_begin;
        uint32_t col = 0;
        for (uint32_t i = 0; i < total_num_cols; ++i)
        {
            std::string temp;
            fin >> temp;

            if (skip_end != skip && *skip == i)
            {
                ++skip;
            }
            else
            {
                text[col].push_back(temp);
                ++col;
            }
        }
    }
    fin.close();
    std::cerr << "Reading file: ";
    t.end();

    auto num_rows = text[0].size();

    std::vector<std::vector<double>> data;
    data.resize(num_cols);
    for (auto& d : data)
        d.resize(num_rows);

    t.start();
#pragma omp parallel for collapse(2)
    for (uint32_t i = 0; i < num_cols; ++i)
        for (uint32_t j = 0; j < num_rows; ++j)
            if (text[i][j].size())
                data[i][j] = std::stod(text[i][j]);
    
    std::cerr << "Parsing text: ";
    t.end();

    auto& last_col = data.back();
    num_rows = std::find(last_col.begin(), last_col.end(), 0.0) - last_col.begin();
    num_rows = 0;
    while (data.back()[num_rows])
        ++num_rows;

    for (auto& d : data)
        d.resize(num_rows);

    t.start();
    for (uint32_t c : options.fluct_columns)
    {
        double mean = 0.0;
#pragma omp parallel for reduction(+:mean)
        for (double x : data[c-1])
            mean += x;
        mean /= data[c-1].size();

#pragma omp parallel for
        for (double& x : data[c])
            x -= mean;
    }
    std::cerr << "Removing means from columns: ";
    t.end();

    std::vector<Correlator> correlators;
    correlators.reserve(num_cols);

    for (uint32_t i = 0; i < num_cols; ++i)
        correlators.emplace_back(options.num_correlators, options.correlator_length, options.num_to_average);
    
    t.start();
#pragma omp parallel for num_threads(num_cols)
    for (uint32_t i = 0; i < num_cols; ++i)
        for (uint32_t j = 0; j < num_rows; ++j)
            correlators[i].add(data[i][j]);

    std::cerr << "Populating correlators: ";
    t.end();

    return correlators;
}

void writeOutfile(std::string& outfile, double timestep, std::vector<Correlator>& correlators)
{
    std::ofstream fout(outfile);

    if (fout.bad())
    {
        std::cerr << "Error opening output file " << outfile << '\n';
        exit(FileError);
    }

    fout.setf(std::ios::scientific);
    fout.precision(8);
    auto timesteps = correlators[0].computeTimesteps(timestep);

    for (uint32_t i = 0; i < timesteps.size(); ++i)
    {
        fout << timesteps[i];
        for (const auto& c : correlators)
            fout << ' ' << std::setw(15) << c.result[i];
        fout << '\n';
    }

    fout.close();
}

int main(int argc, char* argv[])
{
    Timer all;
    all.start();

    Options options = parseArgs(argc, argv);

    std::vector<Correlator> correlators = ingestFile(options);

    Timer t;
    t.start();

#pragma omp parallel for num_threads(correlators.size())
    for (auto& c : correlators)
        c.evaluate();

    std::cerr << "Evaluating correlators: ";
    t.end();

    writeOutfile(options.outfile, options.timestep, correlators);

    std::cerr << "Total time: ";
    all.end();
    return 0;
}
