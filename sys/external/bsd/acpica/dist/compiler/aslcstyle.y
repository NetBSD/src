NoEcho('
/******************************************************************************
 *
 * Module Name: aslcstyle.y - Production rules for symbolic operators
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

')

/*******************************************************************************
 *
 * Production rules for the symbolic (c-style) operators
 *
 ******************************************************************************/

/*
 * ASL Extensions: C-style math/logical operators and expressions.
 * The implementation transforms these operators into the standard
 * AML opcodes and syntax.
 *
 * Supported operators and precedence rules (high-to-low)
 *
 * NOTE: The operator precedence and associativity rules are
 * implemented by the tokens in asltokens.y
 *
 * (left-to-right):
 *  1)      ( ) expr++ expr--
 *
 * (right-to-left):
 *  2)      ! ~
 *
 * (left-to-right):
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
 *
 * (right-to-left):
 *  13)     = += -= *= /= %= <<= >>= &= ^= |=
 */


/*******************************************************************************
 *
 * Basic operations for math and logical expressions.
 *
 ******************************************************************************/

Expression

    /* Unary operators */

    : PARSEOP_EXP_LOGICAL_NOT           {$<n>$ = TrCreateLeafOp (PARSEOP_LNOT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>2,1,$3);}
    | PARSEOP_EXP_NOT                   {$<n>$ = TrCreateLeafOp (PARSEOP_NOT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>2,2,$3,TrCreateNullTargetOp ());}

    | SuperName PARSEOP_EXP_INCREMENT   {$<n>$ = TrCreateLeafOp (PARSEOP_INCREMENT);}
                                        {$$ = TrLinkOpChildren ($<n>3,1,$1);}
    | SuperName PARSEOP_EXP_DECREMENT   {$<n>$ = TrCreateLeafOp (PARSEOP_DECREMENT);}
                                        {$$ = TrLinkOpChildren ($<n>3,1,$1);}

    /* Binary operators: math and logical */

    | TermArg PARSEOP_EXP_ADD           {$<n>$ = TrCreateLeafOp (PARSEOP_ADD);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_DIVIDE        {$<n>$ = TrCreateLeafOp (PARSEOP_DIVIDE);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,4,$1,$4,TrCreateNullTargetOp (),
                                            TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_MODULO        {$<n>$ = TrCreateLeafOp (PARSEOP_MOD);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_MULTIPLY      {$<n>$ = TrCreateLeafOp (PARSEOP_MULTIPLY);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_SHIFT_LEFT    {$<n>$ = TrCreateLeafOp (PARSEOP_SHIFTLEFT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_SHIFT_RIGHT   {$<n>$ = TrCreateLeafOp (PARSEOP_SHIFTRIGHT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_SUBTRACT      {$<n>$ = TrCreateLeafOp (PARSEOP_SUBTRACT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}

    | TermArg PARSEOP_EXP_AND           {$<n>$ = TrCreateLeafOp (PARSEOP_AND);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_OR            {$<n>$ = TrCreateLeafOp (PARSEOP_OR);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}
    | TermArg PARSEOP_EXP_XOR           {$<n>$ = TrCreateLeafOp (PARSEOP_XOR);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,TrCreateNullTargetOp ());}

    | TermArg PARSEOP_EXP_GREATER       {$<n>$ = TrCreateLeafOp (PARSEOP_LGREATER);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}
    | TermArg PARSEOP_EXP_GREATER_EQUAL {$<n>$ = TrCreateLeafOp (PARSEOP_LGREATEREQUAL);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}
    | TermArg PARSEOP_EXP_LESS          {$<n>$ = TrCreateLeafOp (PARSEOP_LLESS);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}
    | TermArg PARSEOP_EXP_LESS_EQUAL    {$<n>$ = TrCreateLeafOp (PARSEOP_LLESSEQUAL);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}

    | TermArg PARSEOP_EXP_EQUAL         {$<n>$ = TrCreateLeafOp (PARSEOP_LEQUAL);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}
    | TermArg PARSEOP_EXP_NOT_EQUAL     {$<n>$ = TrCreateLeafOp (PARSEOP_LNOTEQUAL);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}

    | TermArg PARSEOP_EXP_LOGICAL_AND   {$<n>$ = TrCreateLeafOp (PARSEOP_LAND);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}
    | TermArg PARSEOP_EXP_LOGICAL_OR    {$<n>$ = TrCreateLeafOp (PARSEOP_LOR);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,2,$1,$4);}

    /* Parentheses */

    | PARSEOP_OPEN_PAREN
        Expression
        PARSEOP_CLOSE_PAREN             {$$ = $2;}

    /* Index term -- "= BUF1[5]" on right-hand side of an equals (source) */

    | IndexExpTerm
    ;

    /*
     * Index term -- "BUF1[5] = " or " = BUF1[5] on either the left side
     * of an equals (target) or the right side (source)
     * Currently used in these terms:
     *      Expression
     *      ObjectTypeSource
     *      DerefOfSource
     *      Type6Opcode
     */
IndexExpTerm

    : SuperName
        PARSEOP_EXP_INDEX_LEFT
        TermArg
        PARSEOP_EXP_INDEX_RIGHT         {$$ = TrCreateLeafOp (PARSEOP_INDEX);
                                        TrLinkOpChildren ($$,3,$1,$3,TrCreateNullTargetOp ());}
    ;


/*******************************************************************************
 *
 * All assignment-type operations -- math and logical. Includes simple
 * assignment and compound assignments.
 *
 ******************************************************************************/

EqualsTerm

    /* Allow parens anywhere */

    : PARSEOP_OPEN_PAREN
        EqualsTerm
        PARSEOP_CLOSE_PAREN             {$$ = $2;}

    /* Simple Store() operation */

    | SuperName
        PARSEOP_EXP_EQUALS
        TermArg                         {$$ = TrCreateAssignmentOp ($1, $3);}

    /* Chained equals: (a=RefOf)=b, a=b=c=d etc. */

    | PARSEOP_OPEN_PAREN
        EqualsTerm
        PARSEOP_CLOSE_PAREN
        PARSEOP_EXP_EQUALS
        TermArg                         {$$ = TrCreateAssignmentOp ($2, $5);}

    /* Compound assignments -- Add (operand, operand, target) */

    | TermArg PARSEOP_EXP_ADD_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_ADD);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_DIV_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_DIVIDE);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,4,$1,$4,TrCreateNullTargetOp (),
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_MOD_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_MOD);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_MUL_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_MULTIPLY);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_SHL_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_SHIFTLEFT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_SHR_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_SHIFTRIGHT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_SUB_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_SUBTRACT);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_AND_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_AND);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_OR_EQ         {$<n>$ = TrCreateLeafOp (PARSEOP_OR);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}

    | TermArg PARSEOP_EXP_XOR_EQ        {$<n>$ = TrCreateLeafOp (PARSEOP_XOR);}
        TermArg                         {$$ = TrLinkOpChildren ($<n>3,3,$1,$4,
                                            TrSetOpFlags (TrCreateTargetOp ($1, NULL), OP_IS_TARGET));}
    ;
