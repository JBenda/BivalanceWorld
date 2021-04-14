#pragma once

#include "Field.hpp"
#include "Expression.hpp"
#include <iostream>

class VectorProvider : public ObjectProvider {
public:
	VectorProvider(const std::vector<Object>& _objs)
		: m_objs{_objs}
	{ setupMapping(); }
	const Object& getObject(int _sym) const override final {
		if (m_mapping[_sym] < 0) {
			throw ObjectProvider::SymbolNotDefined{};
		}
		return m_objs[m_mapping[_sym]];
	}
private:
	void setupMapping() {
		for(std::size_t i = 0; i < m_mapping.size(); ++i) {
			m_mapping[i] = -1;
		}
		for(size_t itr = 0; itr < m_objs.size(); ++itr) {
			for(std::size_t i = 0; i < Symbols.size(); ++i) {
				if (m_mapping[i] < 0 && m_objs[itr].hasSymbol(i)) {
					m_mapping[i] = static_cast<int>(itr);
				}
			}
		}
	}
	const std::vector<Object>& m_objs;
	std::array<int, Symbols.size()> m_mapping;
};

template<typename C>
inline void print(const ObjectProvider& _provider, C& _c, int generation) {
	for(auto& exp : _c) {
		if (exp.generation == generation) {continue;}
		if (exp.tree) {
			try {
				bool result = exp.tree->eval(_provider);
				std::cout << (result ? "bs_right;" : "bs_wrong;")
					<< exp.bufnr << ';' << exp.line << std::endl;
			} catch (const ObjectProvider::SymbolNotDefined&) {
				std::cout << "bs_unknown;" << exp.bufnr << ';' << exp.line << std::endl;
			}
		} else {
			std::cout << "bs_error;" << exp.bufnr << ';' << exp.line << std::endl;
		}
		exp.generation = generation;
	}
}
