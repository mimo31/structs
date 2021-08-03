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

	LexToken(const LexTokenType type) : type(type)
	{}

	LexToken(const LexTokenType type, const str& content) : type(type), content(content)
	{
	}
};

using std::endl;

bool tokenize(vec<LexToken>& tokens, istream& defs, ostream& error)
{
	bool failed = false;

	vec<char> chars;
	const str input(std::istreambuf_iterator<char>(defs), {});
	
	bool insideLiteral = false;
	vec<char> literalContent;
	const auto submitLiteral = [&]
	{
		literalContent.push_back('\0');
		tokens.push_back(LexToken(LexTokenType::Literal, str(&literalContent.front())));
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
			tokens.push_back(tokenType == LexTokenType::Identifier ? LexToken(tokenType, id) : LexToken(tokenType));
			identifierStart = -1;
		}
	};

	uint64_t lineNumber = 1;

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
				tokens.push_back(LexToken(tokenType));
				i++;
				return true;
			}
			return false;
		};
		if (checkFor2CharOperator('=', '>', LexTokenType::Implies))
			continue;
		if (checkFor2CharOperator('-', '>', LexTokenType::PromotesTo))
			continue;
		const auto checkForOperator = [&](const char c, const LexTokenType tokenType)
		{
			if (input[i] == c)
			{
				endIdentifier(i);
				tokens.push_back(LexToken(tokenType));
				return true;
			}
			return false;
		};
		const unordered_map<char, LexTokenType> operators{
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
			{';', LexTokenType::Semic}
		};

		if (operators.find(input[i]) != operators.end())
		{
			endIdentifier(i);
			tokens.push_back(LexToken(operators.at(input[i])));
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

bool parse(Universe& p, istream& defs, ostream& error)
{
	bool failed = true;

	vec<LexToken> tokens;
	failed |= tokenize(tokens, defs, error);

	for (uint64_t i = 0; i < tokens.size(); i++)
	{
		
	}
}