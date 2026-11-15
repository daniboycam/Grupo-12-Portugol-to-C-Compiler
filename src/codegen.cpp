#include "portugol/codegen.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <unordered_set>

namespace portugol {

namespace {

// ---- traducao de nomes, tipos e literais ---------------------------------

std::string tipoCpp(const std::string& tipo) {
    if (tipo == "inteiro") return "int";
    if (tipo == "real") return "double";
    if (tipo == "cadeia") return "std::string";
    if (tipo == "caracter") return "char";
    if (tipo == "logico") return "bool";
    return "void";
}

std::string valorPadrao(const std::string& tipo) {
    if (tipo == "inteiro") return "0";
    if (tipo == "real") return "0.0";
    if (tipo == "cadeia") return "\"\"";
    if (tipo == "caracter") return "'\\0'";
    return "false";
}

// Identificadores validos em Portugol que nao podem ser usados em C++.
std::string nomeCpp(const std::string& nome) {
    static const std::unordered_set<std::string> reservadas = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool",
        "break", "case", "catch", "char", "char16_t", "char32_t", "class", "compl", "const",
        "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do",
        "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
        "float", "for", "friend", "goto", "if", "inline", "int", "long", "main", "mutable",
        "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or",
        "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "return",
        "short", "signed", "sizeof", "static", "static_assert", "static_cast", "std",
        "struct", "switch", "template", "this", "thread_local", "throw", "true", "try",
        "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq"};
    return reservadas.count(nome) != 0 ? nome + "_" : nome;
}

std::string escaparByte(unsigned char c, char aspa) {
    switch (c) {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\r': return "\\r";
        case '\\': return "\\\\";
        default: break;
    }
    if (c == static_cast<unsigned char>(aspa)) return std::string("\\") + aspa;
    // Controles (e bytes nao ASCII em literal caracter) viram octal de tres
    // digitos, que nunca se mistura com o caractere seguinte.
    if (c < 0x20 || c == 0x7f || (aspa == '\'' && c >= 0x80)) {
        char buf[8];
        std::snprintf(buf, sizeof buf, "\\%03o", c);
        return buf;
    }
    return std::string(1, static_cast<char>(c));
}

std::string literalCadeia(const std::string& texto) {
    std::string r = "\"";
    for (char c : texto) r += escaparByte(static_cast<unsigned char>(c), '"');
    return r + "\"";
}

std::string literalCaracter(char c) {
    return "'" + escaparByte(static_cast<unsigned char>(c), '\'') + "'";
}

// Menor representacao decimal que volta exatamente ao mesmo double, sempre
// com ponto ou expoente para continuar sendo literal real em C++ (2.0 nao
// pode virar "2", que e inteiro e mudaria o resultado de uma divisao).
std::string literalReal(double v) {
    char buf[40];
    for (int precisao = 1; precisao <= 17; ++precisao) {
        std::snprintf(buf, sizeof buf, "%.*g", precisao, v);
        if (std::strtod(buf, nullptr) == v) break;
    }
    std::string s = buf;
    if (s.find_first_of(".eEn") == std::string::npos) s += ".0";
    return s;
}

// ---- gerador ---------------------------------------------------------

class Gerador {
public:
    std::string gerar(const NoAST* raiz) {
        out_ << "// Codigo C++17 gerado pelo portugolc a partir de um programa Portugol.\n"
             << "#include <iostream>\n"
             << "#include <string>\n\n";
        if (raiz == nullptr) {
            out_ << "int main() {\n    return 0;\n}\n";
            return out_.str();
        }

        bool temFuncoes = false;
        for (const NoAST* item : raiz->filhos) {
            if (item != nullptr && item->tipo == NO_FUNCAO) temFuncoes = true;
        }

        if (!temFuncoes) {
            // Programa sem funcoes: os comandos formam o corpo de main.
            emMain_ = true;
            out_ << "int main() {\n";
            for (const NoAST* item : raiz->filhos) comando(item, 1);
            out_ << "    return 0;\n}\n";
            return out_.str();
        }

        // Prototipos permitem chamar uma funcao definida mais abaixo, como a
        // analise semantica aceita.
        bool algumPrototipo = false;
        for (const NoAST* item : raiz->filhos) {
            if (item != nullptr && item->tipo == NO_FUNCAO && item->nome != "principal") {
                out_ << cabecalho(item, false) << ";\n";
                algumPrototipo = true;
            }
        }
        if (algumPrototipo) out_ << "\n";

        bool anteriorFoiGlobal = false;
        for (const NoAST* item : raiz->filhos) {
            if (item == nullptr) continue;
            if (item->tipo == NO_FUNCAO) {
                if (anteriorFoiGlobal) out_ << "\n";
                funcao(item);
                out_ << "\n";
                anteriorFoiGlobal = false;
            } else {
                anteriorFoiGlobal = true;
                // Unico outro item permitido no nivel do programa: variavel global.
                declaracao(item, 0, true);
            }
        }
        return out_.str();
    }

private:
    std::ostringstream out_;
    bool emMain_ = false;  // dentro de main(): "retorne;" vira "return 0;"

    static std::string recuo(int nivel) { return std::string(static_cast<size_t>(nivel) * 4, ' '); }

    // ---- funcoes ----

    std::string cabecalho(const NoAST* f, bool comAtributos) {
        if (f->nome == "principal") return "int main()";
        std::string s = tipoCpp(f->op) + " " + nomeCpp(f->nome) + "(";
        for (size_t i = 0; i < f->filhos.size(); ++i) {
            const NoAST* p = f->filhos[i];
            if (i > 0) s += ", ";
            if (comAtributos && !p->usado) s += "[[maybe_unused]] ";
            s += tipoCpp(p->op) + " " + nomeCpp(p->nome);
        }
        return s + ")";
    }

    void funcao(const NoAST* f) {
        const bool principal = f->nome == "principal";
        emMain_ = principal;
        out_ << cabecalho(f, true) << " {\n";
        if (f->dir != nullptr) {
            for (const NoAST* item : f->dir->filhos) comando(item, 1);
        }
        const bool terminaComRetorne = f->dir != nullptr && !f->dir->filhos.empty() &&
                                       f->dir->filhos.back() != nullptr &&
                                       f->dir->filhos.back()->tipo == NO_RETORNE;
        if (principal && f->op == "vazio" && !terminaComRetorne) out_ << "    return 0;\n";
        out_ << "}\n";
        emMain_ = false;
    }

    // ---- declaracoes ----

    std::string declarador(const NoAST* d) {
        std::string s = nomeCpp(d->nome);
        if (d->valor >= 0) return s + "[" + std::to_string(d->valor) + "] = {}";
        return s + " = " + (d->esq != nullptr ? expr(d->esq, true) : valorPadrao(d->tipoDado));
    }

    // Uma linha por variavel, para marcar so as que nunca sao lidas.
    void declaracao(const NoAST* n, int nivel, bool global) {
        for (const NoAST* d : n->filhos) {
            out_ << recuo(nivel);
            if (!global && !d->usado) out_ << "[[maybe_unused]] ";
            out_ << tipoCpp(n->op) << " " << declarador(d) << ";\n";
        }
    }

    // Declaracao na inicializacao do "for": todas numa so, com o mesmo tipo.
    std::string declaracaoEmLinha(const NoAST* n) {
        std::string s;
        bool algumaNaoUsada = false;
        for (const NoAST* d : n->filhos) algumaNaoUsada = algumaNaoUsada || !d->usado;
        if (algumaNaoUsada) s += "[[maybe_unused]] ";
        s += tipoCpp(n->op) + " ";
        for (size_t i = 0; i < n->filhos.size(); ++i) {
            if (i > 0) s += ", ";
            s += declarador(n->filhos[i]);
        }
        return s;
    }

    std::string atribuicao(const NoAST* n) {
        return expr(n->esq, true) + " " + n->op + " " + expr(n->dir, true);
    }

    // ---- comandos ----

    void bloco(const NoAST* b, int nivel) {
        if (b != nullptr) {
            for (const NoAST* item : b->filhos) comando(item, nivel);
        }
    }

    void comando(const NoAST* n, int nivel) {
        if (n == nullptr) return;
        const std::string r = recuo(nivel);
        switch (n->tipo) {
            case NO_DECL_VAR:
                declaracao(n, nivel, false);
                break;

            case NO_ATRIBUICAO:
                out_ << r << atribuicao(n) << ";\n";
                break;

            case NO_SE:
                out_ << r;
                se(n, nivel);
                out_ << "\n";
                break;

            case NO_ENQUANTO:
                out_ << r << "while (" << expr(n->esq, true) << ") {\n";
                bloco(n->dir, nivel + 1);
                out_ << r << "}\n";
                break;

            case NO_PARA: {
                std::string init;
                if (n->esq != nullptr) {
                    init = n->esq->tipo == NO_DECL_VAR ? declaracaoEmLinha(n->esq)
                                                       : atribuicao(n->esq);
                }
                const std::string cond = n->dir != nullptr ? " " + expr(n->dir, true) : "";
                const std::string passo = n->extra != nullptr ? " " + atribuicao(n->extra) : " ";
                out_ << r << "for (" << init << ";" << cond << ";" << passo << ") {\n";
                if (!n->filhos.empty()) bloco(n->filhos[0], nivel + 1);
                out_ << r << "}\n";
                break;
            }

            case NO_PARE:
                out_ << r << "break;\n";
                break;

            case NO_CONTINUE:
                out_ << r << "continue;\n";
                break;

            case NO_RETORNE:
                if (n->esq != nullptr) {
                    out_ << r << "return " << expr(n->esq, true) << ";\n";
                } else {
                    out_ << r << (emMain_ ? "return 0;\n" : "return;\n");
                }
                break;

            case NO_LEIA:
                out_ << r << "std::cin >> " << expr(n->esq, true) << ";\n";
                break;

            case NO_ESCREVA:
                out_ << r << "std::cout";
                for (const NoAST* e : n->filhos) {
                    out_ << " << ";
                    if (e->tipoDado == "logico") {
                        out_ << "(" << expr(e, false) << " ? \"verdadeiro\" : \"falso\")";
                    } else {
                        out_ << expr(e, false);
                    }
                }
                out_ << ";\n";
                break;

            case NO_CHAMADA:
                out_ << r << expr(n, true) << ";\n";
                break;

            case NO_BLOCO:
                out_ << r << "{\n";
                bloco(n, nivel + 1);
                out_ << r << "}\n";
                break;

            default:
                // Funcoes aninhadas e expressoes soltas sao barradas antes.
                break;
        }
    }

    // if/else; um "senao se" vira "else if" na mesma linha.
    void se(const NoAST* n, int nivel) {
        out_ << "if (" << expr(n->esq, true) << ") {\n";
        bloco(n->dir, nivel + 1);
        out_ << recuo(nivel) << "}";
        if (n->extra == nullptr) return;
        if (n->extra->tipo == NO_SE) {
            out_ << " else ";
            se(n->extra, nivel);
        } else {
            out_ << " else {\n";
            bloco(n->extra, nivel + 1);
            out_ << recuo(nivel) << "}";
        }
    }

    // ---- expressoes ----

    // Operando de operador binario. Um literal cadeia vira std::string para
    // que concatenacao e comparacao funcionem ("a" + "b" em C++ soma ponteiros).
    std::string operando(const NoAST* e) {
        if (e->tipo == NO_STRING) return "std::string(" + literalCadeia(e->nome) + ")";
        return expr(e, false);
    }

    // "topo" indica que a expressao ja esta isolada (lado direito de
    // atribuicao, argumento, condicao...), dispensando os parenteses externos.
    // Fora disso, toda operacao sai entre parenteses: a precedencia do
    // Portugol fica explicita e o g++ nao emite -Wparentheses.
    std::string expr(const NoAST* e, bool topo) {
        if (e == nullptr) return "";
        switch (e->tipo) {
            case NO_NUM: return std::to_string(e->valor);
            case NO_REAL: return literalReal(e->valorReal);
            case NO_STRING: return literalCadeia(e->nome);
            case NO_CHAR: return literalCaracter(e->valorChar);
            case NO_LOGICO: return e->valor != 0 ? "true" : "false";
            case NO_ID: return nomeCpp(e->nome);
            case NO_ACESSO_VETOR: return nomeCpp(e->nome) + "[" + expr(e->esq, true) + "]";
            case NO_OP: {
                const std::string s = operando(e->esq) + " " + e->op + " " + operando(e->dir);
                return topo ? s : "(" + s + ")";
            }
            case NO_OP_UNARIA: {
                const std::string s = e->op + expr(e->esq, false);
                return topo ? s : "(" + s + ")";
            }
            case NO_CHAMADA: {
                std::string s = nomeCpp(e->nome) + "(";
                for (size_t i = 0; i < e->filhos.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += expr(e->filhos[i], true);
                }
                return s + ")";
            }
            default:
                return "";
        }
    }
};

}  // namespace

std::string gerarCodigoCpp(const NoAST* raiz) {
    Gerador gerador;
    return gerador.gerar(raiz);
}

}  // namespace portugol
