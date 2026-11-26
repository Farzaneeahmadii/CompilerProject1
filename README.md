Zahra Ahmadi
```
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o bminor.exe
```

## Run

```sh
./bminor --tokens examples/example.bminor
./bminor --ast examples/example.bminor
./bminor --ansi examples/example.bminor
./bminor --html examples/example.bminor > highlighted.html
```

On Windows, replace `./bminor` with `bminor.exe`.

- `--tokens`: prints the source filename, token type, lexeme, line, and column.
- `--ast`: prints the AST and source positions. Type annotations are empty in
  this phase and are shown as `<null>`.
- `--ansi`: prints the original source with ANSI colors.
- `--html`: writes a standalone HTML document with embedded CSS.

The program returns `0` when no diagnostics are produced, `1` when lexical or
syntax diagnostics are found, and `2` for command-line or input-file errors.

## Project files

- `main.cpp`: lexer, AST, recursive-descent parser, recovery, and output.
- `grammar.ebnf`: grammar used by the parser.
- `lexical-rules.md`: token definitions and matching rules.
- `examples/example.bminor`: a valid sample program.
- `examples/invalid.bminor`: an intentionally invalid sample for diagnostics.

The parser uses a recursive-descent strategy. Expression parsing is organized
by precedence levels, avoiding left recursion. Syntax errors are reported and
the parser skips input to a semicolon or closing brace before continuing.

The B-Minor lecture handout describes `while` as a reserved word, but does not
include a `while` statement in the supported statement forms. For that reason,
`while` is tokenized as a keyword but is not parsed as a statement here.
