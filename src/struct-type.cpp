#include "struct-type.hpp"

#include <cassert>

str StructType::getName() const
{
	return name;
}

PropertyHandle StructType::addProperty(const str& name)
{
	assert(getProperty(name) == NoProperty);
	
	properties.push_back(name);
	const PropertyHandle handle = properties.size();
	propertyMap[name] = handle;
	return handle;
}

PropertyHandle StructType::getProperty(const str& name) const
{
	if (propertyMap.find(name) != propertyMap.end())
		return propertyMap.at(name);
	return NoProperty;
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
	assert(getMember(name) == NoMember);

	members.push_back({name, type});
	const MemberHandle handle = members.size();
	memberMap[name] = handle;
	return handle;
}

MemberHandle StructType::getMember(const str& name) const
{
	if (memberMap.find(name) != memberMap.end())
		return memberMap.at(name);
	return NoMember;
}

const StructType* StructType::getMemberType(const str& name) const
{
	assert(getMember(name) != NoMember);

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

void StructType::addMemberEquality(const DeepMemberHandle& handle0, const DeepMemberHandle& handle1)
{
	assert(getDeepMemberType(handle0));
	assert(getDeepMemberType(handle1));
	memberEqualities.push_back({handle0, handle1});
}

void StructType::addPropertyEquality(const DeepPropertyHandle& p0, const DeepPropertyHandle& p1)
{
	assert(checkDeepPropertyValid(p0));
	assert(checkDeepPropertyValid(p1));

	propertyEqualities.push_back({p0, p1});
}

void StructType::addPropertyRelations(const PropertyRelations& newRelations)
{
	for (const vec<DeepProperty>& propOr : newRelations)
		for (const DeepProperty& prop : propOr)
			assert(checkDeepPropertyValid(prop.handle));
	relations.insert(relations.end(), newRelations.begin(), newRelations.end());
}

void StructType::addPromotion(const PropertyHandle propertyHandle, const StructType* const promoteTo)
{
	promotions.push_back({ propertyHandle, promoteTo });
}

bool StructType::isNameUsed(const str& name) const
{
	return getMember(name) || getProperty(name);
}

const StructType* StructType::getDeepMemberType(const DeepMemberHandle& handle) const
{
	const StructType* type = this;
	for (const MemberHandle& mHandle : handle)
	{
		if (mHandle > type->members.size())
		{
			type = nullptr;
			break;
		}
		else
			type = type->members[mHandle - 1].second;
	}
	return type;
}

bool StructType::checkDeepPropertyValid(const DeepPropertyHandle& handle)
{
	const StructType* parentType = getDeepMemberType(handle.memberPath);
	return parentType && handle.pHandle <= parentType->properties.size();
}