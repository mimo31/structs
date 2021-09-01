#include <fstream>
#include <iostream>

#include "parser.hpp"

int main()
{
	Universe universe;
	std::ifstream typesSrc("../../data/types");
	parse(universe, typesSrc, std::cout);
}