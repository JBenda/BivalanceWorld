#include "Field.hpp"

template<>
form_t hide::form<Size::SMALL, Form::CUBE> =
	"         "
	"    XX   "
	"    XX   "
	"         ";

template<>
form_t hide::form<Size::MEDIUM, Form::CUBE> = 
	"   XXXX  "
	"   XXXX  "
	"   XXXX  "
	"         ";

template<>
form_t hide::form<Size::LARGE, Form::CUBE> = 
	" XXXXXXX "
	" XXXXXXX "
	" XXXXXXX "
	" XXXXXXX ";

const char* toString(Size _s) {
	switch (_s) {
		case Size::SMALL: return "Small";
		case Size::MEDIUM: return "Meduium";
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
