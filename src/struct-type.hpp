#pragma once

#include "str.hpp"
#include "umap.hpp"

typedef uint32_t PropertyHandle;

typedef uint32_t MemberHandle;

using std::pair;

typedef vec<MemberHandle> DeepMemberHandle;

typedef vec<PropertyHandle> DeepPropertyHandle;

struct DeepProperty
{
	DeepPropertyHandle handle;
	bool negated;
};

class StructType
{
public:
	static constexpr PropertyHandle noProperty = 0;
	static constexpr MemberHandle noMember = 0;

	str name;

	StructType(const str& name) : name(name)
	{}

	str getName() const;

	PropertyHandle addProperty(const str& name);
	PropertyHandle getProperty(const str& name) const;
	str getPropertyName(PropertyHandle handle) const;
	size_t getPropertyCount() const;

	MemberHandle addMember(const str& name, const StructType* type);
	MemberHandle getMember(const str& name) const;
	const StructType* getMemberType(const str& name) const;
	const StructType* getMemberType(MemberHandle handle) const;
	str getMemberName(MemberHandle handle) const;
	size_t getMemberCount() const;


	// TODO: already (pre)compute the connected components when adding member or property equalities

	void addMemberEquality(const DeepMemberHandle& handle0, const DeepMemberHandle& handle1);

	// TODO: add methods for adding property equality and property relations



private:
	str name;

	vec<str> properties;
	umap<str, PropertyHandle> propertyMap;

	vec<pair<str, const StructType*>> members;
	umap<str, MemberHandle> memberMap;

	// change the two containers below to already store the equality-connected components

	vec<pair<DeepMemberHandle, DeepMemberHandle>> memberEqualities;

	vec<pair<DeepPropertyHandle, DeepPropertyHandle>> propertyEqualities;

	vec<vec<DeepProperty>> relations;
};