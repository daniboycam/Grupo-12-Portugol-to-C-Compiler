#ifndef PORTUGOL_SEMANTICO_HPP
#define PORTUGOL_SEMANTICO_HPP

#include <iostream>

#include "portugol/ast.hpp"

namespace portugol {

// Analise semantica: percorre a AST produzida pelo parser e verifica
//
// - uso de variaveis, vetores e funcoes nao declarados;
// - redeclaracao no mesmo escopo (global, funcao, bloco, laco "para");
// - compatibilidade de tipos em inicializacoes, atribuicoes (simples e
//   compostas) e expressoes aritmeticas, relacionais, logicas e bit a bit;
// - chamadas de funcao: existencia, aridade e tipo de cada argumento;
// - "retorne": compatibilidade com o tipo da funcao e "retorne;" sem valor
//   apenas em funcoes vazio; funcoes com retorno precisam retornar em todos
//   os caminhos;
// - "pare" e "continue" apenas dentro de laços;
// - indices de vetor do tipo inteiro (e dentro dos limites, quando constantes);
// - declaracao de funcao apenas no nivel do programa.
//
// Cada erro e escrito em "erros" no formato
//     Erro semântico na linha <n>: <mensagem>
// e a analise continua para relatar todos os problemas de uma vez.
//
// Alem de validar, a analise anota a arvore para a geracao de codigo:
// NoAST::tipoDado recebe o tipo de cada expressao e NoAST::usado marca os
// declaradores e parametros lidos em alguma expressao.
//
// Retorna o numero de erros encontrados (0 quando o programa e valido).
int analisarSemantica(NoAST* raiz, std::ostream& erros = std::cerr);

}  // namespace portugol

#endif  // PORTUGOL_SEMANTICO_HPP
