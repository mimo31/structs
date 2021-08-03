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
	Exclusive,
	ExclusiveOr,
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

bool tokenize(vec<LexToken>& tokens, istream& defs, ostream& error)
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
					error << "Error @ line " << lineNumber << ": EOF after an escape slash." << endl;
					failed = true;
					continue;
				}
				if (input[i + 1] == '\\')
				{
					literalContent.push_back('\\');
				}
				else if (input[i + 1] == '\"')
				{
					literalContent.push_back('\"');
				}
				else
				{
					error << "Error @ line " << lineNumber << ": Illegal char after an escape slash." << endl;
					failed = true;
				}
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
		error << "Error @ line " << lineNumber << ": Illegal char." << endl;
		failed = true;
	}
	if (insideLiteral)
	{
		submitLiteral();
		error << "Error @ line " << lineNumber << ": EOF inside a string literal." << endl;
	}
	else
	{
		endIdentifier(input.length());
	}

	return failed;
}

bool parse(Universe& universe, istream& defs, ostream& error)
{
	bool failed = true;

	vec<LexToken> tokens;
	failed |= tokenize(tokens, defs, error);

	const auto addType = [&](const str& name, const uint32_t lineNumber)
	{
		if (universe.getType(name))
			error << "Error @ line " << lineNumber << ": The type " << name << " has already been declared before." << endl;
		else
			universe.addType(name);
	};

	for (uint64_t i = 0; i < tokens.size(); i++)
	{
		// deals with type declarations
		if (tokens[i].type == LexTokenType::KWType)
		{
			if (i + 1 == tokens.size() || tokens[i + 1].type != LexTokenType::Identifier)
			{
				error << "Error @ line " << tokens[i + 1].lineNumber << ": Expected an identifier after a type keyword." << endl;
				failed = true;
				if (i + 1 != tokens.size())
				{
					i++;
					continue;
				}
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
					error << "Error @ line " << tokens[i].lineNumber << ": Expected a semicolon or a comma after an identifier." << endl;
					failed = true;
					break;
				}
				if (tokens[i + 1].type != LexTokenType::Identifier)
				{
					error << "Error @ line " << tokens[i + 1].lineNumber << ": Expected an identifier after a comma." << endl;
					i++;
					failed = true;
					break;
				}
				addType(tokens[i + 1].content, tokens[i].lineNumber);
			}
			continue;
		}

		// TODO: deal with member declarations

		// TODO: deal with member assignments

		// TODO: deal with property declarations

		// TODO: deal with property relationships

		// TODO: deal with example declarations
	}

	return failed;
}