#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include "Field.hpp"

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
	void addGeneration() { ++m_generation; }
    void clear();
    void add(int bufnr, int line, ::Expression* exp);
private:
	std::vector<Expression> m_lines;
	int m_generation = 0;
};

int matchC(const std::string_view& view, const char_t* term);
struct Function {
	constexpr Function(const char_t* name, ExpressionFactory* factory)
		: name{name}, factory{factory}{}
	const char_t* name;
	ExpressionFactory* factory;
};

extern std::array<Function, 18> FunctionNames;

struct ExNot : public Expression{
    ExNot(Expression* exp) : m_exp{exp} { }
    bool eval(const ObjectProvider& objs) const override final {
        return !m_exp->eval(objs);
    }
    ~ExNot() { delete m_exp; }
private:
    Expression* m_exp;
};

struct ExAnd : public Expression {
    ExAnd(Expression* lh, Expression* rh) : m_exps{lh, rh} {}
    bool eval(const ObjectProvider& objs) const override final {
        return m_exps[0]->eval(objs) && m_exps[1]->eval(objs);
    }
    ~ExAnd() { delete m_exps[0]; delete m_exps[1]; }
private:
    Expression* m_exps[2];
};

struct ExOr : public Expression {
    ExOr(Expression* lh, Expression* rh) : m_exps{lh, rh} {}
    bool eval(const ObjectProvider& objs) const override final {
        return m_exps[0]->eval(objs) || m_exps[1]->eval(objs);
    }
    ~ExOr() { delete m_exps[0]; delete m_exps[1]; }
private:
    Expression* m_exps[2];
};


struct BinFac  {
    virtual Expression* create(int lh, int rh) const = 0;
};
struct ExEQ : public Expression {
    struct Fac : public BinFac {
        Expression* create(int lh, int rh) const override final {
            return new ExEQ(lh, rh);
        }
    };
	ExEQ(int lh, int rh) : m_args{lh, rh} {}
	bool eval(const ObjectProvider& _objP) const override final {
		return _objP.getObject(m_args[0]).getPosition()
			== _objP.getObject(m_args[1]).getPosition();

	}
    static const BinFac* fac() {
        static Fac fac;
        return &fac;
    }
private:
	int m_args[2];
};

struct ExNEQ : public Expression {
    struct Fac : public BinFac {
        Expression* create(int lh, int rh) const override final {
            return new ExNEQ(lh, rh);
        }
    };
	ExNEQ(int lh, int rh) : m_args{lh, rh} {}
	bool eval(const ObjectProvider& _objP) const override final {
		return _objP.getObject(m_args[0]).getPosition()
			!= _objP.getObject(m_args[1]).getPosition();
	}
    static const BinFac* fac() {
        static Fac fac;
        return &fac;
    }
private:
	int m_args[2];
};
