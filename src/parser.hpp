#pragma once

#include <iosfwd>

#include "universe.hpp"

using std::istream;
using std::ostream;

// returns true iff there was an error
bool parse(Universe& universe, istream& defs, ostream& error);