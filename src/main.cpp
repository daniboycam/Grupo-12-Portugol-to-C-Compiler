#include <iostream>

extern int yyparse();

int main() {
    int resultado = yyparse();
    if (resultado == 0) {
        std::cout << "Analise sintatica concluida com sucesso.\n";
        return 0;
    } else {
        std::cerr << "Falha na analise sintatica.\n";
        return 1;
    }
}
