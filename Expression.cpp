#include "Expression.hpp"
#include "Field.hpp"

#include <array>
#include <algorithm>


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

std::array<Function,18> FunctionNames = []() {
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

void ExpressionHandler::add(int buf, int ln, ::Expression* exp) {
		m_lines.push_back({exp, buf, ln});
}

void ExpressionHandler::clear() {
	for(const auto& e : m_lines) {
		delete e.tree;
	}
	m_lines.clear();
	++m_generation;
}
