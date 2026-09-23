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

## 3. Regras de Pull Request (PR)

> ⚠️ **ATENÇÃO: TODO PULL REQUEST EXIGE REVISOR OBRIGATÓRIO!**

1. **Branch de Destino:**
   - O PR deve ser sempre aberto contra a branch **`devel`**.
2. **Vínculo com a Issue:**
   - No corpo do PR, indique a issue relacionada utilizando palavras-chave de fechamento automático:
     - `Resolve #<numero-da-issue>` ou `Closes #<numero-da-issue>`.
3. **Revisor Obrigatório (Code Review):**
   - É **obrigatório** atribuir pelo menos **1 membro do grupo** no painel lateral de **Reviewers**.
   - O autor do PR **NUNCA** deve fazer o merge sem a aprovação explícita de ao menos um colega de equipe.
   - O revisor deve inspecionar o código, verificar conformidade e aprovar (*Approve*) antes da mesclagem.
4. **Verificação antes da Abertura:**
   - O código deve compilar localmente sem erros ou warnings (`make` ou `cmake --build build`).
   - Todos os testes devem estar passando.
   - O pipeline do GitHub Actions (CI) deve passar com status verde.
