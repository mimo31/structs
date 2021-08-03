#include "struct-type.hpp"

#include <cassert>

str StructType::getName() const
{
	return name;
}

PropertyHandle StructType::addProperty(const str& name)
{
	assert(getProperty(name) == noProperty);
	
	properties.push_back(name);
	const PropertyHandle handle = properties.size();
	propertyMap[name] = handle;
	return handle;
}

PropertyHandle StructType::getProperty(const str& name) const
{
	if (propertyMap.find(name) != propertyMap.end())
		return propertyMap.at(name);
	return noProperty;
}

str StructType::getPropertyName(const PropertyHandle handle) const
{
	assert(handle > 0);
	assert(handle <= properties.size());

	return properties[handle - 1];
}

size_t StructType::getPropertyCount() const
{
	return properties.size();
}

MemberHandle StructType::addMember(const str& name, const StructType* const type)
{
	assert(getMember(name) == noMember);

	members.push_back({name, type});
	const MemberHandle handle = members.size();
	memberMap[name] = handle;
	return handle;
}

MemberHandle StructType::getMember(const str& name) const
{
	if (memberMap.find(name) != memberMap.end())
		return memberMap.at(name);
	return noMember;
}

const StructType* StructType::getMemberType(const str& name) const
{
	assert(getMember(name) != noMember);

	return getMemberType(getMember(name));
}

const StructType* StructType::getMemberType(const MemberHandle handle) const
{
	assert(handle > 0);
	assert(handle <= members.size());

	return members[handle - 1].second;
}

str StructType::getMemberName(const MemberHandle handle) const
{
	assert(handle > 0);
	assert(handle <= members.size());

	return members[handle - 1].first;
}

size_t StructType::getMemberCount() const
{
	return members.size();
}