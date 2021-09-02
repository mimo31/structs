#pragma once

#include "str.hpp"
#include "umap.hpp"
#include "vec.hpp"

typedef uint32_t PropertyHandle;

typedef uint32_t MemberHandle;

using std::pair;

typedef vec<MemberHandle> DeepMemberHandle;

struct DeepPropertyHandle
{
	vec<MemberHandle> memberPath;
	PropertyHandle pHandle;
};

struct DeepProperty
{
	DeepPropertyHandle handle;
	bool negated;

	DeepProperty() = default;
	DeepProperty(const DeepPropertyHandle& handle) : handle(handle), negated(false)
	{
	}
	DeepProperty(const DeepPropertyHandle& handle, const bool negated) : handle(handle), negated(negated)
	{
	}
};

typedef vec<vec<DeepProperty>> PropertyRelations;

class StructType
{
public:
	static constexpr PropertyHandle NoProperty = 0;
	static constexpr MemberHandle NoMember = 0;

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
	const StructType* getDeepMemberType(const DeepMemberHandle& handle) const;
	str getMemberName(MemberHandle handle) const;
	size_t getMemberCount() const;

	void addMemberEquality(const DeepMemberHandle& handle0, const DeepMemberHandle& handle1);
	void addPropertyEquality(const DeepPropertyHandle& p0, const DeepPropertyHandle& p1);
	void addPropertyRelations(const PropertyRelations& newRelations);
	void addPromotion(PropertyHandle propertyHandle, const StructType* promoteTo);

	bool isNameUsed(const str& name) const;

	// TODO: add a method for processing the added equalities and relations (to be called after the analysis of the sources)
	// (probably building some union-find; and doing that for recursively from the lowest/simplest types)

private:
	str name;

	vec<str> properties;
	umap<str, PropertyHandle> propertyMap;

	vec<pair<str, const StructType*>> members;
	umap<str, MemberHandle> memberMap;

	vec<pair<DeepMemberHandle, DeepMemberHandle>> memberEqualities;

	vec<pair<DeepPropertyHandle, DeepPropertyHandle>> propertyEqualities;

	// each relations says that the OR of the specified properties is true
	PropertyRelations relations;

	vec<pair<PropertyHandle, const StructType*>> promotions;

	bool checkDeepPropertyValid(const DeepPropertyHandle& handle);
};