#include <fstream>
#include <iostream>

#include "parser.hpp"

int main()
{
	Universe universe;
	std::ifstream typesSrc("../../data/types");
	parse(universe, typesSrc, std::cout);
	universe.preprocess();

	for (const auto& tp : universe.getTypes())
	{
		std::cout << tp->getName() << ": " << tp->getDeepPropertyDistinctCount() << "/" << tp->getDeepPropertyFullCount() << " " << tp->getDeepMemberDistinctCount() << "/" << tp->getDeepMemberFullCount() << std::endl;
	}
}