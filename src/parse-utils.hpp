#pragma once

#include <fstream>

#include "print.hpp"
#include "str.hpp"

using std::ostream;

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
	Equivalent,
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

	void reportProc(const str& message)
	{
		*errorStream << "Processing error: " << message << endl;
		reported = true;
	}

	bool getReported() const
	{
		return reported;
	}

	ErrorReporter(ostream& errorStream) : errorStream(&errorStream)
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