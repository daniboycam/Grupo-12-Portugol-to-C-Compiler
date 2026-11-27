#ifndef PORTUGOL_TABELA_HPP
#define PORTUGOL_TABELA_HPP

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

namespace portugol {

// Categoria do simbolo na tabela.
enum class CategoriaSimbolo { VARIAVEL, PARAMETRO, FUNCAO };

// Entrada da tabela de simbolos.
struct Simbolo {
    std::string nome;
    std::string tipo;  // tipo da variavel/parametro; "funcao" para funcoes
    int linha;
    CategoriaSimbolo categoria = CategoriaSimbolo::VARIAVEL;
    int dimensao = 0;                           // 0 para escalar, >0 para vetor
    std::string tipo_retorno;                   // apenas para FUNCAO
    std::vector<std::string> tipos_parametros;  // apenas para FUNCAO
    std::string escopo;                         // ex.: "global", "soma", "principal/para"
};

// Tabela de simbolos do programa com suporte a escopos aninhados.
//
// Os escopos formam uma pilha: o global fica sempre na base e cada funcao,
// bloco "{ ... }" ou laco "para" empilha um novo. Um nome so e recusado se ja
// existir no escopo do topo, entao o mesmo nome pode ser usado em funcoes
// diferentes e um escopo interno pode esconder (shadowing) um externo.
//
// Os simbolos de escopos ja fechados continuam guardados, para que a tabela
// completa possa ser impressa depois da analise.
class TabelaSimbolos {
public:
    TabelaSimbolos();

    // Entra em um novo escopo (funcao, bloco ou "para").
    void entrarEscopo(const std::string& nome = "bloco");

    // Sai do escopo atual. O escopo global nunca e removido.
    void sairEscopo();

    // Profundidade da pilha (1 = apenas o global).
    std::size_t nivel() const;

    // Fecha escopos ate a pilha ter "nivel" escopos. Usado pelo parser para
    // fechar tambem os escopos que a recuperacao de erro deixou abertos.
    void voltarAoNivel(std::size_t nivel);

    // Insere o simbolo no escopo atual. Retorna false se ja existir um simbolo
    // com o mesmo nome no MESMO escopo (o simbolo nao e inserido nesse caso).
    bool inserirSimbolo(const std::string& nome,
                        const std::string& tipo,
                        int linha,
                        CategoriaSimbolo cat = CategoriaSimbolo::VARIAVEL,
                        int dimensao = 0,
                        const std::string& tipo_retorno = "",
                        const std::vector<std::string>& tipos_parametros = {});

    // Busca o simbolo de dentro para fora (respeitando shadowing).
    const Simbolo* buscarSimbolo(const std::string& nome) const;

    // Busca o simbolo apenas no escopo atual.
    const Simbolo* buscarSimboloEscopoAtual(const std::string& nome) const;

    // Todos os simbolos cadastrados, inclusive de escopos ja fechados, na
    // ordem em que foram inseridos.
    const std::deque<Simbolo>& simbolos() const;

    // Imprime todos os simbolos cadastrados (inclusive de escopos fechados).
    void imprimirTabela() const;

    // Remove todos os simbolos e reinicia o escopo global.
    void limpar();

    // Numero total de simbolos cadastrados.
    std::size_t tamanho() const;

private:
    struct Escopo {
        std::string nome;
        std::vector<const Simbolo*> simbolos;
    };

    // Caminho do escopo atual, sem o global (ex.: "principal/para").
    std::string caminhoEscopoAtual() const;

    std::vector<Escopo> pilha_;
    // deque: inserir no fim nao invalida os ponteiros guardados nos escopos.
    std::deque<Simbolo> todos_simbolos_;
};

// Instancia global usada pelo parser e pelo driver.
extern TabelaSimbolos tabela;

}  // namespace portugol

#endif  // PORTUGOL_TABELA_HPP
