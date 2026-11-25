#ifndef PORTUGOL_TABELA_HPP
#define PORTUGOL_TABELA_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace portugol {

// Categoria do simbolo na tabela.
enum class CategoriaSimbolo { VARIAVEL, PARAMETRO, FUNCAO };

// Entrada da tabela de simbolos.
struct Simbolo {
    std::string nome;
    std::string tipo;
    int linha;
    CategoriaSimbolo categoria = CategoriaSimbolo::VARIAVEL;
    int dimensao = 0; // 0 para escalar, >0 para vetor
    std::string tipo_retorno; // apenas para FUNCAO
    std::vector<std::string> tipos_parametros; // apenas para FUNCAO
};

// Tabela de simbolos do programa com suporte a escopos aninhados.
class TabelaSimbolos {
public:
    TabelaSimbolos();

    // Entra em um novo escopo (funcao ou bloco).
    void entrarEscopo(const std::string& nome = "bloco");
    
    // Sai do escopo atual.
    void sairEscopo();

    // Insere o simbolo no escopo atual. Retorna false se ja existir
    // no MESMO escopo (permitindo shadowing).
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

    // Imprime todos os simbolos cadastrados (inclusive de escopos fechados).
    void imprimirTabela() const;

    // Remove todos os simbolos e reinicia o escopo global.
    void limpar();

    // Numero total de simbolos cadastrados.
    std::size_t tamanho() const;

private:
    struct Escopo {
        std::string nome;
        std::vector<Simbolo> simbolos;
    };
    
    std::vector<Escopo> pilha_;
    std::vector<Simbolo> todos_simbolos_;
};

// Instancia global usada pelo parser e pelo driver.
extern TabelaSimbolos tabela;

}

#endif