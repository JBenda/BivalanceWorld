#include "Field.hpp"


int ColorId = 0;

namespace hide {
	template<Size S, Form F = static_cast<Form>(0)>
	const form_t& getFormForm(Form);
}


template<Size S = Size::BEGIN>
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

const form_t& findForm(Size _s, Form _f) {
	return getForm(_s, _f);
}

namespace hide {
	template<Size S, Form F>
	extern const form_t form;

	template<Size S, Form F>
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
template<>
const form_t hide::form<Size::SMALL, Form::CUBE> =
	"         "
	"   XXX   "
	"   XXX   "
	"         ";

template<>
const form_t hide::form<Size::MEDIUM, Form::CUBE> = 
	"   XXXX  "
	"   XXXX  "
	"   XXXX  "
	"         ";

template<>
const form_t hide::form<Size::LARGE, Form::CUBE> = 
	" XXXXXXX "
	" XXXXXXX "
	" XXXXXXX "
	" XXXXXXX ";

template<>
const form_t hide::form<Size::SMALL, Form::TET> =
	"         "
	"    X    "
	"   XXX   "
	"         ";

template<>
const form_t hide::form<Size::MEDIUM, Form::TET> = 
	"    X    "
	"   XXX   "
	"  XXXXX  "
	"         ";

template<>
const form_t hide::form<Size::LARGE, Form::TET> = 
	"    X    "
	"   XXX   "
	"  XXXXX  "
	" XXXXXXX ";

template<>
const form_t hide::form<Size::SMALL, Form::DODEC> =
	"         "
	"    X    "
	"    X    "
	"         ";

template<>
const form_t hide::form<Size::MEDIUM, Form::DODEC> = 
	"    X    "
	"   XXX   "
	"   XXX   "
	"    X    ";

template<>
const form_t hide::form<Size::LARGE, Form::DODEC> = 
	"   XXX   "
	"  XXXXX  "
	"  XXXXX  "
	"   XXX   ";

const char* toString(Size _s) {
	switch (_s) {
		case Size::SMALL: return "Small";
		case Size::MEDIUM: return "Medium";
		case Size::LARGE: return "Large";
		case Size::END: return nullptr;
	}
	return nullptr;
}

const char* toString(Form _f) {
	switch(_f) {
		case Form::TET: return "Tet";
		case Form::CUBE: return "Cube";
		case Form::DODEC: return "Dodec";
		case Form::END: return nullptr;
	}
	return nullptr;
}

std::array<int, static_cast<int>(Form::END)> FormColor;

void initForm() {
	auto itr = FormColor.begin();
	init_pair(ColorId, COLOR_BLACK, COLOR_MAGENTA);
	*itr++ = ColorId++;
	init_pair(ColorId, COLOR_BLACK, COLOR_CYAN);
	*itr++ = ColorId++;
	init_pair(ColorId, COLOR_BLACK, COLOR_YELLOW);
	*itr++ = ColorId++;
}

auto fetchField(const json& j, const char* _name) {
	if (const auto itr = j.find(_name); itr != j.end()) {
		return *itr;
	} else {
		throw FieldNotFound(_name);
	}
}

Object::Object(const json& j) {
	for (const auto& s : fetchField(j, "Consts")) {
		m_symbols.set(getSymbolIndex(s.get<std::string>()[0]));
	}
	for(const auto& p : fetchField(j, "Predicates")) {
		std::string pred = p.get<std::string>();
		bool match = false;
		if (!m_predicates.form) {
			for(Form f = Form::BEGIN; f != Form::END; f = f+1) {
				if (pred == toString(f)) {
					m_predicates.form = f;
					match = true;
					break;
				}
			}
		}
		if (!match && !m_predicates.size) {
			for(Size s = Size::BEGIN; s != Size::END; s = s+1) {
				if (pred == toString(s)) {
					m_predicates.size = s;
					match = true;
					break;
				}
			}
		}
		if(!match) {throw PredicateNotFound(pred); }
	}
	auto pos = fetchField(j, "Tags");
	m_position = vec2(pos[0], pos[1]);
}

