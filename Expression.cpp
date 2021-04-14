#include "Expression.hpp"
#include "Field.hpp"

#include <array>
#include <algorithm>

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

int checkIfSymbol(char_t* itr) {
	for(std::size_t i = 0; i < Symbols.size(); ++i) {
		if (*itr == Symbols[i]) {
			return i;
		}
	}
	return -1;
}
constexpr char_t DELIM = ';';
char_t* find(char_t* itr, char_t s = DELIM) {
	while(*itr && *itr != s) {++itr;}
	if (! *itr) {
		char_t d[2] = {s, 0};
		throw SentParseError(u8"expected a '" + string(d) + u8"' to exist");
	}
	return itr;
}

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
	~ExEQ() override = default;
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
	~ExNEQ() override = default;
private:
	int m_args[2];
};

char_t* skip(char_t* itr) {
	while(*itr && (*itr == ' ' || *itr == '\t')){++itr;}
	return itr;
}

bool match(char_t* itr, const char_t* term) {
	while(*term && *itr && *term++ == *itr++);
	return !*term;
}

int matchC(char_t* itr, const char_t* term) {
	while(*term && *itr && *term == *itr) {
		++itr;
		++term;
	}
	if (!*term) { return 0; }
	if (!*itr) { return -1; }
	return *itr < *term ? -1 : 1;
}

int matchC(const std::string_view& view, const char_t* term) {
    auto itr = view.begin();
    auto end = view.end();
    while(*term && itr != end && *term == *itr) {
        ++itr;
        ++term;
    }
    if (!*term) { return 0;}
    if (itr == end) { return -1; }
    return *itr < *term ? -1 : 1;
}

struct Function {
	constexpr Function(const char_t* name, ExpressionFactory* factory)
		: name{name}, factory{factory}{}
	const char_t* name;
	ExpressionFactory* factory;
};
constexpr bool less(const Function& lh, const Function& rh) {
	const char_t* _lh = lh.name;
	const char_t* _rh = rh.name;
	while(*_lh && *_rh && *_rh==*_lh) {
		++_lh;
		++_rh;
	}
	if (*_lh) {
		if (*_rh) {
			return *_lh < *_rh;
		} else {
			return false;
		}
	} else if (*_rh) {
		return true;
	} else {
		return false;
	}
}

syms_t splitSyms(char_t* itr) {
	if (*itr != '(') {
		throw SentParseError(u8"Expected (, got: " + string(itr));
	}
	++itr;
	syms_t syms;
	auto sym = syms.begin();
	*sym = -1;
	while(*itr && *itr != ')') {
		if (*itr == ' ' || *itr == '\t') {}
		else if (*itr == ',')  {
			if (++sym == syms.end()) {
				throw SentParseError(u8"To Many Arguments in Function!");
			}
			*sym = -1;
		} else {
			*sym = checkIfSymbol(itr);
			if (*sym == -1) {
				throw SentParseError(u8"Undefined Symbol: " + string(itr));
			}
		}
		++itr;
	}
	if (++sym != syms.end()) {*sym = -1;}
	if (*itr != ')') {
		throw SentParseError(u8"Expected ), got: " + string(itr));
	}
	return syms;
}

template<int L>
void checkArgs(const syms_t& syms) {
	static constexpr syms_t t{};
	static_assert(L <= t.size(), "Needed arguments are to many");
	for(int i = 0; i < L; ++i) {
		if (syms[i] < 0) {
			throw SentParseError(u8"Not enough arguments provided!");
		}
	}
	if constexpr (L < t.size()) {
		if (syms[L] >= 0) {
			throw SentParseError(u8"To Many Arguments provided!");
		}
	}
}

template<int L, bool (*FN)(const Object**)>
struct ExpressionFactoryInstance : public ExpressionFactory{
	struct ExpressionInstance : public Expression {
		ExpressionInstance(const syms_t& t) : m_syms{t}{}
		bool eval(const ObjectProvider& _objP) const override final {
			std::array<const Object*, L> objs;
			for(int i = 0; i < L; ++i) {
				objs[i] = &_objP.getObject(m_syms[i]);
			}
			return FN(objs.data());
		}
	private:
		syms_t m_syms;
	};
    Expression* create(syms_t syms) override final {
        checkArgs<L>(syms);
        return new ExpressionInstance{syms};
    }
	Expression* create(char_t* itr) override final {
		syms_t syms = splitSyms(itr);
		checkArgs<L>(syms);
		return new ExpressionInstance{syms};
	}
};
bool SameRow(const Object** objs) {
	return objs[0]->getPosition().y
		== objs[1]->getPosition().y;
}
using ExSRFac = ExpressionFactoryInstance<2,SameRow>;

bool Medium(const Object** objs) {
	return objs[0]->getSize() == Size::MEDIUM;
}
using ExMFac = ExpressionFactoryInstance<1, Medium>;

bool Adjoins(const Object** objs) {
	int dx = objs[0]->getPosition().x - objs[1]->getPosition().x;
	int dy = objs[0]->getPosition().y - objs[1]->getPosition().y;
	return (abs(dx) + abs(dy)) == 1;
}
using ExAFac = ExpressionFactoryInstance<2, Adjoins>;

bool Tet(const Object** objs) {
	return objs[0]->getFormType() == Form::TET;
}
using ExTFac = ExpressionFactoryInstance<1, Tet>;

bool SameCol(const Object** objs) {
	return objs[0]->getPosition().x
		== objs[1]->getPosition().x;
}
using ExSCFac = ExpressionFactoryInstance<2, SameCol>;

bool Smaller(const Object** objs) {
	return objs[0]->getSize() < objs[1]->getSize();
}
using ExSmallerFac = ExpressionFactoryInstance<2, Smaller>;

bool BackOf(const Object** objs) {
	return objs[0]->getPosition().y 
		< objs[1]->getPosition().y;
}
using ExBOFac = ExpressionFactoryInstance<2, BackOf>;

bool RightOf(const Object** objs) {
	return objs[0]->getPosition().x
		> objs[1]->getPosition().x;
}
using ExROFac = ExpressionFactoryInstance<2, RightOf>;

bool Dodec(const Object** objs) {
	return objs[0]->getFormType() == Form::DODEC;
}
using ExDFac = ExpressionFactoryInstance<1, Dodec>;

bool SameShape(const Object** objs) {
	return objs[0]->getFormType() == objs[1]->getFormType();
}
using ExSSFac = ExpressionFactoryInstance<2, SameShape>;

bool LeftOf(const Object** objs) {
	return objs[0]->getPosition().x < objs[1]->getPosition().x;
}
using ExLOFac = ExpressionFactoryInstance<2, LeftOf>;

bool Large(const Object** objs) {
	return objs[0]->getSize() == Size::LARGE;
}
using ExLargeFac = ExpressionFactoryInstance<1, Large>;

bool SameSize(const Object** objs) {
	return objs[0]->getSize() == objs[1]->getSize();
}
using ExSameSizeFac = ExpressionFactoryInstance<2, SameSize>;

bool FrontOf(const Object** objs) {
	return objs[0]->getPosition().y > objs[1]->getPosition().y;
}
using ExFOFac = ExpressionFactoryInstance<2, FrontOf>;

bool Small(const Object** objs) {
	return objs[0]->getSize() == Size::SMALL;
}
using ExSmallFac = ExpressionFactoryInstance<1, Small>;

bool Cube(const Object** objs) {
	return objs[0]->getFormType() == Form::CUBE;
}
using ExCFac = ExpressionFactoryInstance<1, Cube>;

bool Between(const Object** objs) {
	vec2 target = objs[0]->getPosition();
	vec2 start = objs[1]->getPosition();
	vec2 end = objs[2]->getPosition();
	vec2 d = end - start;
	if (d.x == 0) {
		if (d.y == 0) { return false; } // cant between one spot
		d.y = d.y < 0 ? -1 : 1;
	} else if (d.y == 0) {
		d.x = d.x < 0 ? -1 : 1;
	} else if (abs(d.x) == abs(d.y)) {
		d.x = d.x < 0 ? -1 : 1;
		d.y = d.y < 0 ? -1 : 1;
	} else {
		// there is no line between arg1 and arg2
		return false;
	}
	vec2 p = start;
	while(p != end) {
		if (p == target) { return true; }
		p += d;
	}
	return false;
}
using ExBFac = ExpressionFactoryInstance<3, Between>;

bool Larger(const Object** objs) {
	return objs[0]->getSize() > objs[1]->getSize();
}
using ExLargerFac = ExpressionFactoryInstance<2, Larger>;

auto FunctionNames = []() {
	std::array<Function, 18> fns = {{
		{u8"SameRow", new ExSRFac{}},
		{u8"Medium", new ExMFac{}},
		{u8"Adjoins",new ExAFac{}},
		{u8"Tet", new ExTFac{}},
		{u8"SameCol", new ExSCFac{}},
		{u8"Smaller", new ExSmallerFac{}},
		{u8"BackOf", new ExBOFac{}},
		{u8"RightOf", new ExROFac{}},
		{u8"Dodec", new ExDFac{}},
		{u8"SameShape", new ExSSFac{}},
		{u8"LeftOf", new ExLOFac{}},
		{u8"Large", new ExLargeFac{}},
		{u8"SameSize", new ExSameSizeFac{}},
		{u8"FrontOf", new ExFOFac{}},
		{u8"Small", new ExSmallFac{}},
		{u8"Cube", new ExCFac{}},
		{u8"Between", new ExBFac{}},
		{u8"Larger", new ExLargerFac{}}
	}};
	std::sort(fns.begin(), fns.end(), less);
	return fns;
}();


Expression* parseExpression(char_t* itr) {
	itr = skip(itr);
	if(int sym = checkIfSymbol(itr); sym >= 0) {
		int first = sym;
		itr = skip(itr+1);
		enum {EQ, NEQ} op;
		if (match(itr,u8"=")) {
			op = EQ;
			itr += strlen(reinterpret_cast<const char*>(u8"="));
		} else if (match(itr,u8"≠")) {
			op = NEQ;
			itr += strlen(reinterpret_cast<const char*>(u8"!="));
		} else {
			throw SentParseError(string(u8"Unknown symbol operator: ") + itr);
		}
		itr = skip(itr);
		int second = 0;
		if ((second = checkIfSymbol(itr)) < 0) {
			throw SentParseError(string(u8"Expected symbol after operator, got: ") + itr);
		}
		switch(op) {
			case EQ: return new ExEQ(first, second);
			case NEQ: return new ExNEQ(first, second);
		}
	}

	auto begin = FunctionNames.begin();
	auto end = FunctionNames.end();
	while(begin != end) {
		auto fn = begin + (end - begin)/2;
		int cmp = matchC(itr, fn->name);
		if (cmp == 0) {
			for(auto b = fn->name; *b != 0; ++b) {++itr;}
			return fn->factory->create(itr);
		} else if (cmp < 0) {
			end = fn;
		} else {
			begin = fn + 1;
		}
	}
	throw SentParseError(string(u8"Function not found: ") + itr);
}



void ExpressionHandler::add(int buf, int ln, ::Expression* exp) {
		m_lines.push_back({exp, buf, ln});
}
char_t* ExpressionHandler::add(char_t* itr) {
	char_t* i = find(itr);
	*i = 0;
	int bufnr = std::atoi(reinterpret_cast<char*>(itr));
	itr = i+1;
	i = find(itr);
	*i = 0;
	int line = std::atoi(reinterpret_cast<char*>(itr));
	itr = i +1;
	char_t* begin =  find(itr, '"') + 1;
	char_t* end = find(begin, '"');
	*end = 0;
	try {
        add(bufnr, line, parseExpression(begin));
	} catch (const SentParseError& err) {
		// TODO: log error
        add(bufnr, line, nullptr);
	}
	return find(end + 1, ']');
}

char_t* ExpressionHandler::parseCommand(char_t*itr) {
	if (*itr++ != '[') {
		throw SentParseError(string(u8"expected [ at start of expression, found: '")
				+ (*(itr-1)) + u8"'");
	}
	if (itr[0] == 'c' && itr[1] == ']') {
		itr = clear(itr+1);
	} else if (*itr == 'a' && itr[1] == ';'){
		itr = add(itr+2);
	} else {
		char_t* i = itr;
		while(*i && *i != ';') {++i;}
		*i = 0;
		throw SentParseError(string(u8"Unknown function: '") + itr + u8"'");
	}
	if (*itr != ']') {
		throw SentParseError(string(u8"expected ] at end of expression, found: '")
				+ itr + u8"'");
	}
	return itr+1;

}
void ExpressionHandler::clear() {
	for(const auto& e : m_lines) {
		delete e.tree;
	}
	m_lines.clear();
	++m_generation;
}
char_t* ExpressionHandler::clear(char_t* itr) {
    clear();
	return itr;
}
