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
	return deepPropertyGroups.size();
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
	preprocessPropertyEqualities();
	preprocessChildPromotions();
	preprocessRelations();
	preprocessed = true;
}

bool StructType::isPreprocessed() const
{
	return preprocessed;
}

size_t StructType::getFlatRelationCount() const
{
	return flatRelations.size();
}

size_t StructType::getPossibleInstancesCount() const
{
	/*
	for (const vec<DeepProperty>& relation : relations)
	{
		cout << endl;
		for (const DeepProperty& property : relation)
		{
			assert(property.handle.memberPath.empty());
			cout << (property.negated ? "~" : "") << property.handle.pHandle - 1 << " ";
		}
	}
	for (const vec<FlatProperty>& relation : flatRelations)
	{
		cout << endl;
		for (const FlatProperty& property : relation)
			cout << (property.negated ? "~" : "") << property.index << " ";
	}
	*/
	return getPossibleInstancesCount(vec<bool>(deepPropertyGroups.size(), false), vec<bool>(deepPropertyGroups.size()));
}

void StructType::precheck(ErrorReporter& er) const
{
	checkPromotions(er);
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

uint32_t StructType::getDeepPropertyIndex(const DeepPropertyHandle& handle) const
{
	const DeepMemberHandle& path = handle.memberPath;
	if (path.empty())
		return deepPropertyGroup[0][handle.pHandle - 1];
	//const uint32_t memberIndex = getDeepMemberIndex(path);
	DeepMemberHandle pathTail;
	pathTail.insert(pathTail.end(), path.begin() + 1, path.end());
	return deepPropertyGroup[path.front()][members[path.front() - 1].second->getDeepPropertyIndex({ pathTail, handle.pHandle })];
}

uint32_t StructType::getDeepPropertyIndex(const DeepMemberHandle& handle, const uint32_t propertyIndex) const
{
	if (handle.empty())
		return propertyIndex;
	//const uint32_t memberIndex = getDeepMemberIndex(handle);
	DeepMemberHandle pathTail;
	pathTail.insert(pathTail.end(), handle.begin() + 1, handle.end());
	return deepPropertyGroup[handle.front()][members[handle.front() - 1].second->getDeepPropertyIndex(pathTail, propertyIndex)];
}

void StructType::preprocessPropertyEqualities()
{
	vec<vec<vec<pair<uint32_t, uint32_t>>>> neighbors;
	neighbors.push_back(vec<vec<pair<uint32_t, uint32_t>>>(getPropertyCount()));
	for (const auto& mem : members)
		neighbors.push_back(vec<vec<pair<uint32_t, uint32_t>>>(mem.second->deepPropertyGroups.size()));
	for (const auto& eq : propertyEqualities)
	{
		const auto& toIndPair = [&](const DeepPropertyHandle& handle) -> pair<uint32_t, uint32_t>
		{
			if (handle.memberPath.empty())
				return { 0, handle.pHandle - 1 };
			const uint32_t memInd = handle.memberPath.front();//getDeepMemberIndex(handle.memberPath);
			DeepMemberHandle pathTail;
			pathTail.insert(pathTail.end(), handle.memberPath.begin() + 1, handle.memberPath.end());
			return { memInd, members[handle.memberPath.front() - 1].second->getDeepPropertyIndex({ pathTail, handle.pHandle }) };
		};
		const pair<uint32_t, uint32_t> p0 = toIndPair(eq.first);
		const pair<uint32_t, uint32_t> p1 = toIndPair(eq.second);
		neighbors[p0.first][p0.second].push_back(p1);
		neighbors[p1.first][p1.second].push_back(p0);
	}
	for (const auto& eq : memberEqualities)
	{
		const StructType* eqType = getDeepMemberType(eq.first);
		for (uint32_t pi = 0; pi < eqType->deepPropertyGroups.size(); pi++)
		{
			const auto& toIndPair = [&](const DeepMemberHandle& handle) -> pair<uint32_t, uint32_t>
			{
				DeepMemberHandle tail;
				tail.insert(tail.end(), handle.begin() + 1, handle.end());
				return { handle.front(), members[handle.front() - 1].second->getDeepPropertyIndex(tail, pi) };
			};
			const pair<uint32_t, uint32_t> p0 = toIndPair(eq.first);
			const pair<uint32_t, uint32_t> p1 = toIndPair(eq.second);
			neighbors[p0.first][p0.second].push_back(p1);
			neighbors[p1.first][p1.second].push_back(p0);
		}
	}

	deepPropertyGroup.reserve(getMemberCount() + 1);
	constexpr uint32_t notset = std::numeric_limits<uint32_t>::max();
	deepPropertyGroup.push_back(vec<uint32_t>(getPropertyCount(), notset));
	for (const auto& mem : members)
		deepPropertyGroup.push_back(vec<uint32_t>(mem.second->deepPropertyGroups.size(), notset));
	const auto& bfs = [&](const uint32_t mi, const uint32_t pi, const auto& bfs_fun) -> void
	{
		deepPropertyGroups.back().push_back({ mi, pi });
		deepPropertyGroup[mi][pi] = deepPropertyGroups.size() - 1;
		for (const pair<uint32_t, uint32_t>& nei : neighbors[mi][pi])
			if (deepPropertyGroup[nei.first][nei.second] == notset)
				bfs_fun(nei.first, nei.second, bfs_fun);
	};
	for (uint32_t pi = 0; pi < getPropertyCount(); pi++)
	{
		if (deepPropertyGroup[0][pi] == notset)
		{
			deepPropertyGroups.push_back({});
			bfs(0, pi, bfs);
		}
	}
	for (uint32_t mim = 0; mim < getMemberCount(); mim++)
	{
		for (uint32_t pi = 0; pi < members[mim].second->deepPropertyGroups.size(); pi++)
		{
			if (deepPropertyGroup[mim + 1][pi] == notset)
			{
				deepPropertyGroups.push_back({});
				bfs(mim + 1, pi, bfs);
			}
		}
	}
}

void StructType::checkPromotions(ErrorReporter& er) const
{
	for (const pair<PropertyHandle, const StructType*>& promotion : promotions)
	{
		const MemberHandle mh = promotion.second->getMember(name);
		if (!mh)
		{
			er.reportProc("Type " + name + " promotes to type " + promotion.second->name + ", which does not have member with the name " + name + ".");
			continue;
		}
		if (promotion.second->getMemberType(mh) != this)
			er.reportProc("Type " + name + " promotes to type " + promotion.second->name + ", whose member " + name + " is of type " + promotion.second->getMemberType(mh)->name + " instead of " + name + ".");
	}
}

void StructType::preprocessChildPromotions()
{
	for (const auto& m : members)
	{
		for (const pair<PropertyHandle, const StructType*>& promotion : m.second->promotions)
		{
			if (promotion.second == this)
			{
				const MemberHandle mh = getMember(m.second->name);
				const uint32_t flatPropertyIndex = deepPropertyGroup[mh][m.second->deepPropertyGroup[0][promotion.first - 1]];
				flatRelations.push_back({ FlatProperty(flatPropertyIndex, false) });
			}
		}
	}
}

void StructType::preprocessRelations()
{
	for (const vec<DeepProperty>& relation : relations)
	{
		vec<FlatProperty> newRelation(relation.size());
		for (uint32_t i = 0; i < relation.size(); i++)
		{
			newRelation[i].index = getDeepPropertyIndex(relation[i].handle);
			newRelation[i].negated = relation[i].negated;
		}
		flatRelations.push_back(newRelation);
	}
	for (uint32_t i = 0; i < getMemberCount(); i++)
	{
		for (const vec<FlatProperty>& memberRelation : members[i].second->flatRelations)
		{
			vec<FlatProperty> substRelation(memberRelation.size());
			for (uint32_t j = 0; j < memberRelation.size(); j++)
			{
				substRelation[j].index = deepPropertyGroup[i + 1][memberRelation[j].index];
				substRelation[j].negated = memberRelation[j].negated;
			}
			flatRelations.push_back(substRelation);
		}
	}
	if (flatRelations.empty())
		return;
	for (vec<FlatProperty>& relation : flatRelations)
	{
		sort(relation.begin(), relation.end(), [](const FlatProperty lhs, const FlatProperty rhs)
		{
			return lhs.index < rhs.index || (lhs.index == rhs.index && !lhs.negated && rhs.negated);
		});
	}
	sort(flatRelations.begin(), flatRelations.end(), [](const vec<FlatProperty>& lhs, const vec<FlatProperty>& rhs)
	{
		if (lhs.size() < rhs.size())
			return true;
		if (lhs.size() > rhs.size())
			return false;
		for (uint32_t i = 0; i < lhs.size(); i++)
		{
			if (lhs[i].index < rhs[i].index || (lhs[i].index == rhs[i].index && !lhs[i].negated && rhs[i].negated))
				return true;
			if (lhs[i].index > rhs[i].index || lhs[i].negated != rhs[i].negated)
				return false;
		}
		return false;
	});
	vec<vec<FlatProperty>> filtered;
	filtered.push_back(flatRelations.front());
	for (uint32_t i = 1; i < flatRelations.size(); i++)
	{
		if (flatRelations[i] != flatRelations[i - 1])
			filtered.push_back(flatRelations[i]);
	}
	flatRelations = filtered;
}

bool StructType::checkDeepPropertyValid(const DeepPropertyHandle& handle)
{
	const StructType* parentType = getDeepMemberType(handle.memberPath);
	return parentType && handle.pHandle <= parentType->properties.size();
}

size_t StructType::getPossibleInstancesCount(vec<bool> specified, vec<bool> values) const
{
	bool changed;
	do
	{
		changed = false;
		for (const vec<FlatProperty>& relation : flatRelations)
		{
			int32_t unspecInd = -1;
			bool useless = false;
			for (uint32_t i = 0; i < relation.size(); i++)
			{
				const uint32_t ind = relation[i].index;
				if (specified[ind])
				{
					if (values[ind] != relation[i].negated)
					{
						useless = true;
						break;
					}
				}
				else
				{
					if (unspecInd == -1)
						unspecInd = i;
					else
						unspecInd = -2;
				}
			}
			if (!useless && unspecInd == -1)
				return 0;
			if (!useless && unspecInd != -2)
			{
				specified[relation[unspecInd].index] = true;\
				values[relation[unspecInd].index] = !relation[unspecInd].negated;
				changed = true;
			}
		}
	} while (changed);
	for (uint32_t i = 0; i < specified.size(); i++)
	{
		if (!specified[i])
		{
			specified[i] = true;
			values[i] = false;
			const size_t c0 = getPossibleInstancesCount(specified, values);
			values[i] = true;
			const size_t c1 = getPossibleInstancesCount(specified, values);
			return c0 + c1;
		}
	}
	/*cout << endl;
	for (uint32_t i = 0; i < values.size(); i++)
		cout << values[i] << " ";*/
	return 1;
}