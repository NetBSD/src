[//]: # ($NetBSD: README.md,v 1.10 2023/01/21 21:14:38 rillig Exp $)

# Introduction

Lint1 analyzes a single translation unit of C code.

* It reads the output of the C preprocessor, retaining the comments.
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

In _strict bool mode_, lint treats `bool` as a type that is incompatible with
other scalar types, like in C#, Go, Java.
See the test `d_c99_bool_strict.c` for details.

Lint warns about type conversions that may result in alignment problems.
See the test `msg_135.c` for examples.

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

## Configurable diagnostic messages

Whether lint prints a message and whether each message is an error, a warning
or just informational depends on several things:

* The language level, with its possible values:
    * traditional C (`-t`)
    * migration from traditional C and C90 (default)
    * C90 (`-s`)
    * C99 (`-S`)
    * C11 (`-Ac11`)
* In GCC mode (`-g`), lint allows several GNU extensions,
  reducing the amount of printed messages.
* In strict bool mode (`-T`), lint issues errors when `bool` is mixed with
  other scalar types, reusing the existing messages 107 and 211, while also
  defining new messages that are specific to strict bool mode.
* The option `-a` performs the check for lossy conversions from large integer
  types, the option `-aa` extends this check to small integer types as well,
  reusing the same message ID.
* The option `-X` suppresses arbitrary messages by their message ID.
* The option `-q` enables additional queries that are not suitable as regular
  warnings but may be interesting to look at on a case-by-case basis.

# Fundamental types

Lint mainly analyzes expressions (`tnode_t`), which are formed from operators
(`op_t`) and their operands (`tnode_t`).
Each node has a data type (`type_t`) and a few other properties that depend on
the operator.

## type_t

The basic types are `int`, `_Bool`, `unsigned long`, `pointer` and so on,
as defined in `tspec_t`.

Concrete types like `int` or `const char *` are created by `gettyp(INT)`,
or by deriving new types from existing types, using `block_derive_pointer`,
`block_derive_array` and `block_derive_function`.
(See [below](#memory-management) for the meaning of the prefix `block_`.)

After a type has been created, it should not be modified anymore.
Ideally all references to types would be `const`, but that's still on the
to-do list and not trivial.
In the meantime, before modifying a type,
it needs to be copied using `block_dup_type` or `expr_dup_type`.

## tnode_t

When lint parses an expression,
it builds a tree of nodes representing the AST.
Each node has an operator that defines which other members may be accessed.
The operators and their properties are defined in `ops.def`.
Some examples for operators:

| Operator | Meaning                                    |
|----------|--------------------------------------------|
| CON      | compile-time constant in `tn_val`          |
| NAME     | references the identifier in `tn_sym`      |
| UPLUS    | the unary operator `+tn_left`              |
| PLUS     | the binary operator `tn_left + tn_right`   |
| CALL     | a direct function call                     |
| ICALL    | an indirect function call                  |
| CVT      | an implicit conversion or an explicit cast |

As an example, the expression `strcmp(names[i], "name")` has this internal
structure:

~~~text
 1: 'call' type 'int'
 2:  '&' type 'pointer to function(pointer to const char, pointer to const char) returning int'
 3:    'name' 'strcmp' with extern 'function(pointer to const char, pointer to const char) returning int'
 4:  'push' type 'pointer to const char'
 5:    'convert' type 'pointer to const char'
 6:      '&' type 'pointer to char'
 7:        'string' type 'array[5] of char', lvalue, length 4, "name"
 8:    'push' type 'pointer to const char'
 9:      'load' type 'pointer to const char'
10:        '*' type 'pointer to const char', lvalue
11:          '+' type 'pointer to pointer to const char'
12:            'load' type 'pointer to pointer to const char'
13:              'name' 'names' with auto 'pointer to pointer to const char', lvalue
14:            '*' type 'long'
15:              'convert' type 'long'
16:                'load' type 'int'
17:                  'name' 'i' with auto 'int', lvalue
18:              'constant' type 'long', value 8
~~~

| Lines  | Notes                                                            |
|--------|------------------------------------------------------------------|
| 4, 8   | Each argument of the function call corresponds to a `PUSH` node. |
| 5, 9   | The left operand of a `PUSH` node is the actual argument.        |
| 8      | The right operand is the `PUSH` node of the previous argument.   |
| 5, 9   | The arguments of a call are ordered from right to left.          |
| 10, 11 | Array access is represented as `*(left + right)`.                |
| 14, 18 | Array and struct offsets are in premultiplied form.              |
| 18     | The size of a pointer on this platform is 8 bytes.               |

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

| Function/Code       | File    | Remarks                                              |
|---------------------|---------|------------------------------------------------------|
| build_binary        | tree.c  | Creates an expression for a unary or binary operator |
| initialization_expr | init.c  | Checks a single initializer                          |
| expr                | tree.c  | Checks a full expression                             |
| typeok              | tree.c  | Checks two types for compatibility                   |
| vwarning_at         | err.c   | Prints a warning                                     |
| verror_at           | err.c   | Prints an error                                      |
| assert_failed       | err.c   | Prints the location of a failed assertion            |
| `switch (yyn)`      | cgram.c | Reduction of a grammar rule                          |

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
At the start and the end of the comment, the placeholder `...` stands for an
arbitrary sequence of characters.
There may be other code or comments in the same line of the `.c` file.

Each diagnostic has its own test `msg_???.c` that triggers the corresponding
diagnostic.
Most other tests focus on a single feature.

## Adding a new test

1. Run `make add-test NAME=test_name`.
2. Run `cd ../../../tests/usr.bin/xlint/lint1`.
3. Sort the `FILES` lines in `Makefile`.
4. Make the test generate the desired diagnostics.
5. Run `./accept.sh test_name` until it no longer complains.
6. Run `cd ../../..`.
7. Run `cvs commit distrib/sets/lists/tests/mi tests/usr.bin/xlint`.
