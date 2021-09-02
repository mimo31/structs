#include "struct-type.hpp"

#include <algorithm>
#include <cassert>
#include <unordered_set>

#include "print.hpp"

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

size_t StructType::getDeepPropertyFullCount() const
{
	size_t sm = getPropertyCount();
	for (const auto& m : members)
		sm += m.second->getDeepPropertyFullCount();
	return sm;
}

size_t StructType::getDeepPropertyDistinctCount() const
{
	// TODO: implement
	return 0;
}

MemberHandle StructType::addMember(const str& name, StructType* const type)
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

size_t StructType::getDeepMemberFullCount() const
{
	size_t sm = getMemberCount();
	for (const auto& m : members)
		sm += m.second->getDeepMemberFullCount();
	return sm;
}

size_t StructType::getDeepMemberDistinctCount() const
{
	return deepMemberGroups.size();
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

void StructType::preprocess()
{
	for (const auto& m : members)
		if (!m.second->preprocessed)
			m.second->preprocess();
	preprocessMemberEqualities();
}

bool StructType::isPreprocessed() const
{
	return preprocessed;
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

uint32_t StructType::getDeepMemberGroup(const DeepMemberHandle& handle) const
{
	assert(!handle.empty());
	DeepMemberHandle tail;
	tail.insert(tail.end(), handle.begin() + 1, handle.end());
	return deepMemberGroup[handle.front() - 1][members[handle.front() - 1].second->getDeepMemberIndex(tail)];
}

uint32_t StructType::getDeepMemberIndex(const DeepMemberHandle& handle) const
{
	if (handle.empty())
		return 0;
	return getDeepMemberGroup(handle) + 1;
}

uint32_t StructType::getDeepMemberGroup(const DeepMemberHandle& handle, const uint32_t endGroupNumber) const
{
	if (handle.empty())
		return endGroupNumber;
	DeepMemberHandle tail;
	tail.insert(tail.end(), handle.begin() + 1, handle.end());
	return deepMemberGroup[handle.front() - 1][members[handle.front() - 1].second->getDeepMemberIndex(tail, endGroupNumber)];
}

uint32_t StructType::getDeepMemberIndex(const DeepMemberHandle& handle, const uint32_t endGroupNumber) const
{
	return getDeepMemberGroup(handle, endGroupNumber) + 1;
}

void StructType::preprocessMemberEqualities()
{
	vec<vec<vec<pair<uint32_t, uint32_t>>>> neighbors;
	for (uint32_t mi = 0; mi < getMemberCount(); mi++)
	{
		neighbors.push_back(vec<vec<pair<uint32_t, uint32_t>>>(members[mi].second->deepMemberGroups.size() + 1));
	}
	for (const auto& eq : memberEqualities)
	{
		const uint32_t member0 = eq.first.front() - 1;
		DeepMemberHandle tail0;
		tail0.insert(tail0.end(), eq.first.begin() + 1, eq.first.end());
		const uint32_t dMember0Index = members[member0].second->getDeepMemberIndex(tail0);
		const pair<uint32_t, uint32_t> pr0{ member0, dMember0Index };

		StructType* const tp = tail0.empty() ? members[member0].second : members[member0].second->deepMemberType[dMember0Index - 1];

		const uint32_t member1 = eq.second.front() - 1;
		DeepMemberHandle tail1;
		tail1.insert(tail1.end(), eq.second.begin() + 1, eq.second.end());
		const uint32_t dMember1Index = members[member1].second->getDeepMemberIndex(tail1);
		const pair<uint32_t, uint32_t> pr1{ member1, dMember1Index };

		neighbors[member0][dMember0Index].push_back(pr1);
		neighbors[member1][dMember1Index].push_back(pr0);

		for (uint32_t i = 0; i < tp->deepMemberGroups.size(); i++)
		{
			const uint32_t ind0 = members[member0].second->getDeepMemberIndex(tail0, i);
			const uint32_t ind1 = members[member1].second->getDeepMemberIndex(tail1, i);

			neighbors[member0][ind0].push_back({ member1, ind1 });
			neighbors[member1][ind1].push_back({ member0, ind0 });
		}
	}

	deepMemberGroup.reserve(getMemberCount());
	constexpr uint32_t notset = std::numeric_limits<uint32_t>::max();
	for (uint32_t mi = 0; mi < getMemberCount(); mi++)
	{
		deepMemberGroup.push_back(vec<uint32_t>(members[mi].second->deepMemberGroups.size() + 1, notset));
	}
	const auto bfs = [&](const uint32_t mi, const uint32_t dm, const auto& bfs_fun) -> void
	{
		deepMemberGroups.back().push_back({ mi, dm });
		deepMemberGroup[mi][dm] = deepMemberGroups.size() - 1;
		for (const pair<uint32_t, uint32_t>& nei : neighbors[mi][dm])
			if (deepMemberGroup[nei.first][nei.second] == notset)
				bfs_fun(nei.first, nei.second, bfs_fun);
	};
	for (uint32_t mi = 0; mi < getMemberCount(); mi++)
	{
		for (uint32_t dm = 0; dm <= members[mi].second->deepMemberGroups.size(); dm++)
		{
			if (deepMemberGroup[mi][dm] == notset)
			{
				deepMemberGroups.push_back({});
				deepMemberType.push_back(dm == 0 ? members[mi].second : members[mi].second->deepMemberType[dm - 1]);
				bfs(mi, dm, bfs);
			}
		}
	}
}

bool StructType::checkDeepPropertyValid(const DeepPropertyHandle& handle)
{
	const StructType* parentType = getDeepMemberType(handle.memberPath);
	return parentType && handle.pHandle <= parentType->properties.size();
}