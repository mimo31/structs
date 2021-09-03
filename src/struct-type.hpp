#pragma once

#include "str.hpp"
#include "umap.hpp"
#include "vec.hpp"

typedef uint32_t PropertyHandle;

typedef uint32_t MemberHandle;

using std::pair;

typedef vec<MemberHandle> DeepMemberHandle;

namespace std
{

template<>
struct hash<DeepMemberHandle>
{
	size_t operator()(const DeepMemberHandle& handle) const
	{
		size_t hsh = 9823453;
		for (const MemberHandle& partial : handle)
		{
			hsh += partial + partial * hsh + 294587;
		}
		return hsh;
	}
};

}

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
	size_t getDeepPropertyFullCount() const;
	size_t getDeepPropertyDistinctCount() const;

	MemberHandle addMember(const str& name, StructType* type);
	MemberHandle getMember(const str& name) const;
	const StructType* getMemberType(const str& name) const;
	const StructType* getMemberType(MemberHandle handle) const;
	const StructType* getDeepMemberType(const DeepMemberHandle& handle) const;
	str getMemberName(MemberHandle handle) const;
	size_t getMemberCount() const;
	size_t getDeepMemberFullCount() const;
	size_t getDeepMemberDistinctCount() const;

	void addMemberEquality(const DeepMemberHandle& handle0, const DeepMemberHandle& handle1);
	void addPropertyEquality(const DeepPropertyHandle& p0, const DeepPropertyHandle& p1);
	void addPropertyRelations(const PropertyRelations& newRelations);
	void addPromotion(PropertyHandle propertyHandle, const StructType* promoteTo);

	bool isNameUsed(const str& name) const;

	void preprocess();
	bool isPreprocessed() const;

	// TODO: add a method for processing the added equalities and relations (to be called after the analysis of the sources)
	// (probably building some union-find; and doing that for recursively from the lowest/simplest types)

	uint32_t getDeepMemberId(const DeepMemberHandle& handle) const;
	uint32_t getDeepMemberCount() const;

private:
	str name;

	vec<str> properties;
	umap<str, PropertyHandle> propertyMap;

	vec<pair<str, StructType*>> members;
	umap<str, MemberHandle> memberMap;

	vec<pair<DeepMemberHandle, DeepMemberHandle>> memberEqualities;

	vec<pair<DeepPropertyHandle, DeepPropertyHandle>> propertyEqualities;

	// each relations says that the OR of the specified properties is true
	PropertyRelations relations;

	vec<pair<PropertyHandle, const StructType*>> promotions;

	bool preprocessed = false;

	vec<vec<uint32_t>> deepMemberGroup;
	vec<vec<pair<uint32_t, uint32_t>>> deepMemberGroups;
	vec<StructType*> deepMemberType;

	uint32_t getDeepMemberGroup(const DeepMemberHandle& handle) const;
	uint32_t getDeepMemberIndex(const DeepMemberHandle& handle) const;

	uint32_t getDeepMemberGroup(const DeepMemberHandle& handle, uint32_t endGroupNumber) const;
	uint32_t getDeepMemberIndex(const DeepMemberHandle& handle, uint32_t endGroupNumber) const;

	void preprocessMemberEqualities();

	vec<vec<uint32_t>> deepPropertyGroup;
	vec<vec<pair<uint32_t, uint32_t>>> deepPropertyGroups;

	uint32_t getDeepPropertyIndex(const DeepPropertyHandle& handle) const;
	uint32_t getDeepPropertyIndex(const DeepMemberHandle& path, uint32_t propertyIndex) const;

	void preprocessPropertyEqualities();

	bool checkDeepPropertyValid(const DeepPropertyHandle& handle);
};