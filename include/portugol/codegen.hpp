#ifndef PORTUGOL_CODEGEN_HPP
#define PORTUGOL_CODEGEN_HPP

#include <string>

#include "portugol/ast.hpp"

namespace portugol {

// Geracao de codigo final: traduz a AST de um programa Portugol para um
// programa C++17 completo, compilavel com
//     g++ -std=c++17 -Wall -Wextra -Wpedantic
//
// Pre-condicao: a arvore passou por analisarSemantica() sem erros. O gerador
// usa as anotacoes deixadas por ela (NoAST::tipoDado e NoAST::usado) e nao
// repete nenhuma verificacao.
//
// Traducao:
// - tipos: inteiro -> int, real -> double, cadeia -> std::string,
//          caracter -> char, logico -> bool, vazio -> void;
// - funcao principal() -> int main(); programa sem funcoes vira o corpo de main;
// - leia(x) -> std::cin >> x;  escreva(a, b) -> std::cout << a << b;
//   valores logicos sao escritos como "verdadeiro"/"falso";
// - se/senao -> if/else, enquanto -> while, para -> for, pare -> break;
// - variaveis sem inicializador recebem o valor padrao do tipo (0, 0.0, "",
//   '\0', false), como no Portugol;
// - todo identificador ganha o prefixo "p_" (x -> p_x), para nao colidir com
//   palavras reservadas, nomes da biblioteca padrao ou macros do C++;
// - uma variavel local que esconde outro nome ganha nome proprio (p2_x...),
//   porque em C++ o nome novo ja valeria no proprio inicializador;
// - caracter entregue a um destino cadeia vira std::string(1, c).
std::string gerarCodigoCpp(const NoAST* raiz);

}  // namespace portugol

#endif  // PORTUGOL_CODEGEN_HPP
