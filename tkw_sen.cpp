#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <inotifytools/inotify.h>
#include <unistd.h>
 
#include "peglib.h"
#include "Expression.hpp"
#include "Printer.hpp"
#include "Field.hpp"
#include "Utils.hpp"

int fd;
int wd;
void printOnChange(const char* _fileName,
		std::vector<Object>* objs,
		json* wld, VectorProvider* provider, ExpressionHandler* exs) {
	fd = inotify_init();
	if(fd < 0) {
		std::cerr << "failed to setup inotify" << std::endl;
		return;
	}
	wd = inotify_add_watch(fd, _fileName, IN_MODIFY | IN_IGNORED);
	int length;
	char buffer[BUF_LEN];
	bool run = true;
	while(run && (length = read(fd, buffer, BUF_LEN)) > 0) {
		int i = 0; 
		while(i < length) {
			struct inotify_event *event = 
				reinterpret_cast<inotify_event*>(buffer + i);
			if (event->mask & IN_MODIFY) {
				std::ifstream file(_fileName);
				file >> *wld;
				file.close();
				objs->clear();
				for(const auto& el : *wld) {
					objs->push_back(Object(el));
				}
				*provider = std::move(VectorProvider(*objs));
				exs->addGeneration();
				print(*provider, *exs, exs->generation());
			}
			if (event->mask & IN_IGNORED) {
				run = false;
				i = length;
			}
			i += EVENT_SIZE + event->len;
		}
	}
}

int main(int argc, const char** argv) {
	const char* filename = fetchFilename(argc, argv);
	if (!filename) {
		return -1;
	}
	ExpressionHandler exs;
	std::ofstream log("/tmp/MyLog.tmp");
	peg::parser parser(R"==(
        # grammar Sen

		expr <- ("[" cmd "]")+
		cmd <- "c" / "a;" number ";" number ";" "\"" term^invTerm %whitespace "\"" ","?
		number <- [0-9]+
		term <- "¬" %whitespace term  / bool "∧" term / bool %whitespace "∨" term / "(" term %whitespace ")" / bool
		bool <- sym %whitespace bin sym /   func
		sym  <- [a-f]
		bin  <- "=" | "≠" | "!="
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
		/* throw std::runtime_error("Unknown function: " + std::string(vs.token())); */
		return nullptr;
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
		ExpressionFactory* fac = std::any_cast<ExpressionFactory*>(vs[0]);
		if (fac) {
			return fac->create(
					std::any_cast<syms_t>(vs[1]));
		} else {
			return nullptr;
		}
	};
	parser["bin"] = [](const peg::SemanticValues& vs)-> const BinFac* {
		if (vs.token() == "=") {
			return ExEQ::fac();
		} else {
			return ExNEQ::fac();
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
	std::ifstream file(filename);
	file >> wld;
	file.close();
	std::vector<Object> objs;
	for(const auto& elm : wld) {
		objs.push_back(Object(elm));
	}
	VectorProvider provider(objs);
	std::string line;
	std::thread updater(printOnChange,filename, &objs, &wld, &provider, &exs); 

	while(std::getline(std::cin, line)) {
		log << line << std::endl;
		parser.parse(line);
		print(provider, exs, exs.generation());
	}
	inotify_rm_watch(fd, wd);
	updater.join();
	close(fd);
}
