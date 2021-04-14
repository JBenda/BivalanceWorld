#include <iostream>
#include <fstream>
#include <string>
 
#include "peglib.h"
#include "Expression.cpp"
#include "Printer.hpp"
#include "Field.hpp"

int main(int argc, const char** argv) {
	ExpressionHandler exs;
	std::ofstream log("/tmp/MyLog.tmp");
	peg::parser parser(R"==(
        # grammar Sen

		expr <- ("[" cmd "]")+
		cmd <- "c" / "a;" number ";" number ";" "\"" term^invTerm %whitespace "\"" ","?
		number <- [0-9]+
		term <- "¬" %whitespace bool  / bool "∧" term / bool %whitespace "∨" term / "(" term %whitespace ")" / bool
		bool <- sym %whitespace bin sym /   func
		sym  <- [a-f]
		bin  <- "=" / "≠"
		func <- fnName "(" args %whitespace ")"
		fnName <- [A-Z][A-Za-z]+
		args <- sym ( %whitespace "," sym )*
		~%whitespace  <-  [ \t\r\n]*

		invTerm <- [^"]*
	)==");
	if (!static_cast<bool>(parser)) {
		std::cerr << "fail!";
		log << "fail!" << std::endl;
		return -1;
	}
	parser["sym"] = [](const peg::SemanticValues& vs) -> int{
		char sym[] = {vs.token()[0], 0};
		for(int i = 0; i < static_cast<int>(Symbols.size()); ++i) {
			if (Symbols[i] == sym[0]) {
				return i;
			}
		}
		return -1;
	};
	parser["fnName"] = [](const peg::SemanticValues& vs) -> ExpressionFactory*{
		auto begin = FunctionNames.begin();
		auto end = FunctionNames.end();
		while(begin != end) {
			auto fn = begin + (end - begin)/2;
			int cmp = matchC(vs.token(), fn->name);
			if (cmp == 0) {
				return fn->factory;
			} else if (cmp < 0) {
				end = fn;
			} else {
				begin = fn + 1;
			}
		}
		throw std::runtime_error("Unknown function: " + std::string(vs.token()));
	};
	parser["args"] = [](const peg::SemanticValues& vs) -> syms_t {
		syms_t syms;
		auto sym = syms.begin();
		for(const auto& el : vs) {
			if (sym == syms.end()) { throw SentParseError(u8"to many arguments!"); }
			*sym = std::any_cast<int>(el);
			++sym;
		}
		if (sym != syms.end()) {
			*sym = -1;
		}
		return syms;
	};
	parser["func"] = [](const peg::SemanticValues& vs) -> Expression* {
		return std::any_cast<ExpressionFactory*>(vs[0])->create(
				std::any_cast<syms_t>(vs[1]));
	};
	parser["bin"] = [](const peg::SemanticValues& vs)-> const BinFac* {
		switch(vs.choice()) {
			case 0: return ExEQ::fac();
			default: return ExNEQ::fac();
		}
	};
	parser["term"] = [](const peg::SemanticValues& vs) -> Expression* {
		switch(vs.choice()) {
			case 0:
				return new ExNot(std::any_cast<Expression*>(vs[0]));
			case 1:
				return new ExAnd(
						std::any_cast<Expression*>(vs[0]),
						std::any_cast<Expression*>(vs[1]));
			case 2:
				return new ExOr(
						std::any_cast<Expression*>(vs[0]),
						std::any_cast<Expression*>(vs[1]));
			default:
				return std::any_cast<Expression*>(vs[0]);
		}
	};
	parser["bool"] = [](const peg::SemanticValues& vs) -> Expression* {
		switch(vs.choice()) {
			case 0:
				return std::any_cast<const BinFac*>(vs[1])->create(
						std::any_cast<int>(vs[0]),
						std::any_cast<int>(vs[2])
					);
			default:
				return std::any_cast<Expression*>(vs[0]);
		}
	};
	parser["number"] = [](const peg::SemanticValues& vs) -> int {
		return vs.token_to_number<int>();
	};

	parser["invTerm"] = [](const peg::SemanticValues&) -> void { };

	parser["cmd"] = [&log, &exs](const peg::SemanticValues& vs) -> void {
		log << "cmd\n";
		log.flush();
		switch(vs.choice()) {
			case 0:
				exs.clear();		
				break;
			case 1:
				exs.add(
						std::any_cast<int>(vs[0]),
						std::any_cast<int>(vs[1]),
						(vs[2].type() == typeid(void))
						? nullptr
						: std::any_cast<Expression*>(vs[2]));
				break;
		}
	};
	parser.enable_packrat_parsing();	
	json wld;
	std::ifstream file("1_1-Wld.wld");
	file >> wld;
	std::vector<Object> objs;
	for(const auto& elm : wld) {
		objs.push_back(Object(elm));
	}
	VectorProvider provider(objs);
	std::string line;
	while(std::getline(std::cin, line)) {
		log << line << std::endl;
		parser.parse(line);
		/*while(*itr == '[') {
			itr = reinterpret_cast<char*>(exs.parseCommand(reinterpret_cast<char_t *>(itr)));	
		}*/
		print(provider, exs, exs.generation());
		/*if (*itr == 0) {
			end = buffer;
		} else {
			char* i = buffer;
			while(*itr) {
				*i++ = *itr++;
			}
			end = i;
		}
		*end = 0;*/
	}
}
