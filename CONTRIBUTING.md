# Guia de Contribuição — Grupo 12

Este documento estabelece as diretrizes de desenvolvimento, padronização de commits, fluxo de branches e regras de Pull Requests para o compilador de Portugol para C++.

---

## 1. Fluxo de Branches

- **`main`**: branch de produção e entregas oficiais de marcos (ex.: Marco P1). Nenhum commit direto é permitido.
- **`devel`**: branch de integração contínua do desenvolvimento. Todos os Pull Requests de funcionalidades e correções devem ter a `devel` como branch base (alvo).
- **Branches de trabalho**: devem ser criadas a partir da `devel` mais atualizada:
  - `feat/<numero-da-issue>-<descricao-curta>`: para novas funcionalidades.
    - Exemplo: `feat/3-palavras-reservadas-flex`
  - `fix/<numero-da-issue>-<descricao-curta>`: para correções de bugs.
    - Exemplo: `fix/10-recuperacao-erro-lexico`
  - `docs/<numero-da-issue>-<descricao-curta>`: para alterações em documentação.
    - Exemplo: `docs/2-especificacao-gramatica-lexica`
  - `test/<numero-da-issue>-<descricao-curta>`: para inclusão ou ajuste de testes.
    - Exemplo: `test/6-testes-tokens-validos`

---

## 2. Padrão de Commits (Conventional Commits em Português)

As mensagens de commit devem seguir o formato:

```text
<tipo>: <descrição curta no modo imperativo e em letras minúsculas>
```

### Tipos Permitidos:
- **`feat:`** Adição de uma nova funcionalidade (ex.: `feat: adiciona reconhecimento de operadores relacionais`)
- **`fix:`** Correção de um erro ou comportamento inesperado (ex.: `fix: corrige contagem de linhas em comentarios de bloco`)
- **`docs:`** Alteração exclusiva em documentação (ex.: `docs: atualiza tabela de tokens no README`)
- **`test:`** Adição ou correção de testes automatizados (ex.: `test: adiciona casos de teste para literais reais`)
- **`refactor:`** Mudança interna no código que não altera seu comportamento externo (ex.: `refactor: reorganiza funcoes auxiliares do scanner`)
- **`chore:`** Alterações em arquivos de configuração, build ou dependências (ex.: `chore: configura flex no CMakeLists.txt`)

---

## 3. Pull Requests (PR)

1. **Branch de Destino:** Sempre abrir contra a branch **`devel`**.
2. **Vínculo com a Issue:** Indique no corpo do PR `Resolve #<numero>` para vincular e fechar a issue automaticamente.
3. **Revisão:** Atribua ao menos 1 colega do grupo como revisor no painel lateral. A mesclagem é feita após a revisão.
4. **Validação:** Garanta que o projeto compila sem erros (`make` ou `cmake`) e que os testes passaram antes de abrir o PR.

