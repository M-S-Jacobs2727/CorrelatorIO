#include <iostream>
#include <fstream>

#include "correlator.h"

int main(int argc, char *argv[]) {

	if (argc < 3) {
		std::cout << "Usage:\n" << argv[0] << " <input-file> <output-file>\n";
		return 1;
	}

	auto infile = argv[1];
	auto outfile = argv[2];

	Correlator c(32, 16, 2);
	c.initialize();

	std::ifstream fin;
	double val;
	fin.open(infile);

	if(!fin.is_open() || fin.bad()) {
		std::cerr << "Error opening input file " << infile << '\n';
		return 1;
	}
	
	while(!fin.eof()) {
		fin >> val;
		c.add(val);
	}
	
	fin.close();

	c.evaluate();

	std::ofstream fout;
	fout.open(outfile);
	
	if(!fout.is_open() || fout.bad()) {
		std::cerr << "Error opening output file " << outfile << '\n';
		return 1;
	}
	
	for (unsigned int i = 0; i < c.npcorr; ++i)
		fout << c.t[i] << ' ' << c.f[i] << '\n';
	
	fout.close();

	return 0;
}
