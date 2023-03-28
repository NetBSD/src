/*	$NetBSD: parse_init_declarator.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "parse_init_declarator.c"

/*
 * Test parsing of init-declarator, which occurs as part of a top-level
 * declaration.
 *
 * See also: GCC, c-parser.cpp, function c_parser_declaration_or_fndef.
 */

/* lint1-extra-flags: -X 351 */

int global_var;

int *init_declarator_without_initializer
    __asm("") __attribute__((deprecated));

/* XXX: GCC does not accept this, neither should lint. */
int *init_declarator_without_initializer_wrong_order
    __attribute__((deprecated)) __asm("");

int *init_declarator_with_initializer
    __asm("") __attribute__((deprecated)) = &global_var;

/* XXX: GCC does not accept this, neither should lint. */
int *init_declarator_with_initializer_wrong_order
    __attribute__((deprecated)) __asm("") = &global_var;

/* The attributes may only occur before the initializer, not after it. */
int *init_declarator_with_initializer_attribute_too_late
    __asm("") = &global_var __attribute__((deprecated));
/* expect-1: error: syntax error '__attribute__' [249] */

/* cover cgram_declare, freeyyv */
int original __symbolrename(renamed);
