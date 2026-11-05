#ifndef PORTUGOL_TABELA_HPP
#define PORTUGOL_TABELA_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace portugol {

// Entrada da tabela de simbolos.
struct Simbolo {
    std::string nome;
    std::string tipo;
    int         linha;
};

// Tabela de simbolos do programa.
//
// Mantem um unico escopo (o global) nesta versao.
class TabelaSimbolos {
public:
    // Insere o simbolo. Retorna false se ja existir um simbolo com o mesmo
    // nome no escopo corrente (o simbolo nao e inserido nesse caso).
    bool inserirSimbolo(const std::string& nome,
                        const std::string& tipo,
                        int linha);

    // Retorna o simbolo com o nome dado, ou nullptr se nao existir.
    const Simbolo* buscarSimbolo(const std::string& nome) const;

    // Imprime todos os simbolos cadastrados, um por linha.
    void imprimirTabela() const;

    // Remove todos os simbolos.
    void limpar();

    // Numero de simbolos cadastrados.
    std::size_t tamanho() const;

private:
    std::vector<Simbolo> simbolos_;
};

// Instancia global usada pelo parser e pelo driver.
extern TabelaSimbolos tabela;

}

#endif