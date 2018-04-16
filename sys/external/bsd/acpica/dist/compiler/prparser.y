%{
/******************************************************************************
 *
 * Module Name: prparser.y - Bison input file for preprocessor parser
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "aslcompiler.h"

#define _COMPONENT          ASL_PREPROCESSOR
        ACPI_MODULE_NAME    ("prparser")

void *                      AslLocalAllocate (unsigned int Size);

/* Bison/yacc configuration */

#undef alloca
#define alloca              AslLocalAllocate

int                         PrParserlex (void);
int                         PrParserparse (void);
void                        PrParsererror (char const *msg);
extern char                 *PrParsertext;

UINT64                      PrParserResult; /* Expression return value */

/* Bison/yacc configuration */

#ifndef yytname
#define yytname             PrParsername
#endif
#define YYDEBUG             1               /* Enable debug output */
#define YYERROR_VERBOSE     1               /* Verbose error messages */
#define YYFLAG              -32768

/* Define YYMALLOC/YYFREE to prevent redefinition errors  */

#define YYMALLOC            malloc
#define YYFREE              free
%}

%union
{
     UINT64                 value;
     UINT32                 op;
     char                   *str;
}

/*! [Begin] no source code translation */

%type  <value>  Expression

%token <op>     EXPOP_EOF
%token <op>     EXPOP_NEW_LINE
%token <op>     EXPOP_NUMBER
%token <op>     EXPOP_HEX_NUMBER
%token <op>     EXPOP_RESERVED1
%token <op>     EXPOP_RESERVED2
%token <op>     EXPOP_PAREN_OPEN
%token <op>     EXPOP_PAREN_CLOSE

%left <op>      EXPOP_LOGICAL_OR
%left <op>      EXPOP_LOGICAL_AND
%left <op>      EXPOP_OR
%left <op>      EXPOP_XOR
%left <op>      EXPOP_AND
%left <op>      EXPOP_EQUAL EXPOP_NOT_EQUAL
%left <op>      EXPOP_GREATER EXPOP_LESS EXPOP_GREATER_EQUAL EXPOP_LESS_EQUAL
%left <op>      EXPOP_SHIFT_RIGHT EXPOP_SHIFT_LEFT
%left <op>      EXPOP_ADD EXPOP_SUBTRACT
%left <op>      EXPOP_MULTIPLY EXPOP_DIVIDE EXPOP_MODULO
%right <op>     EXPOP_ONES_COMPLIMENT EXPOP_LOGICAL_NOT

/* Tokens above must be kept in synch with dtparser.y */

%token <op>     EXPOP_DEFINE
%token <op>     EXPOP_IDENTIFIER

%%

/*
 *  Operator precedence rules (from K&R)
 *
 *  1)      ( )
 *  2)      ! ~ (unary operators that are supported here)
 *  3)      *   /   %
 *  4)      +   -
 *  5)      >>  <<
 *  6)      <   >   <=  >=
 *  7)      ==  !=
 *  8)      &
 *  9)      ^
 *  10)     |
 *  11)     &&
 *  12)     ||
 */

/*! [End] no source code translation !*/

Value
    : Expression EXPOP_NEW_LINE                     { PrParserResult=$1; return 0; } /* End of line (newline) */
    | Expression EXPOP_EOF                          { PrParserResult=$1; return 0; } /* End of string (0) */
    ;

Expression

      /* Unary operators */

    : EXPOP_LOGICAL_NOT         Expression          { $$ = DtDoOperator ($2, EXPOP_LOGICAL_NOT,     $2);}
    | EXPOP_ONES_COMPLIMENT     Expression          { $$ = DtDoOperator ($2, EXPOP_ONES_COMPLIMENT, $2);}

      /* Binary operators */

    | Expression EXPOP_MULTIPLY         Expression  { $$ = DtDoOperator ($1, EXPOP_MULTIPLY,        $3);}
    | Expression EXPOP_DIVIDE           Expression  { $$ = DtDoOperator ($1, EXPOP_DIVIDE,          $3);}
    | Expression EXPOP_MODULO           Expression  { $$ = DtDoOperator ($1, EXPOP_MODULO,          $3);}
    | Expression EXPOP_ADD              Expression  { $$ = DtDoOperator ($1, EXPOP_ADD,             $3);}
    | Expression EXPOP_SUBTRACT         Expression  { $$ = DtDoOperator ($1, EXPOP_SUBTRACT,        $3);}
    | Expression EXPOP_SHIFT_RIGHT      Expression  { $$ = DtDoOperator ($1, EXPOP_SHIFT_RIGHT,     $3);}
    | Expression EXPOP_SHIFT_LEFT       Expression  { $$ = DtDoOperator ($1, EXPOP_SHIFT_LEFT,      $3);}
    | Expression EXPOP_GREATER          Expression  { $$ = DtDoOperator ($1, EXPOP_GREATER,         $3);}
    | Expression EXPOP_LESS             Expression  { $$ = DtDoOperator ($1, EXPOP_LESS,            $3);}
    | Expression EXPOP_GREATER_EQUAL    Expression  { $$ = DtDoOperator ($1, EXPOP_GREATER_EQUAL,   $3);}
    | Expression EXPOP_LESS_EQUAL       Expression  { $$ = DtDoOperator ($1, EXPOP_LESS_EQUAL,      $3);}
    | Expression EXPOP_EQUAL            Expression  { $$ = DtDoOperator ($1, EXPOP_EQUAL,           $3);}
    | Expression EXPOP_NOT_EQUAL        Expression  { $$ = DtDoOperator ($1, EXPOP_NOT_EQUAL,       $3);}
    | Expression EXPOP_AND              Expression  { $$ = DtDoOperator ($1, EXPOP_AND,             $3);}
    | Expression EXPOP_XOR              Expression  { $$ = DtDoOperator ($1, EXPOP_XOR,             $3);}
    | Expression EXPOP_OR               Expression  { $$ = DtDoOperator ($1, EXPOP_OR,              $3);}
    | Expression EXPOP_LOGICAL_AND      Expression  { $$ = DtDoOperator ($1, EXPOP_LOGICAL_AND,     $3);}
    | Expression EXPOP_LOGICAL_OR       Expression  { $$ = DtDoOperator ($1, EXPOP_LOGICAL_OR,      $3);}

      /* Parentheses: '(' Expression ')' */

    | EXPOP_PAREN_OPEN          Expression
        EXPOP_PAREN_CLOSE                           { $$ = $2;}

      /* #if defined (ID) or #if defined ID */

    | EXPOP_DEFINE EXPOP_PAREN_OPEN EXPOP_IDENTIFIER
        EXPOP_PAREN_CLOSE                           { $$ = PrIsDefined (PrParserlval.str);}

    | EXPOP_DEFINE EXPOP_IDENTIFIER                 { $$ = PrIsDefined (PrParserlval.str);}

    | EXPOP_IDENTIFIER                              { $$ = PrResolveDefine (PrParserlval.str);}

      /* Default base for a non-prefixed integer is 10 */

    | EXPOP_NUMBER                                  { AcpiUtStrtoul64 (PrParsertext, &$$);}

      /* Standard hex number (0x1234) */

    | EXPOP_HEX_NUMBER                              { AcpiUtStrtoul64 (PrParsertext, &$$);}
    ;
%%

/*
 * Local support functions, including parser entry point
 */
#define PR_FIRST_PARSE_OPCODE   EXPOP_EOF
#define PR_YYTNAME_START        3


/******************************************************************************
 *
 * FUNCTION:    PrParsererror
 *
 * PARAMETERS:  Message             - Parser-generated error message
 *
 * RETURN:      None
 *
 * DESCRIPTION: Handler for parser errors
 *
 *****************************************************************************/

void
PrParsererror (
    char const              *Message)
{

    sprintf (StringBuffer, "Preprocessor Parser : %s (near line %u)",
        Message, Gbl_CurrentLineNumber);
    DtError (ASL_ERROR, ASL_MSG_SYNTAX,
        NULL, (char *) StringBuffer);
}


/******************************************************************************
 *
 * FUNCTION:    PrGetOpName
 *
 * PARAMETERS:  ParseOpcode         - Parser token (EXPOP_*)
 *
 * RETURN:      Pointer to the opcode name
 *
 * DESCRIPTION: Get the ascii name of the parse opcode for debug output
 *
 *****************************************************************************/

char *
PrGetOpName (
    UINT32                  ParseOpcode)
{
#ifdef ASL_YYTNAME_START
    /*
     * First entries (PR_YYTNAME_START) in yytname are special reserved names.
     * Ignore first 6 characters of name (EXPOP_)
     */
    return ((char *) yytname
        [(ParseOpcode - PR_FIRST_PARSE_OPCODE) + PR_YYTNAME_START] + 6);
#else
    return ("[Unknown parser generator]");
#endif
}


/******************************************************************************
 *
 * FUNCTION:    PrEvaluateExpression
 *
 * PARAMETERS:  ExprString          - Expression to be evaluated. Must be
 *                                    terminated by either a newline or a NUL
 *                                    string terminator
 *
 * RETURN:      64-bit value for the expression
 *
 * DESCRIPTION: Main entry point for the DT expression parser
 *
 *****************************************************************************/

UINT64
PrEvaluateExpression (
    char                    *ExprString)
{

    DbgPrint (ASL_DEBUG_OUTPUT,
        "**** Input expression: %s\n", ExprString);

    /* Point lexer to the input string */

    if (PrInitLexer (ExprString))
    {
        DtError (ASL_ERROR, ASL_MSG_COMPILER_INTERNAL,
            NULL, "Could not initialize lexer");
        return (0);
    }

    /* Parse/Evaluate the input string (value returned in PrParserResult) */

    PrParserparse ();
    PrTerminateLexer ();

    DbgPrint (ASL_DEBUG_OUTPUT,
        "**** Parser returned value: %u (%8.8X%8.8X)\n",
        (UINT32) PrParserResult, ACPI_FORMAT_UINT64 (PrParserResult));

    return (PrParserResult);
}
