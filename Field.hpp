#pragma once

#include <array>
#include <string>
#include <exception>
#include <stdexcept>


static constexpr int FORM_HEIGHT = 4;
static constexpr int FORM_WIDTH = 9;
enum class Size { SMALL, MEDIUM, LARGE, END};
constexpr Size operator+(Size s, int i) { return static_cast<Size>(static_cast<int>(s) + i); }
enum class Form { TET, CUBE, DODEC, END};
constexpr Form operator+(Form f, int i) { return static_cast<Form>(static_cast<int>(f) + i); }
static constexpr std::array<char, 6> symbols =
	{'a', 'b', 'c', 'd', 'e', 'f'};

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

using form_t = const char[FORM_WIDTH*FORM_HEIGHT + 1];
namespace hide {
	template<Size S, Form F>
	static constexpr form_t form = "";

	template<Size S, Form F = static_cast<Form>(0)>
	const form_t&
	getFormForm(Form _f) {
		if constexpr (F != Form::END) {
			if (_f == F) {
				return form<S, F>;
			} else {
				return getFormForm<S,F+1>(_f);
			}
		} else {
			throw FormNotFoundException(S, _f);
		}
	}
}

template<Size S = static_cast<Size>(0)>
const form_t&
getForm(Size _s, Form _f) {
	if constexpr (S != Size::END) {
		if (S == _s) {
			return hide::getFormForm<S>(_f);
		} else {
			return getForm<S+1>(_s, _f);
		}
	} else {
		throw FormNotFoundException(_s, _f);
	}
}


class Field {
public:
};
