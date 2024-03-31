[//]: # ($NetBSD: README.md,v 1.18 2024/03/31 20:28:45 rillig Exp $)

# Introduction

Lint1 analyzes a single translation unit of C code.

* It reads the output of the C preprocessor, retaining the comments.
* The lexer in `scan.l` and `lex.c` splits the input into tokens.
* The parser in `cgram.y` creates types and expressions from the tokens.
* The checks for declarations are in `decl.c`.
* The checks for initializations are in `init.c`.
* The checks for types and expressions are in `tree.c`.

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
    * migration from traditional C to C90 (default)
    * C90 (`-s`)
    * C99 (`-S`)
    * C11 (`-Ac11`)
    * C23 (`-Ac23`)
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

# Limitations

Lint operates on the level of individual expressions.

* It does not build an AST of the statements of a function, therefore it
  cannot reliably analyze the control flow in a single function.
* It does not store the control flow properties of functions, therefore it
  cannot relate parameter nullability with the return value.
* It does not have information about functions, except for their prototypes,
  therefore it cannot relate them across translation units.
* It does not store detailed information about complex data types, therefore
  it cannot cross-check them across translation units.

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
The operators and their properties are defined in `oper.c`.
Some examples for operators:

| Operator | Meaning                                        |
|----------|------------------------------------------------|
| CON      | compile-time constant in `u.value`             |
| NAME     | references the identifier in `u.sym`           |
| UPLUS    | the unary operator `+u.ops.left`               |
| PLUS     | the binary operator `u.ops.left + u.ops.right` |
| CALL     | a function call                                |
| CVT      | an implicit conversion or an explicit cast     |

As an example, the expression `strcmp(names[i], "name")` has this internal
structure:

~~~text
 1: 'call' type 'int'
 2:   '&' type 'pointer to function(pointer to const char, pointer to const char) returning int'
 3:     'name' 'strcmp' with extern 'function(pointer to const char, pointer to const char) returning int'
 4:   'load' type 'pointer to const char'
 5:     '*' type 'pointer to const char', lvalue
 6:       '+' type 'pointer to pointer to const char'
 7:         'load' type 'pointer to pointer to const char'
 8:           'name' 'names' with auto 'pointer to pointer to const char', lvalue
 9:         '*' type 'long'
10:           'convert' type 'long'
11:             'load' type 'int'
12:               'name' 'i' with auto 'int', lvalue
13:           'constant' type 'long', value 8
14:   'convert' type 'pointer to const char'
15:     '&' type 'pointer to char'
16:       'string' type 'array[5] of char', lvalue, "name"
~~~

| Lines      | Notes                                                       |
|------------|-------------------------------------------------------------|
| 1, 2, 4, 7 | A function call consists of the function and its arguments. |
| 4, 14      | The arguments of a call are ordered from left to right.     |
| 5, 6       | Array access is represented as `*(left + right)`.           |
| 9, 13      | Array and struct offsets are in premultiplied form.         |
| 9          | The type `ptrdiff_t` on this platform is `long`, not `int`. |
| 13         | The size of a pointer on this platform is 8 bytes.          |

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

| Abbr | Expanded                                     |
|------|----------------------------------------------|
| l    | left                                         |
| r    | right                                        |
| o    | old (during type conversions)                |
| n    | new (during type conversions)                |
| op   | operator                                     |
| arg  | the number of the parameter, for diagnostics |

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
An `expect` comment cannot span multiple lines.
At the start and the end of the comment, the placeholder `...` stands for an
arbitrary sequence of characters.
There may be other code or comments in the same line of the `.c` file.

Each diagnostic has its own test `msg_???.c` that triggers the corresponding
diagnostic.
Most other tests focus on a single feature.

## Adding a new test

1. Run `make add-test NAME=test_name`.
2. Run `cd ../../../tests/usr.bin/xlint/lint1`.
3. Make the test generate the desired diagnostics.
4. Run `./accept.sh test_name` until it no longer complains.
5. Run `cd ../../..`.
6. Run `cvs commit distrib/sets/lists/tests/mi tests/usr.bin/xlint`.
