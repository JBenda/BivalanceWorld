#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>

using syms_t = std::array<int, 4>;
using char_t = char8_t;
using string = std::u8string;

struct SentParseError : public std::runtime_error {
	SentParseError(const string& what) : std::runtime_error(reinterpret_cast<const char*>(what.c_str())) {}
};
class Object;
struct ObjectProvider {
	struct SymbolNotDefined : public std::runtime_error {
	SymbolNotDefined() : std::runtime_error{"Symbol not defined yet!"}
	{}
	};
	virtual const Object& getObject(int _sym) const = 0;
};
struct Expression {
	virtual bool eval(const ObjectProvider& _objP) const = 0;
	virtual ~Expression() = default;
};

struct ExpressionFactory {
	virtual Expression* create(char_t* itr) = 0;
    virtual Expression* create(syms_t syms) = 0;
	virtual ~ExpressionFactory() = default;
};

class ExpressionHandler {
	struct Expression {
		::Expression* tree;
		int bufnr;
		int line;
		int generation = 0;
	};

public:
	char_t* parseCommand(char_t*);
	auto begin() { return m_lines.begin(); }
	auto end() { return m_lines.end(); }
	int generation() const { return m_generation; }
    void clear();
    void add(int bufnr, int line, ::Expression* exp);
private:
	char_t* clear( char_t*);
	char_t* add( char_t*);
	
	std::vector<Expression> m_lines;
	int m_generation = 0;
};
