#pragma once

#include <unordered_map>

#include "ptr.hpp"
#include "str.hpp"
#include "umap.hpp"
#include "vec.hpp"

#include "struct-type.hpp"

class Universe
{
public:
	void addType(const str& name);

	// returns nullptr if such type doesn't exist
	StructType* getType(const str& name) const;



private:
	vec<uptr<StructType>> typesOwn;
	umap<str, StructType*> types;
};