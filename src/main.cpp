#include <fstream>
#include <iostream>

#include "parse-utils.hpp"
#include "parser.hpp"
#include "print.hpp"

int main()
{
	Universe universe;
	std::ifstream typesSrc("../../data/types");
	ErrorReporter er(std::cout);
	parse(universe, typesSrc, er);
	universe.preprocess();
	//std::cout << universe.getType("set")->getPossibleInstancesCount() << endl;
	for (const auto& tp : universe.getTypes())
	{
		std::cout << tp->getName() << ": " << tp->getDeepPropertyDistinctCount() << "/" << tp->getDeepPropertyFullCount()
			<< " " << tp->getDeepMemberDistinctCount() << "/" << tp->getDeepMemberFullCount()
			<< " " << tp->getFlatRelationCount()
			<< " " << tp->getPossibleInstancesCount()
			<< std::endl;
	}
}