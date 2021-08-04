#include "parser.hpp"

#include <fstream>

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
};

class ErrorReporter
{
private:
	bool reported = false;
	ostream* errorStream;

public:
	void reportLex(const uint32_t lineNumber, const str& message)
	{
		report("Lex", lineNumber, message);
	}

	void reportSyn(const uint32_t lineNumber, const str& message)
	{
		report("Syntax", lineNumber, message);
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
	void report(const str& code, const uint32_t lineNumber, const str& message)
	{
		*errorStream << code << " error @ line " << lineNumber << ": " << message << endl;
		reported = true;
	}
};

void tokenize(vec<LexToken>& tokens, istream& defs, ErrorReporter& rp)
{
	bool failed = false;

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

	int64_t nwlScan = 0;
	for (int64_t i = 0; i < input.length(); i++)
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
					rp.reportLex(lineNumber, "EOF after an escape slash.");
					continue;
				}
				if (input[i + 1] == '\\')
					literalContent.push_back('\\');
				else if (input[i + 1] == '\"')
					literalContent.push_back('\"');
				else
					rp.reportLex(lineNumber, "Illegal char after an escape slash.");
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

		if ((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z') || (input[i] >= '0' && input[i] <= '9') || input[i] == '-' || input[i] == '_')
		{
			if (identifierStart == -1)
				identifierStart = i;
			continue;
		}

		endIdentifier(i);
		rp.reportLex(lineNumber, "Illegal char.");
	}
	if (insideLiteral)
	{
		submitLiteral();
		rp.reportLex(lineNumber, "EOF inside a string literal.");
	}
	else
	{
		endIdentifier(input.length());
	}
}

void processTypeDeclaration(Universe& universe, const vec<Identifier>& typeDeclarations, ErrorReporter& rp)
{
	for (const Identifier& typeDecl : typeDeclarations)
	{
		if (universe.getType(typeDecl.name))
		{
			rp.reportSem(typeDecl, "The type " + typeDecl.name + " has already been declared before.");
			continue;
		}
		universe.addType(typeDecl.name);
	}
}

void processPropertyDeclaration(Universe& universe, const Identifier& parentType, const vec<Identifier>& propertyDeclarations, ErrorReporter& rp)
{
	StructType* const type = universe.getType(parentType.name);
	if (!type)
	{
		rp.reportSem(parentType, parentType.name + " doesn't name a type.");
		return;
	}
	
	for (const Identifier& propertyDecl : propertyDeclarations)
	{
		const str& name = propertyDecl.name;
		if (type->getProperty(name))
		{
			rp.reportSem(propertyDecl, "Property " + name + " for type " + parentType.name + " has already been defined before.");
			continue;
		}
		if (type->getMember(name))
		{
			rp.reportSem(propertyDecl, "Type " + parentType.name + " has already had a member named " + name + " defined.");
			continue;
		}
		type->addProperty(name);
	}
}

void processMemberDeclaration(Universe& universe, const Identifier& declaredType, const vec<pair<Identifier, Identifier>>& memberDeclarations, ErrorReporter& rp)
{
	const StructType* const type = universe.getType(declaredType.name);
	if (!type)
	{
		rp.reportSem(declaredType, declaredType.name + " doesn't name a type.");
		return;
	}

	for (const auto& memberDecl : memberDeclarations)
	{
		StructType* const parentType = universe.getType(memberDecl.first.name);
		if (!parentType)
		{
			rp.reportSem(memberDecl.first, memberDecl.first.name + " doesn't name a type.");
			continue;
		}

		const str& mname = memberDecl.second.name;
		if (parentType->getMember(mname))
		{
			rp.reportSem(memberDecl.second, "Type " + memberDecl.first.name + " has already had a member named " + mname + " defined.");
			continue;
		}
		if (parentType->getProperty(mname))
		{
			rp.reportSem(memberDecl.second, "Type " + memberDecl.first.name + " has already had a property name " + mname + " defined.");
			continue;
		}
		parentType->addMember(mname, type);
	}
}

struct PropertyExpression
{

};

//void processEqualityExpression(Universe& universe, const vec<Identifier>& scope, const vec<vec<Identifier>, bool>& equality)

void processExpression(Universe& universe, const vec<Identifier>& scope, ErrorReporter& rp)
{

}

void processExample(/* TODO: implement */)
{
	// TODO: implement
}

void syntaxAnalysis(const vec<LexToken>& tokens, Universe& universe, ostream& error)
{
	// TODO: move code from parse to here
}

bool parse(Universe& universe, istream& defs, ostream& error)
{
	// TODO: construct the ErrorReporter

	bool failed = false;
	const auto reportError = [&failed, &error](const uint32_t lineNumber, const str& message)
	{
		error << "Error @ line " << lineNumber << ": " << message << endl;
		failed = true;
	};

	vec<LexToken> tokens;
	failed |= tokenize(tokens, defs, error);

	const auto checkTokenType = [&](const uint32_t index, const LexTokenType tokenType)
	{
		return index < tokens.size() && tokens[index].type == tokenType;
	};

	const auto addType = [&](const str& name, const uint32_t lineNumber)
	{
		if (universe.getType(name))
			reportError(lineNumber, "The type " + name + " has already been declared before.");
		else
			universe.addType(name);
	};

	for (uint64_t i = 0; i < tokens.size(); i++)
	{
		// deals with type declarations
		if (tokens[i].type == LexTokenType::KWType)
		{
			if (!checkTokenType(i + 1, LexTokenType::Identifier))
			{
				reportError(tokens[i].lineNumber, "Expected an identifier after a type keyword.");
				if (i + 1 != tokens.size())
					i++;
				continue;
			}
			i++;
			addType(tokens[i].content, tokens[i].lineNumber);
			i--;
			while ((i += 2) + 1 < tokens.size())
			{
				if (tokens[i].type == LexTokenType::Semic)
					break;
				if (tokens[i].type != LexTokenType::Comma)
				{
					reportError(tokens[i].lineNumber, "Expected a semicolon or a comma after an identifier.");
					break;
				}
				if (tokens[i + 1].type != LexTokenType::Identifier)
				{
					reportError(tokens[i + 1].lineNumber, "Expected an identifier after a comma.");
					i++;
					break;
				}
				addType(tokens[i + 1].content, tokens[i].lineNumber);
			}
			continue;
		}

		// deals with property declarations
		if (tokens[i].type == LexTokenType::KWProperty)
		{
			if (!checkTokenType(i + 1, LexTokenType::LAngleBra))
			{
				reportError(tokens[i].lineNumber, "Expected an opening angle bracket after the property keyword.");
				if (i + 1 != tokens.size())
					i++;
				continue;
			}
			if (!checkTokenType(i + 2, LexTokenType::Identifier))
			{
				reportError(tokens[i + 1].lineNumber, "Expected a type identifier after an opening angle bracket.");
				if (i + 2 != tokens.size())
					i += 2;
				else
					i++;
				continue;
			}
			StructType* const parentType = universe.getType(tokens[i + 2].content);
			if (!parentType)
			{
				reportError(tokens[i + 2].lineNumber, "The identifier " + tokens[i + 2].content + " doesn't name a type.");
				i += 2;
				continue;
			}
			if (!checkTokenType(i + 3, LexTokenType::RAngleBra))
			{
				reportError(tokens[i + 2].lineNumber, "Expected a closing angle bracket after a type identifier.");
				if (i + 3 != tokens.size())
					i += 3;
				else
					i += 2;
				continue;
			}
			if (!checkTokenType(i + 4, LexTokenType::Identifier))
			{
				reportError(tokens[i + 3].lineNumber, "Expected an identifier for the property name.");
				if (i + 4 != tokens.size())
					i += 4;
				else
					i += 3;
				continue;
			}
			const auto submitProperty = [&](const str& propertyName, const uint32_t lineNumber)
			{
				if (parentType->getProperty(propertyName))
				{
					reportError(lineNumber, "A property " + propertyName + " for type " + parentType->getName() + " has already been defined before.");
					return;
				}
				if (parentType->getMember(propertyName))
				{
					reportError(lineNumber, "Type " + parentType->getName() + " has already had a member named " + propertyName + " defined.");
					return;
				}
				parentType->addProperty(propertyName);
			};
			i += 4;
			submitProperty(tokens[i + 4].content, tokens[i + 4].lineNumber);
			bool good = false;
			while (i + 1 < tokens.size())
			{
				if (tokens[i + 1].type == LexTokenType::Semic)
				{
					good = true;
					i++;
					break;
				}
				if (tokens[i + 1].type == LexTokenType::Comma)
				{
					if (!checkTokenType(i + 2, LexTokenType::Identifier))
					{
						reportError(tokens[i + 1].lineNumber, "Expected a type identifier after a comma.");
						i++;
						break;
					}
					submitProperty(tokens[i + 2].content, tokens[i + 2].lineNumber);
					i += 2;
				}
			}
			if (!good)
				reportError(tokens[i].lineNumber, "Expected a comma or a semicolon to end the property list.");
		}

		// TODO: deal with member declarations
		if (checkTokenType(i, LexTokenType::Identifier) && checkTokenType(i + 1, LexTokenType::Identifier))
		{
			StructType* const declaredType = universe.getType(tokens[i].content);
			if (!declaredType)
			{
				reportError(tokens[i].lineNumber, tokens[i].content + " doesn't name a type.");
				continue;
			}
			StructType* const parentType0 = universe.getType(tokens[i + 1].content);
			if (!parentType0)
			{
				reportError(tokens[i + 1].lineNumber, tokens[i + 1].content + " doesn't name a type.");
				continue;
			}
			if (!checkTokenType(i + 2, LexTokenType::Dot))
			{
				reportError(tokens[i + 1].lineNumber, "Expected a dot after a parent type identifier.");
				continue;
			}
		}

		// TODO: deal with member assignments

		// TODO: deal with property declarations

		// TODO: deal with property relationships

		// TODO: deal with example declarations
	}

	return failed;
}