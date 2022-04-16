[//]: # ($NetBSD: README.md,v 1.3 2022/04/16 09:18:33 rillig Exp $)

# Introduction

Lint1 analyzes a single translation unit of C code.

* It reads the output of the C preprocessor, comments are retained.
* The lexer in `scan.l` and `lex.c` splits the input into tokens.
* The parser in `cgram.y` creates types and expressions from the tokens.
* It checks declarations in `decl.c`.
* It checks initializations in `init.c`.
* It checks types and expressions in `tree.c`.

To see how a specific lint message is triggered, read the corresponding unit
test in `tests/usr.bin/xlint/lint1/msg_???.c`.

# Features

## Type checking

Lint has stricter type checking than most C compilers.
It warns about type conversions that may result in alignment problems,
see the test `msg_135.c` for examples.

## Control flow analysis

Lint roughly tracks the control flow inside a single function.
It doesn't follow `goto` statements precisely though,
it rather assumes that each label is reachable.
See the test `msg_193.c` for examples.

## Error handling

Lint tries to continue parsing and checking even after seeing errors.
This part of lint is not robust though, so expect some crashes here,
as variables may not be properly initialized or be null pointers.
The cleanup after handling a parse error is often incomplete.

# Fundamental types

Lint mainly analyzes expressions (`tnode_t`), which are formed from operators
(`op_t`) and their operands (`tnode_t`).
Each node has a type (`type_t`) and a few other properties.

## type_t

The elementary types are `int`, `_Bool`, `unsigned long`, `pointer` and so on,
as defined in `tspec_t`.

Actual types like `int`, `const char *` are created by `gettyp(INT)`,
or by deriving new types from existing types, using `block_derive_pointer`,
`block_derive_array` and `block_derive_function`.
(See [below](#memory-management) for the meaning of the prefix `block_`.)

After a type has been created, it should not be modified anymore.
Ideally all references to types would be `const`, but that's a lot of work.
Before modifying a type,
it needs to be copied using `block_dup_type` or `expr_dup_type`.

## tnode_t

When lint parses an expressions,
it builds a tree of nodes representing the AST.
Each node has an operator, which defines which other members may be accessed.
The operators and their properties are defined in `ops.def`.
Some examples for operators:

| Operator | Meaning                                                 |
|----------|---------------------------------------------------------|
| CON      | compile-time constant in `tn_val`                       |
| NAME     | references the identifier in `tn_sym`                   |
| UPLUS    | the unary operator `+tn_left`                           |
| PLUS     | the binary operator `tn_left + tn_right`                |
| CALL     | a function call, typically CALL(LOAD(NAME("function"))) |
| ICALL    | an indirect function call                               |
| CVT      | an implicit conversion or an explicit cast              |

See `debug_node` for how to interpret the members of `tnode_t`.

## sym_t

There is a single symbol table (`symtab`) for the whole translation unit.
This means that the same identifier may appear multiple times.
To distinguish the identifiers, each symbol has a block level.
Symbols from inner scopes are added to the beginning of the table,
so they are found first when looking for the identifier.

# Memory management

## Block scope

The memory that is allocated by the `block_*_alloc` functions is freed at the
end of analyzing the block, that is, after the closing `}`.
See `compound_statement_rbrace:` in `cgram.y`.

## Expression scope

The memory that is allocated by the `expr_*_alloc` functions is freed at the
end of analyzing the expression.
See `expr_free_all`.

# Null pointers

* Expressions can be null.
    * This typically happens in case of syntax errors or other errors.
* The subtype of a pointer, array or function is never null.

# Common variable names

| Name | Type      | Meaning                                              |
|------|-----------|------------------------------------------------------|
| t    | `tspec_t` | a simple type such as `INT`, `FUNC`, `PTR`           |
| tp   | `type_t`  | a complete type such as `pointer to array[3] of int` |
| stp  | `type_t`  | the subtype of a pointer, array or function          |
| tn   | `tnode_t` | a tree node, mostly used for expressions             |
| op   | `op_t`    | an operator used in an expression                    |
| ln   | `tnode_t` | the left-hand operand of a binary operator           |
| rn   | `tnode_t` | the right-hand operand of a binary operator          |
| sym  | `sym_t`   | a symbol from the symbol table                       |

# Abbreviations in variable names

| Abbr | Expanded                                    |
|------|---------------------------------------------|
| l    | left                                        |
| r    | right                                       |
| o    | old (during type conversions)               |
| n    | new (during type conversions)               |
| op   | operator                                    |
| arg  | the number of the argument, for diagnostics |

# Debugging

Useful breakpoints are:

| Location                      | Remarks                                              |
|-------------------------------|------------------------------------------------------|
| build_binary in tree.c        | Creates an expression for a unary or binary operator |
| initialization_expr in init.c | Checks a single initializer                          |
| expr in tree.c                | Checks a full expression                             |
| typeok in tree.c              | Checks two types for compatibility                   |
| vwarning_at in err.c          | Prints a warning                                     |
| verror_at in err.c            | Prints an error                                      |
| assert_failed in err.c        | Prints the location of a failed assertion            |

# Tests

The tests are in `tests/usr.bin/xlint`.
By default, each test is run with the lint flags `-g` for GNU mode,
`-S` for C99 mode and `-w` to report warnings as errors.

Each test can override the lint flags using comments of the following forms:

* `/* lint1-flags: -tw */` replaces the default flags.
* `/* lint1-extra-flags: -p */` adds to the default flags.

Most tests check the diagnostics that lint generates.
They do this by placing `expect` comments near the location of the diagnostic.
The comment `/* expect+1: ... */` expects a diagnostic to be generated for the
code 1 line below, `/* expect-5: ... */` expects a diagnostic to be generated
for the code 5 lines above.
Each `expect` comment must be in a single line.
There may be other code or comments in the same line.

Each diagnostic has its own test `msg_???.c` that triggers the corresponding
diagnostic.
Most other tests focus on a single feature.

## Adding a new test

1. Run `make -C tests/usr.bin/xlint/lint1 add-test NAME=test_name`.
2. Sort the `FILES` lines in `tests/usr.bin/xlint/lint1/Makefile`.
3. Make the test generate the desired diagnostics.
4. Run `cd tests/usr.bin/xlint/lint1 && sh ./accept.sh test_name`.
6. Run `cvs commit distrib/sets/lists/tests/mi tests/usr.bin/xlint`.
