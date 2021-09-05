#include "universe.hpp"

void Universe::addType(const str& name)
{
	typesOwn.push_back(make_unique<StructType>(name));
	types[name] = typesOwn.back().get();
}

StructType* Universe::getType(const str& name) const
{
	if (types.find(name) != types.end())
		return types.at(name);
	return nullptr;
}

const vec<uptr<StructType>>& Universe::getTypes() const
{
	return typesOwn;
}

void Universe::precheck(ErrorReporter& er)
{
	for (const auto& tp : typesOwn)
		tp->precheck(er);
}

void Universe::preprocess()
{
	for (const auto& tp : typesOwn)
	{
		if (!tp->isPreprocessed())
			tp->preprocess();
	}
}