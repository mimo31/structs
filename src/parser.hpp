#pragma once

#include <iosfwd>

#include "universe.hpp"

using std::istream;
using std::ostream;

void parse(Universe& universe, istream& defs, ErrorReporter& er);