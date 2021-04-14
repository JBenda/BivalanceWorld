#pragma once

#include <array>
#include <string>
#include <exception>
#include <stdexcept>
#include <bitset>
#include <optional>
#include <compare>

#include <ncurses.h>

#include <iostream>

#include "json.hpp"
using json = nlohmann::json;

extern int ColorId;

struct vec2 {
	vec2(int _x, int _y) : x{_x}, y{_y} {}
	vec2() = default;
	int operator[](int i) {
		if (i == 0) {
			return x;
		} else {
			return y;
		}
	}
	int x = 0;
	int y = 0;
	bool operator==(const vec2& _o) const {
		return x == _o.x && y == _o.y;
	}
	vec2 operator+(const vec2& _o) const {
		return vec2{_o.x + x, _o.y + y};
	}
	vec2 operator-(const vec2& _o) const {
		return vec2{x - _o.x, y - _o.y};
	}
	vec2& operator+=(const vec2& _o) {
		x += _o.x;
		y += _o.y;
		return *this;
	}
};

struct PredicateNotFound : public std::runtime_error {
	PredicateNotFound(const std::string& pred)
		: std::runtime_error{"Predicate not found: "+ pred}{}
};

static constexpr int FORM_HEIGHT = 4;
static constexpr int FORM_WIDTH = 9;
enum class Size { BEGIN, SMALL = BEGIN, MEDIUM, LARGE, END};

constexpr Size operator+(Size s, int i) { return static_cast<Size>(static_cast<int>(s) + i); }
enum class Form { BEGIN, TET = BEGIN, CUBE, DODEC, END};
extern std::array<int, static_cast<int>(Form::END)> FormColor;
constexpr Form operator+(Form f, int i) { return static_cast<Form>(static_cast<int>(f) + i); }

struct SymbolNotFound : public std::runtime_error {
	SymbolNotFound(char c) : std::runtime_error(std::string("Symbol not found: ") + c) {}
};

static constexpr std::array<char, 6> Symbols =
	{'a', 'b', 'c', 'd', 'e', 'f'};
inline int getSymbolIndex(char c) {
	for(std::size_t i = 0; i < Symbols.size(); ++i) {
		if (Symbols[i] == c) {
			return i;
		}
	}
	throw SymbolNotFound(c);
}


const char* toString(Size _s);
const char* toString(Form _f);

struct FormNotFoundException : public std::runtime_error {
	FormNotFoundException(Size s, Form f)
		: std::runtime_error(error(s, f)) {}
private:
	static std::string error(Size s, Form f) {
		return std::string("No matching form found: ")
			+ toString(s) + " " + toString(f);
	}
};

struct FieldNotFound : public std::runtime_error {
	FieldNotFound(const char* _name) : std::runtime_error(
			std::string("Field not found: ") + _name) {}
};

using form_t = const char[FORM_WIDTH*FORM_HEIGHT + 1];


void initForm();

const form_t& findForm(Size _s, Form _f);

class Object {
public:
	Object(const json&);
	void setForm(Form _f) {
		m_predicates.form = _f;
	}
	const form_t& getForm() const {
		return ::findForm(m_predicates.size.value(), m_predicates.form.value());
	}
	int getColor() const {
		return FormColor[static_cast<int>(m_predicates.form.value())];
	}
	void setSize(Size _s) {
		m_predicates.size = _s;
	}
	bool hasSymbol(int i) const {
		if (static_cast<std::size_t>(i) > m_symbols.size()) { return false; }
		return m_symbols[i];
	}
	Size getSize() const {
		return m_predicates.size.value();
	}
	Form getFormType() const {
		return m_predicates.form.value();
	}
	
	int nSymbols() const {
		return m_symbols.count();
	}
	const auto& getSymols() const {
		return m_symbols;
	}

	void setPosition(const vec2& _pos) {
		m_position = _pos;
	}
	void setPosition(int _x, int _y) {
		setPosition(vec2(_x, _y));
	}
	const vec2& getPosition() const {
		return m_position;
	}
	void setSymbol(int _s, bool _set = true) {
		m_symbols.set(_s, _set);
	}
	auto operator<=>(const Object& oth) {
		if (auto cmp = m_position.y <=> oth.m_position.y; cmp != 0) {
			return cmp;
		} else if (auto cmp = m_position.x <=> oth.m_position.x; cmp != 0){
			return cmp;
		}
		return std::strong_ordering::equal;
	}
	auto operator==(const Object& oth) {
		return m_position == oth.m_position;
	}
private:
	std::bitset<Symbols.size()> m_symbols{0};
	struct {
		std::optional<Form> form = std::nullopt;
		std::optional<Size> size = std::nullopt;
	} m_predicates;
	vec2 m_position;
};
