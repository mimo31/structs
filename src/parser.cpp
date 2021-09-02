#include "parser.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <stack>

#include "str.hpp"
#include "vec.hpp"

enum class LexTokenType
{
	LAngleBra,
	RAngleBra,
	Implies,
	Equals,
	PromotesTo,
	LPar,
	RPar,
	Negate,
	LCurlyBra,
	RCurlyBra,
	Property,
	Exclusive, // !
	ExclusiveOr, // *
	Or,
	And,
	Dot,
	Comma,
	Semic,
	Identifier,
	Literal,
	KWExample,
	KWType,
	KWProperty,
	KWName,
	KWDescription
};

struct LexToken
{
	LexTokenType type;
	str content;
	uint32_t lineNumber;

	LexToken(const LexTokenType type, const uint32_t lineNumber) : type(type), lineNumber(lineNumber)
	{}

	LexToken(const LexTokenType type, const str& content, const uint32_t lineNumber) : type(type), content(content), lineNumber(lineNumber)
	{
	}
};

using std::endl;

struct Identifier
{
	str name;
	uint32_t lineNumber;

	Identifier() = default;
	Identifier(const LexToken& token) : name(token.content), lineNumber(token.lineNumber) {}
};

class ErrorReporter
{
public:
	void reportLex(const uint32_t lineNumber, const str& message)
	{
		report("Lex", lineNumber, message);
	}

	void reportSyn(const uint32_t lineNumber, const str& message)
	{
		report("Syntax", lineNumber, message);
	}

	void reportSyn(const Identifier& id, const str& message)
	{
		reportSyn(id.lineNumber, message);
	}

	void reportSem(const uint32_t lineNumber, const str& message)
	{
		report("Semantic", lineNumber, message);
	}

	void reportSem(const Identifier& id, const str& message)
	{
		reportSem(id.lineNumber, message);
	}

	bool getReported() const
	{
		return reported;
	}

	ErrorReporter(ostream* const errorStream) : errorStream(errorStream)
	{
	}

private:
	bool reported = false;
	ostream* errorStream;

	void report(const str& code, const uint32_t lineNumber, const str& message)
	{
		*errorStream << code << " error @ line " << lineNumber << ": " << message << endl;
		reported = true;
	}
};

// Performs lexical analysis. I.e. converts a stream of raw characters into a stream of tokens
void tokenize(vec<LexToken>& tokens, istream& defs, ErrorReporter& er)
{
	vec<char> chars;
	const str input(std::istreambuf_iterator<char>(defs), {});

	uint64_t lineNumber = 1;
	
	bool insideLiteral = false;
	vec<char> literalContent;
	const auto submitLiteral = [&]
	{
		literalContent.push_back('\0');
		tokens.push_back(LexToken(LexTokenType::Literal, str(&literalContent.front()), lineNumber));
		insideLiteral = false;
		literalContent.clear();
	};

	int64_t identifierStart = -1;
	const auto endIdentifier = [&](const int64_t i)
	{
		if (identifierStart != -1)
		{
			const str id = input.substr(identifierStart, i - identifierStart);
			LexTokenType tokenType = LexTokenType::Identifier;
			if (id == "example")
				tokenType = LexTokenType::KWExample;
			else if (id == "type")
				tokenType = LexTokenType::KWType;
			else if (id == "property")
				tokenType = LexTokenType::KWProperty;
			else if (id == "_name")
				tokenType = LexTokenType::KWName;
			else if (id == "_description")
				tokenType = LexTokenType::KWDescription;
			tokens.push_back(tokenType == LexTokenType::Identifier ? LexToken(tokenType, id, lineNumber) : LexToken(tokenType, lineNumber));
			identifierStart = -1;
		}
	};

	uint64_t nwlScan = 0;
	for (uint64_t i = 0; i < input.length(); i++)
	{
		while (nwlScan < i)
		{
			if (input[nwlScan] == '\n')
				lineNumber++;
			nwlScan++;
		}

		if (insideLiteral)
		{
			if (input[i] == '\\')
			{
				if (i + 1 == input.length())
				{
					er.reportLex(lineNumber, "EOF after an escape slash.");
					continue;
				}
				if (input[i + 1] == '\\')
					literalContent.push_back('\\');
				else if (input[i + 1] == '\"')
					literalContent.push_back('\"');
				else
					er.reportLex(lineNumber, "Illegal char after an escape slash.");
				i++;
			}
			else if (input[i] == '\"')
			{
				submitLiteral();
			}
			else
			{
				literalContent.push_back(input[i]);
			}
			continue;
		}
		if (input[i] == '\"')
		{
			endIdentifier(i);
			insideLiteral = true;
			continue;
		}
		const auto checkFor2CharOperator = [&](const char c0, const char c1, const LexTokenType tokenType)
		{
			if (i + 1 != input.length() && input[i] == c0 && input[i + 1] == c1)
			{
				endIdentifier(i);
				tokens.push_back(LexToken(tokenType, lineNumber));
				i++;
				return true;
			}
			return false;
		};
		if (checkFor2CharOperator('=', '>', LexTokenType::Implies))
			continue;
		if (checkFor2CharOperator('-', '>', LexTokenType::PromotesTo))
			continue;
		const umap<char, LexTokenType> operators{
			{'<', LexTokenType::LAngleBra},
			{'>', LexTokenType::RAngleBra},
			{'=', LexTokenType::Equals},
			{'(', LexTokenType::LPar},
			{')', LexTokenType::RPar},
			{'~', LexTokenType::Negate},
			{'{', LexTokenType::LCurlyBra},
			{'}', LexTokenType::RCurlyBra},
			{'/', LexTokenType::Property},
			{'!', LexTokenType::Exclusive},
			{'*', LexTokenType::ExclusiveOr},
			{'|', LexTokenType::Or},
			{'&', LexTokenType::And},
			{'.', LexTokenType::Dot},
			{',', LexTokenType::Comma},
			{';', LexTokenType::Semic}
		};

		if (operators.find(input[i]) != operators.end())
		{
			endIdentifier(i);
			tokens.push_back(LexToken(operators.at(input[i]), lineNumber));
			continue;
		}

		if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r')
		{
			endIdentifier(i);
			continue;
		}

		if ((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z') || (input[i] >= '0' && input[i] <= '9') || input[i] == '-' || input[i] == '_' || input[i] == '^')
		{
			if (identifierStart == -1)
				identifierStart = i;
			continue;
		}

		endIdentifier(i);
		er.reportLex(lineNumber, "Illegal char.");
	}
	if (insideLiteral)
	{
		submitLiteral();
		er.reportLex(lineNumber, "EOF inside a string literal.");
	}
	else
	{
		endIdentifier(input.length());
	}
}

class SynBlock
{
public:
	SynBlock(const vec<LexToken>& tokens, const bool isScope, const uint32_t lineNumber)
		: tokens(tokens),
		isScope(isScope),
		lineNumber(lineNumber)
	{
	}

	void addContent(uptr<SynBlock>&& newContent)
	{
		contents.push_back(std::move(newContent));
	}

	const vec<LexToken>& getTokens() const
	{
		return tokens;
	}

	const vec<uptr<SynBlock>>& getContents() const
	{
		return contents;
	}

	bool getIsScope() const
	{
		return isScope;
	}

	uint32_t getLineNumber() const
	{
		return lineNumber;
	}

private:
	vec<LexToken> tokens;
	bool isScope;
	uint32_t lineNumber;
	vec<uptr<SynBlock>> contents;
};

void blockAnalysis(SynBlock& parent, const vec<LexToken>& tokens, ErrorReporter& er)
{
	if (tokens.empty())
		return;
	uint32_t nxt = 0;
	uint32_t currentStart = 0;
	std::stack<SynBlock*> scopes;
	scopes.push(&parent);
	const auto addTokens = [&](const bool isScope)
	{
		vec<LexToken> commandTokens;
		commandTokens.insert(commandTokens.end(), tokens.begin() + currentStart, tokens.begin() + nxt);
		scopes.top()->addContent(make_unique<SynBlock>(commandTokens, isScope, tokens[currentStart].lineNumber));
	};
	while (nxt < tokens.size())
	{
		if (tokens[nxt].type == LexTokenType::Semic)
		{
			addTokens(false);
			currentStart = nxt + 1;
		}
		else if (tokens[nxt].type == LexTokenType::LCurlyBra)
		{
			addTokens(true);
			scopes.push(scopes.top()->getContents().back().get());
			currentStart = nxt + 1;
		}
		else if (tokens[nxt].type == LexTokenType::RCurlyBra)
		{
			if (currentStart != nxt)
			{
				addTokens(false);
				er.reportSyn(tokens[nxt].lineNumber, "Missing semicolon.");
			}
			currentStart = nxt + 1;
			if (scopes.size() == 1)
				er.reportSyn(tokens[nxt].lineNumber, "Unmatched closing bracket.");
			else
				scopes.pop();
		}
		nxt++;
	}
	if (currentStart != tokens.size())
	{
		addTokens(false);
		er.reportSyn(tokens.back().lineNumber, "Missing semicolon.");
	}
	if (scopes.size() > 1)
		er.reportSyn(tokens.back().lineNumber, scopes.size() == 2 ? "Unclosed bracket." : "Unclosed bracket.");
}

void processTypeDeclaration(Universe& universe, const vec<Identifier>& declarators, ErrorReporter& er)
{
	for (const Identifier& typeDecl : declarators)
	{
		if (universe.getType(typeDecl.name))
		{
			er.reportSem(typeDecl, "The type " + typeDecl.name + " has already been declared before.");
			continue;
		}
		universe.addType(typeDecl.name);
	}
}

void parseNonScopeStatement(Universe& universe, const vec<LexToken>& tokens, ErrorReporter& er)
{
	if (tokens.empty())
		return;
	if (tokens.front().type != LexTokenType::KWType)
	{
		er.reportSyn(tokens.front(), "Unscoped statement that is not a type declaration.");
		return;
	}
	vec<Identifier> declarators;
	declarators.reserve(tokens.size() / 2);
	for (uint32_t i = 1; i < tokens.size(); i++)
	{
		if (i % 2 == 1)
		{
			if (tokens[i].type == LexTokenType::Identifier)
				declarators.push_back(tokens[i]);
			else
				er.reportSyn(tokens[i].lineNumber, "Expected identifier in type declaration.");
		}
		else
		{
			if (tokens[i].type != LexTokenType::Comma)
				er.reportSyn(tokens[i].lineNumber, "Expected comma in type declaration.");
		}
	}
	processTypeDeclaration(universe, declarators, er);
}

DeepMemberHandle getDeepMemberHandle(const StructType& type, const vec<Identifier>& identifiers, ErrorReporter& er)
{
	vec<MemberHandle> handles;
	handles.reserve(identifiers.size());
	const StructType* nextType = &type;
	for (const Identifier& interMemberId : identifiers)
	{
		const MemberHandle handle = nextType->getMember(interMemberId.name);
		if (!handle)
		{
			er.reportSem(interMemberId, interMemberId.name + " is not a member of the type " + nextType->getName() + ".");
			return {};
		}
		handles.push_back(handle);
		nextType = nextType->getMemberType(handle);
	}
	return handles;
}

DeepPropertyHandle getDeepPropertyHandle(const StructType& type, const vec<Identifier>& identifiers, ErrorReporter& er)
{
	assert(!identifiers.empty());
	DeepPropertyHandle handle;
	handle.memberPath.reserve(identifiers.size() - 1);
	const StructType* nextType = &type;
	for (uint32_t i = 0; i + 1 < identifiers.size(); i++)
	{
		const MemberHandle mHandle = nextType->getMember(identifiers[i].name);
		if (!mHandle)
		{
			er.reportSem(identifiers[i], identifiers[i].name + " is not a member of type " + nextType->getName() + ".");
			return {{}, 0};
		}
		handle.memberPath.push_back(mHandle);
		nextType = nextType->getMemberType(mHandle);
	}
	handle.pHandle = nextType->getProperty(identifiers.back().name);
	if (!handle.pHandle)
	{
		er.reportSem(identifiers.back(), identifiers.back().name + " is not a property of type " + nextType->getName() + ".");
		return {{}, 0};
	}
	return handle;
}

void getDeepMemberOrPropertyHandle(const StructType& type, const vec<Identifier>& identifiers, DeepMemberHandle& memberHandle, DeepPropertyHandle& propertyHandle, ErrorReporter& er)
{
	vec<MemberHandle> prehandles;
	prehandles.reserve(identifiers.size() - 1);
	const StructType* nextType = &type;
	for (uint32_t i = 0; i + 1 < identifiers.size(); i++)
	{
		const Identifier& interMemberId = identifiers[i];
		const MemberHandle handle = nextType->getMember(interMemberId.name);
		if (!handle)
		{
			er.reportSem(interMemberId, interMemberId.name + " is not a member of the type " + nextType->getName() + ".");
			return;
		}
		prehandles.push_back(handle);
		nextType = nextType->getMemberType(handle);
	}
	const MemberHandle getMemberHandle = nextType->getMember(identifiers.back().name);
	const PropertyHandle getPropertyHandle = nextType->getProperty(identifiers.back().name);
	if (getMemberHandle)
	{
		prehandles.push_back(getMemberHandle);
		memberHandle = prehandles;
		return;
	}
	if (getPropertyHandle)
	{
		propertyHandle.memberPath = prehandles;
		propertyHandle.pHandle = getPropertyHandle;
		return;
	}
	er.reportSyn(identifiers.back(), identifiers.back().name + " is not a member nor a property of the type " + nextType->getName() + ".");
}

bool checkIsNameUsed(const StructType& type, const Identifier& nameId, ErrorReporter& er)
{
	if (type.getMember(nameId.name))
		er.reportSem(nameId, "Type " + type.getName() + " already contains a member named " + nameId.name + ".");
	else if (type.getProperty(nameId.name))
		er.reportSem(nameId, "Type " + type.getName() + " already contains a property named " + nameId.name + ".");
	else
		return false;
	return true;
}

struct MemberDeclarator
{
	Identifier memberId;
	vec<Identifier> definition;

	MemberDeclarator(const Identifier& memberId) : memberId(memberId)
	{
	}

	MemberDeclarator(const Identifier& memberId, const vec<Identifier>& definition) : memberId(memberId), definition(definition)
	{
	}
};

void processMemberDeclaration(Universe& universe, StructType& type, const Identifier& declaredType, const vec<MemberDeclarator>& declarators, ErrorReporter& er)
{
	const StructType* const memberType = universe.getType(declaredType.name);
	if (!memberType)
		er.reportSem(declaredType, declaredType.name + " doesn't name a type.");
	for (const MemberDeclarator& declarator : declarators)
	{
		const bool nameUsed = checkIsNameUsed(type, declarator.memberId, er);
		MemberHandle newHandle = 0;
		if (!nameUsed && memberType)
			newHandle = type.addMember(declarator.memberId.name, memberType);
		if (!declarator.definition.empty())
		{
			const vec<MemberHandle> eqHandles = getDeepMemberHandle(type, declarator.definition, er);
			if (!eqHandles.empty() && newHandle)
			{
				if (eqHandles.front() == newHandle)
					er.reportSyn(declarator.definition.front(), "Assign-declaration of a member can't depend on that member.");
				else
					type.addMemberEquality({ newHandle }, eqHandles);
			}
		}
	}
}

void processPropertyDeclaration(StructType& type, const vec<Identifier>& propertyNames, ErrorReporter& er)
{
	for (const Identifier& propertyNameId : propertyNames)
	{
		if (!checkIsNameUsed(type, propertyNameId, er))
			type.addProperty(propertyNameId.name);
	}
}

void processPromotion(Universe& universe, StructType& scopeType, const Identifier& propertyIdentifier, const Identifier& typeIdentifier, ErrorReporter& er)
{
	const PropertyHandle property = scopeType.getProperty(propertyIdentifier.name);
	if (!property)
		er.reportSem(propertyIdentifier, propertyIdentifier.name + " doesn't name of property of the type " + scopeType.getName() + ".");
	const StructType* const promoteTo = universe.getType(typeIdentifier.name);
	if (!promoteTo)
		er.reportSem(typeIdentifier, typeIdentifier.name + " doesn't name a type.");
	if (property && promoteTo)
		scopeType.addPromotion(property, promoteTo);
}

enum class PropertyExpressionOperation
{
	None, And, Or, Negate
};

struct PropertyExpression
{
	PropertyExpressionOperation operation;
	vec<PropertyExpression> operands;

	vec<Identifier> memberSequence;
	uint32_t lineNumber;

	PropertyExpression(const PropertyExpressionOperation operation, const vec<PropertyExpression>& operands, const uint32_t lineNumber)
		: operation(operation),
		operands(operands),
		lineNumber(lineNumber)
	{
	}

	PropertyExpression(const vec<Identifier>& memberSequence, const uint32_t lineNumber)
		: operation(PropertyExpressionOperation::None),
		memberSequence(memberSequence),
		lineNumber(lineNumber)
	{
	}
};

PropertyRelations relationsAnd(const PropertyRelations& relations0, const PropertyRelations& relations1)
{
	PropertyRelations relations = relations0;
	relations.insert(relations.end(), relations1.begin(), relations1.end());
	return relations;
}

PropertyRelations relationsOr(const PropertyRelations& relations0, const PropertyRelations& relations1)
{
	if (relations0.empty())
		return relations1;
	if (relations1.empty())
		return relations0;
	PropertyRelations result;
	result.reserve(relations0.size() + relations1.size());
	for (const vec<DeepProperty>& orBlock0 : relations0)
	{
		for (const vec<DeepProperty>& orBlock1 : relations1)
		{
			vec<DeepProperty> orMerge = orBlock0;
			orMerge.insert(orMerge.end(), orBlock1.begin(), orBlock1.end());
			result.push_back(orMerge);
		}
	}
	return result;
}

PropertyRelations relationsOr(const vec<PropertyRelations>& relations)
{
	PropertyRelations newRelations;
	for (const PropertyRelations& partialRelations : relations)
		newRelations = relationsOr(newRelations, partialRelations);
	return newRelations;
}

PropertyRelations relationsNegate(const PropertyRelations& relations)
{
	PropertyRelations negatedRelations;
	for (const vec<DeepProperty>& orBlock : relations)
	{
		PropertyRelations partialRelations;
		partialRelations.reserve(orBlock.size());
		for (const DeepProperty& property : orBlock)
		{
			const DeepProperty negatedProperty(property.handle, !property.negated);
			partialRelations.push_back({ negatedProperty });
		}
		negatedRelations = relationsOr(negatedRelations, partialRelations);
	}
	return negatedRelations;
}

PropertyRelations relationsExclusivity(const vec<PropertyRelations>& relations)
{
	if (relations.size() <= 1)
		return {};
	vec<PropertyRelations> negations;
	negations.reserve(relations.size());
	for (const PropertyRelations& partialRelations : relations)
		negations.push_back(relationsNegate(partialRelations));
	PropertyRelations newRelations;
	for (uint32_t i = 0; i < relations.size(); i++)
	{
		PropertyRelations andBlock;
		andBlock.reserve(relations.size() - 1);
		for (uint32_t j = 0; j < relations.size(); j++)
		{
			if (j == i)
				continue;
			andBlock = relationsAnd(andBlock, negations[j]);
		}
		newRelations = relationsOr(newRelations, andBlock);
	}
	return newRelations;
}

PropertyRelations propertyExpressionToRelations(StructType& scopeType, const PropertyExpression& expression, ErrorReporter& er)
{
	if (expression.operation == PropertyExpressionOperation::None)
	{
		return { { getDeepPropertyHandle(scopeType, expression.memberSequence, er) } };
	}
	if (expression.operation == PropertyExpressionOperation::And)
	{
		PropertyRelations relations;
		for (const PropertyExpression& subexpression : expression.operands)
		{
			const PropertyRelations subrelations = propertyExpressionToRelations(scopeType, subexpression, er);
			relations.insert(relations.end(), subrelations.begin(), subrelations.end());
		}
		return relations;
	}
	if (expression.operation == PropertyExpressionOperation::Or)
	{
		PropertyRelations relations;
		for (const PropertyExpression& subexpression : expression.operands)
		{
			const PropertyRelations subrelations = propertyExpressionToRelations(scopeType, subexpression, er);
			relations = relationsOr(relations, subrelations);
		}
		return relations;
	}
	if (expression.operation == PropertyExpressionOperation::Negate)
	{
		PropertyRelations subrelations = propertyExpressionToRelations(scopeType, expression.operands.front(), er);
		PropertyRelations relations;
		for (vec<DeepProperty>& orBlock : subrelations)
		{
			PropertyRelations partialRelations;
			partialRelations.reserve(orBlock.size());
			for (DeepProperty& property : orBlock)
			{
				property.negated = !property.negated;
				partialRelations.push_back({ property });
			}
			relations = relationsOr(relations, partialRelations);
		}
		return relations;
	}
	assert(!"Unknown property expression operation.");
}

void processExclusivity(StructType& scopeType, const vec<PropertyExpression>& expressions, ErrorReporter& er)
{
	vec<PropertyRelations> relations;
	relations.reserve(expressions.size());
	for (const PropertyExpression& expression : expressions)
		relations.push_back(propertyExpressionToRelations(scopeType, expression, er));
	scopeType.addPropertyRelations(relationsExclusivity(relations));
}

void processExclusiveOr(StructType& scopeType, const vec<PropertyExpression>& expressions, ErrorReporter& er)
{
	vec<PropertyRelations> relations;
	relations.reserve(expressions.size());
	for (const PropertyExpression& expression : expressions)
		relations.push_back(propertyExpressionToRelations(scopeType, expression, er));
	scopeType.addPropertyRelations(relationsExclusivity(relations));
	scopeType.addPropertyRelations(relationsOr(relations));
}

void processImplication(StructType& scopeType, const vec<PropertyExpression>& expressions, ErrorReporter& er)
{
	PropertyRelations relations;
	vec<PropertyRelations> partialRelations;
	partialRelations.reserve(expressions.size());
	for (const PropertyExpression& expression : expressions)
		partialRelations.push_back(propertyExpressionToRelations(scopeType, expression, er));
	for (uint32_t i = 0; i + 1 < expressions.size(); i++)
	{
		const PropertyRelations newRelations = relationsOr(relationsNegate(partialRelations[i]), partialRelations[i + 1]);
		relations = relationsAnd(relations, newRelations);
	}
	scopeType.addPropertyRelations(relations);
}

void processEquality(StructType& scopeType, const vec<PropertyExpression>& expressions, ErrorReporter& er)
{
	if (expressions.front().operation != PropertyExpressionOperation::None)
	{
		er.reportSyn(expressions.front().lineNumber, "Complex expressions as equality operands are not allowed.");
		return;
	}
	DeepMemberHandle memberHandle;
	DeepPropertyHandle propertyHandle;
	propertyHandle.pHandle = StructType::NoProperty;
	getDeepMemberOrPropertyHandle(scopeType, expressions.front().memberSequence, memberHandle, propertyHandle, er);
	for (uint32_t i = 1; i < expressions.size(); i++)
	{
		if (expressions[i].operation != PropertyExpressionOperation::None)
		{
			er.reportSyn(expressions[i].lineNumber, "Complex expressions as equality operands are not allowed.");
			continue;
		}
		if (propertyHandle.pHandle)
		{
			const DeepPropertyHandle currentProperty = getDeepPropertyHandle(scopeType, expressions[i].memberSequence, er);
			if (currentProperty.pHandle)
				scopeType.addPropertyEquality(propertyHandle, currentProperty);
		}
		else
		{
			const DeepMemberHandle currentMember = getDeepMemberHandle(scopeType, expressions[i].memberSequence, er);
			if (!currentMember.empty())
				scopeType.addMemberEquality(memberHandle, currentMember);
		}
	}
}

void processExpressionDeclaration(StructType& scopeType, const PropertyExpression& expression, ErrorReporter& er)
{
	const PropertyRelations relations = propertyExpressionToRelations(scopeType, expression, er);
	scopeType.addPropertyRelations(relations);
}

PropertyExpression parsePropertyExpression(const vec<LexToken>& tokens, const uint32_t from, const uint32_t to, ErrorReporter& er)
{
	assert(!tokens.empty());
	if (from <= to)
	{
		er.reportSyn(from < tokens.size() ? tokens[from] : tokens.back(), "Empty property expression.");
		return PropertyExpression({}, from < tokens.size() ? tokens[from].lineNumber : tokens.back().lineNumber);
	}
	bool hasOr = false;
	bool hasAnd = false;
	{
	int32_t openParens = 0;
	for (uint32_t i = from; i < to; i++)
	{
		if (tokens[i].type == LexTokenType::LPar)
			openParens++;
		else if (tokens[i].type == LexTokenType::RPar)
			openParens--;
		else if (openParens == 0 && tokens[i].type == LexTokenType::Or)
			hasOr = true;
		else if (openParens == 0 && tokens[i].type == LexTokenType::And)
			hasAnd = true;
		if (openParens < 0)
		{
			er.reportSyn(tokens[i], "Closing parenthesis not matched by an opening parenthesis.");
			return PropertyExpression({}, tokens[from].lineNumber);
		}
	}
	if (openParens > 0)
	{
		er.reportSyn(tokens[to - 1], "Too many opening parentheses.");
		return PropertyExpression({}, tokens[from].lineNumber);
	}
	}
	const auto parseAndOr = [&](const LexTokenType tokenType, const PropertyExpressionOperation operation)
	{
		vec<PropertyExpression> operands;
		int32_t openParens = 0;
		uint32_t lastStart = from;
		for (uint32_t i = from; i < to; i++)
		{
			if (tokens[i].type == LexTokenType::LPar)
				openParens++;
			else if (tokens[i].type == LexTokenType::RPar)
				openParens--;
			if (openParens == 0 && tokens[i].type == tokenType)
			{
				operands.push_back(parsePropertyExpression(tokens, lastStart, i, er));
				lastStart = i + 1;
			}
		}
		operands.push_back(parsePropertyExpression(tokens, lastStart, to, er));
		return PropertyExpression(operation, operands, tokens[from].lineNumber);
	};
	if (hasOr)
	{
		return parseAndOr(LexTokenType::Or, PropertyExpressionOperation::Or);
	}
	else if (hasAnd)
	{
		return parseAndOr(LexTokenType::And, PropertyExpressionOperation::And);
	}
	else if (tokens[from].type == LexTokenType::Negate)
	{
		return PropertyExpression(PropertyExpressionOperation::Negate, { parsePropertyExpression(tokens, from + 1, to, er) }, tokens[from].lineNumber);
	}
	else if (tokens[from].type == LexTokenType::LPar && tokens[to - 1].type == LexTokenType::RPar)
	{
		return parsePropertyExpression(tokens, from + 1, to - 1, er);
	}
	vec<Identifier> memberIds;
	if (tokens[from].type != LexTokenType::Identifier)
	{
		er.reportSyn(tokens[from], "Expected an identifier. (assuming direct member chain as property expression)");
		return PropertyExpression({}, tokens[from].lineNumber);
	}
	memberIds.push_back(tokens[from]);
	{
		uint32_t nxt = from + 1;
		while (nxt < to)
		{
			if (tokens[nxt].type != LexTokenType::Dot)
			{
				er.reportSyn(tokens[nxt], "Expected a dot. (assuming direct member chain as property expression)");
				return PropertyExpression({}, tokens[from].lineNumber);
			}
			if (nxt + 1 == to || tokens[nxt + 1].type != LexTokenType::Identifier)
			{
				er.reportSyn(nxt + 1 == tokens.size() ? tokens[nxt] : tokens[nxt + 1], "Expected an identifier. (assuming direct member chain as property expression)");
				return PropertyExpression({}, tokens[from].lineNumber);
			}
			memberIds.push_back(tokens[nxt + 1]);
			nxt += 2;
		}
	}
	return PropertyExpression(memberIds, tokens[from].lineNumber);
}

void parseTypeScope(Universe& universe, const SynBlock* scope, const Identifier& typeIdentifier, ErrorReporter& er)
{
	StructType* const scopeType = universe.getType(typeIdentifier.name);
	if (!scopeType)
		er.reportSem(typeIdentifier, typeIdentifier.name + " doesn't name a type.");
	for (const auto& statement : scope->getContents())
	{
		if (statement->getIsScope())
		{
			er.reportSyn(statement->getLineNumber(), "Nested scopes are not allowed.");
			continue;
		}
		const vec<LexToken>& tokens = statement->getTokens();
		if (tokens.empty())
			continue;
		if (tokens.size() >= 2 && tokens[0].type == LexTokenType::Identifier && tokens[1].type == LexTokenType::Identifier)
		{
			// this is a declaration

			const Identifier declaredType = tokens[0];
			vec<MemberDeclarator> declarators;
			//const StructType* const declaredType = universe.getType(declaredTypeId.name);
			uint32_t nxt = 1;
			while (nxt < tokens.size())
			{
				if (tokens[nxt].type != LexTokenType::Identifier)
				{
					er.reportSyn(tokens[nxt], "Expected an identifier for member name.");
					break;
				}
				const Identifier& memberId = tokens[nxt];
				if (nxt + 1 == tokens.size() || tokens[nxt + 1].type == LexTokenType::Comma)
				{
					declarators.push_back(MemberDeclarator(memberId));
					nxt += 2;
					continue;
				}
				if (tokens[nxt + 1].type != LexTokenType::Equals)
				{
					er.reportSyn(tokens[nxt + 1], "Expected a comma or an equal sign.");
					break;
				}
				if (nxt + 2 == tokens.size() || tokens[nxt + 2].type != LexTokenType::Identifier)
				{
					er.reportSyn(nxt + 2 == tokens.size() ? tokens[nxt + 1] : tokens[nxt + 2], "Expected an identifier.");
					break;
				}
				vec<Identifier> definitionIds = { tokens[nxt + 2] };
				nxt += 3;
				bool error = false;
				while (nxt < tokens.size() && tokens[nxt].type != LexTokenType::Comma)
				{
					if (tokens[nxt].type != LexTokenType::Dot)
					{
						er.reportSyn(tokens[nxt], "Expected a dot.");
						error = true;
						break;
					}
					if (nxt + 1 == tokens.size() || tokens[nxt + 1].type != LexTokenType::Identifier)
					{
						er.reportSyn(nxt + 1 == tokens.size() ? tokens[nxt] : tokens[nxt + 1], "Expected an identifier.");
						error = true;
						break;
					}
					definitionIds.push_back(tokens[nxt + 1]);
					nxt += 2;
				}
				nxt++;
				if (!error)
					declarators.push_back(MemberDeclarator(memberId, definitionIds));
			}
			processMemberDeclaration(universe, *scopeType, declaredType, declarators, er);
			continue;
		}
		if (tokens.front().type == LexTokenType::KWProperty)
		{
			if (tokens.size() == 1 || tokens[1].type != LexTokenType::Identifier)
			{
				er.reportSyn(tokens.size() == 1 ? tokens[0] : tokens[1], "Expected an identifier.");
				continue;
			}
			vec<Identifier> propertyIds;
			uint32_t nxt = 1;
			while (nxt < tokens.size())
			{
				if (tokens[nxt].type == LexTokenType::Identifier)
					er.reportSyn(tokens[nxt], "Expected an identifier.");
				else
					propertyIds.push_back(tokens[nxt]);
				if (nxt + 1 == tokens.size())
					break;
				if (tokens[nxt + 1].type != LexTokenType::Comma)
					er.reportSyn(tokens[nxt + 1], "Expected a comma.");
				nxt += 2;
			}
			processPropertyDeclaration(*scopeType, propertyIds, er);
			continue;
		}
		if (tokens.size() == 3 && tokens[1].type == LexTokenType::PromotesTo)
		{
			if (tokens[0].type != LexTokenType::Identifier)
			{
				er.reportSyn(tokens[0], "Expected an identifier.");
				continue;
			}
			if (tokens[2].type != LexTokenType::Identifier)
			{
				er.reportSyn(tokens[2], "Expected an identifier.");
				continue;
			}
			processPromotion(universe, *scopeType, tokens[0], tokens[2], er);
			continue;
		}
		const auto checkContains = [&tokens](const LexTokenType tokenType)
		{
			return std::any_of(tokens.begin(), tokens.end(), [tokenType](const LexToken& token){ return token.type == tokenType; });
		};
		const auto splitPropertyExpressionsOn = [&tokens, &er](const LexTokenType tokenType)
		{
			uint32_t lastStart = 0;
			vec<PropertyExpression> expressions;
			for (uint32_t i = 0; i < tokens.size(); i++)
			{
				if (tokens[i].type == tokenType)
				{
					expressions.push_back(parsePropertyExpression(tokens, lastStart, i, er));
					lastStart = i + 1;
				}
			}
			expressions.push_back(parsePropertyExpression(tokens, lastStart, tokens.size(), er));
			return expressions;
		};
		if (checkContains(LexTokenType::Exclusive))
		{
			processExclusivity(*scopeType, splitPropertyExpressionsOn(LexTokenType::Exclusive), er);
			continue;
		}
		if (checkContains(LexTokenType::ExclusiveOr))
		{
			processExclusiveOr(*scopeType, splitPropertyExpressionsOn(LexTokenType::ExclusiveOr), er);
			continue;
		}
		if (checkContains(LexTokenType::Equals))
		{
			processEquality(*scopeType, splitPropertyExpressionsOn(LexTokenType::Equals), er);
			continue;
		}
		if (checkContains(LexTokenType::Implies))
		{
			processImplication(*scopeType, splitPropertyExpressionsOn(LexTokenType::Implies), er);
			continue;
		}
		processExpressionDeclaration(*scopeType, parsePropertyExpression(tokens, 0, tokens.size(), er), er);
	}
}

void parseExampleScope(Universe& /*universe*/, const SynBlock* /*scope*/, const Identifier& /*typeIdentifier*/, ErrorReporter& /*er*/)
{
	// TODO: implement
}

void parseScope(Universe& universe, const SynBlock* scope, ErrorReporter& er)
{
	const vec<LexToken>& tokens = scope->getTokens();
	if (tokens.empty())
	{
		er.reportSyn(scope->getLineNumber(), "Expected description before scope.");
		return;
	}
	if (tokens.front().type == LexTokenType::KWExample)
	{
		if (tokens.size() < 2 || tokens[1].type != LexTokenType::LAngleBra)
		{
			er.reportSyn(tokens.size() < 2 ? tokens[0] : tokens[1], "Expected < after the example keyword.");
			return;
		}
		if (tokens.size() < 3 || tokens[2].type != LexTokenType::Identifier)
		{
			er.reportSyn(tokens.size() < 3 ? tokens[1] : tokens[2], "Expected an identifier after < in the scope description.");
			return;
		}
		if (tokens.size() < 4 || tokens[3].type != LexTokenType::RAngleBra)
		{
			er.reportSyn(tokens.size() < 4 ? tokens[2] : tokens[3], "Expected > after the identifier in scope description.");
		}
		parseExampleScope(universe, scope, tokens[2], er);
		return;
	}
	if (tokens.front().type == LexTokenType::Identifier)
	{
		if (tokens.size() > 1)
			er.reportSyn(tokens[1], "Expected only the type identifier in scope description.");
		parseTypeScope(universe, scope, tokens.front(), er);
		return;
	}
	er.reportSyn(scope->getTokens().front(), "Expected the example keyword or an identifier in scope description.");
}

void syntaxAnalysis(Universe& universe, const SynBlock* root, ErrorReporter& er)
{
	for (const auto& content : root->getContents())
	{
		if (content->getIsScope())
			parseScope(universe, content.get(), er);
		else
			parseNonScopeStatement(universe, content->getTokens(), er);
	}
}

bool parse(Universe& universe, istream& defs, ostream& error)
{
	ErrorReporter er(&error);

	vec<LexToken> tokens;
	tokenize(tokens, defs, er);

	uptr<SynBlock> rootBlock = make_unique<SynBlock>(vec<LexToken>(), true, 0);
	blockAnalysis(*rootBlock, tokens, er);

	syntaxAnalysis(universe, rootBlock.get(), er);

	return er.getReported();
}