/******************************************************************************
 *
 * Module Name: asltransform - Parse tree transforms
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
#include "aslcompiler.y.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("asltransform")

/* Local prototypes */

static void
TrTransformSubtree (
    ACPI_PARSE_OBJECT       *Op);

static char *
TrAmlGetNextTempName (
    ACPI_PARSE_OBJECT       *Op,
    UINT8                   *TempCount);

static void
TrAmlInitLineNumbers (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *Neighbor);

static void
TrAmlInitNode (
    ACPI_PARSE_OBJECT       *Op,
    UINT16                  ParseOpcode);

static void
TrAmlSetSubtreeParent (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *Parent);

static void
TrAmlInsertPeer (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *NewPeer);

static void
TrDoDefinitionBlock (
    ACPI_PARSE_OBJECT       *Op);

static void
TrDoSwitch (
    ACPI_PARSE_OBJECT       *StartNode);

static void
TrCheckForDuplicateCase (
    ACPI_PARSE_OBJECT       *CaseOp,
    ACPI_PARSE_OBJECT       *Predicate1);

static BOOLEAN
TrCheckForBufferMatch (
    ACPI_PARSE_OBJECT       *Next1,
    ACPI_PARSE_OBJECT       *Next2);

static void
TrDoMethod (
    ACPI_PARSE_OBJECT       *Op);


/*******************************************************************************
 *
 * FUNCTION:    TrAmlGetNextTempName
 *
 * PARAMETERS:  Op              - Current parse op
 *              TempCount       - Current temporary counter. Was originally
 *                                per-module; Currently per method, could be
 *                                expanded to per-scope.
 *
 * RETURN:      A pointer to name (allocated here).
 *
 * DESCRIPTION: Generate an ACPI name of the form _T_x. These names are
 *              reserved for use by the ASL compiler. (_T_0 through _T_Z)
 *
 ******************************************************************************/

static char *
TrAmlGetNextTempName (
    ACPI_PARSE_OBJECT       *Op,
    UINT8                   *TempCount)
{
    char                    *TempName;


    if (*TempCount >= (10 + 26))  /* 0-35 valid: 0-9 and A-Z for TempName[3] */
    {
        /* Too many temps */

        AslError (ASL_ERROR, ASL_MSG_TOO_MANY_TEMPS, Op, NULL);
        return (NULL);
    }

    TempName = UtLocalCalloc (5);

    if (*TempCount < 10)    /* 0-9 */
    {
        TempName[3] = (char) (*TempCount + '0');
    }
    else                    /* 10-35: A-Z */
    {
        TempName[3] = (char) (*TempCount + ('A' - 10));
    }

    (*TempCount)++;

    /* First three characters are always "_T_" */

    TempName[0] = '_';
    TempName[1] = 'T';
    TempName[2] = '_';

    return (TempName);
}


/*******************************************************************************
 *
 * FUNCTION:    TrAmlInitLineNumbers
 *
 * PARAMETERS:  Op              - Op to be initialized
 *              Neighbor        - Op used for initialization values
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialized the various line numbers for a parse node.
 *
 ******************************************************************************/

static void
TrAmlInitLineNumbers (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *Neighbor)
{

    Op->Asl.EndLine           = Neighbor->Asl.EndLine;
    Op->Asl.EndLogicalLine    = Neighbor->Asl.EndLogicalLine;
    Op->Asl.LineNumber        = Neighbor->Asl.LineNumber;
    Op->Asl.LogicalByteOffset = Neighbor->Asl.LogicalByteOffset;
    Op->Asl.LogicalLineNumber = Neighbor->Asl.LogicalLineNumber;
}


/*******************************************************************************
 *
 * FUNCTION:    TrAmlInitNode
 *
 * PARAMETERS:  Op              - Op to be initialized
 *              ParseOpcode     - Opcode for this node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a node with the parse opcode and opcode name.
 *
 ******************************************************************************/

static void
TrAmlInitNode (
    ACPI_PARSE_OBJECT       *Op,
    UINT16                  ParseOpcode)
{

    Op->Asl.ParseOpcode = ParseOpcode;
    UtSetParseOpName (Op);
}


/*******************************************************************************
 *
 * FUNCTION:    TrAmlSetSubtreeParent
 *
 * PARAMETERS:  Op              - First node in a list of peer nodes
 *              Parent          - Parent of the subtree
 *
 * RETURN:      None
 *
 * DESCRIPTION: Set the parent for all peer nodes in a subtree
 *
 ******************************************************************************/

static void
TrAmlSetSubtreeParent (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *Parent)
{
    ACPI_PARSE_OBJECT       *Next;


    Next = Op;
    while (Next)
    {
        Next->Asl.Parent = Parent;
        Next = Next->Asl.Next;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    TrAmlInsertPeer
 *
 * PARAMETERS:  Op              - First node in a list of peer nodes
 *              NewPeer         - Peer node to insert
 *
 * RETURN:      None
 *
 * DESCRIPTION: Insert a new peer node into a list of peers.
 *
 ******************************************************************************/

static void
TrAmlInsertPeer (
    ACPI_PARSE_OBJECT       *Op,
    ACPI_PARSE_OBJECT       *NewPeer)
{

    NewPeer->Asl.Next = Op->Asl.Next;
    Op->Asl.Next = NewPeer;
}


/*******************************************************************************
 *
 * FUNCTION:    TrAmlTransformWalkBegin
 *
 * PARAMETERS:  ASL_WALK_CALLBACK
 *
 * RETURN:      None
 *
 * DESCRIPTION: Parse tree walk to generate both the AML opcodes and the AML
 *              operands.
 *
 ******************************************************************************/

ACPI_STATUS
TrAmlTransformWalkBegin (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level,
    void                    *Context)
{

    TrTransformSubtree (Op);
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    TrAmlTransformWalkEnd
 *
 * PARAMETERS:  ASL_WALK_CALLBACK
 *
 * RETURN:      None
 *
 * DESCRIPTION: Parse tree walk to generate both the AML opcodes and the AML
 *              operands.
 *
 ******************************************************************************/

ACPI_STATUS
TrAmlTransformWalkEnd (
    ACPI_PARSE_OBJECT       *Op,
    UINT32                  Level,
    void                    *Context)
{

    /* Save possible Externals list in the DefintionBlock Op */

    if (Op->Asl.ParseOpcode == PARSEOP_DEFINITION_BLOCK)
    {
        Op->Asl.Value.Arg = AslGbl_ExternalsListHead;
        AslGbl_ExternalsListHead = NULL;
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    TrTransformSubtree
 *
 * PARAMETERS:  Op        - The parent parse node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Prepare nodes to be output as AML data and operands. The more
 *              complex AML opcodes require processing of the child nodes
 *              (arguments/operands).
 *
 ******************************************************************************/

static void
TrTransformSubtree (
    ACPI_PARSE_OBJECT           *Op)
{
    ACPI_PARSE_OBJECT           *MethodOp;
    ACPI_NAMESTRING_INFO        Info;


    if (Op->Asl.AmlOpcode == AML_RAW_DATA_BYTE)
    {
        return;
    }

    switch (Op->Asl.ParseOpcode)
    {
    case PARSEOP_DEFINITION_BLOCK:

        TrDoDefinitionBlock (Op);
        break;

    case PARSEOP_SWITCH:

        TrDoSwitch (Op);
        break;

    case PARSEOP_METHOD:

        TrDoMethod (Op);
        break;

    case PARSEOP_EXTERNAL:

        ExDoExternal (Op);
        break;

    case PARSEOP___METHOD__:

        /* Transform to a string op containing the parent method name */

        Op->Asl.ParseOpcode = PARSEOP_STRING_LITERAL;
        UtSetParseOpName (Op);

        /* Find the parent control method op */

        MethodOp = Op;
        while (MethodOp)
        {
            if (MethodOp->Asl.ParseOpcode == PARSEOP_METHOD)
            {
                /* First child contains the method name */

                MethodOp = MethodOp->Asl.Child;
                Op->Asl.Value.String = MethodOp->Asl.Value.String;
                return;
            }

            MethodOp = MethodOp->Asl.Parent;
        }

        /* At the root, invocation not within a control method */

        Op->Asl.Value.String = "\\";
        break;

    case PARSEOP_NAMESTRING:
        /*
         * A NameString can be up to 255 (0xFF) individual NameSegs maximum
         * (with 254 dot separators) - as per the ACPI specification. Note:
         * Cannot check for NumSegments == 0 because things like
         * Scope(\) are legal and OK.
         */
        Info.ExternalName = Op->Asl.Value.String;
        AcpiNsGetInternalNameLength (&Info);

        if (Info.NumSegments > 255)
        {
            AslError (ASL_ERROR, ASL_MSG_NAMESTRING_LENGTH, Op, NULL);
        }
        break;

    case PARSEOP_UNLOAD:

        AslError (ASL_WARNING, ASL_MSG_UNLOAD, Op, NULL);
        break;

    case PARSEOP_SLEEP:

        /* Remark for very long sleep values */

        if (Op->Asl.Child->Asl.Value.Integer > 1000)
        {
            AslError (ASL_REMARK, ASL_MSG_LONG_SLEEP, Op, NULL);
        }
        break;

    case PARSEOP_PROCESSOR:

        AslError (ASL_WARNING, ASL_MSG_LEGACY_PROCESSOR_OP, Op, Op->Asl.ExternalName);
        break;

    case PARSEOP_OBJECTTYPE_DDB:

        AslError (ASL_WARNING, ASL_MSG_LEGACY_DDB_TYPE, Op, Op->Asl.ExternalName);
        break;

    default:

        /* Nothing to do here for other opcodes */

        break;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    TrDoDefinitionBlock
 *
 * PARAMETERS:  Op        - Parse node
 *
 * RETURN:      None
 *
 * DESCRIPTION: Find the end of the definition block and set a global to this
 *              node. It is used by the compiler to insert compiler-generated
 *              names at the root level of the namespace.
 *
 ******************************************************************************/

static void
TrDoDefinitionBlock (
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT       *Next;
    UINT32                  i;


    /* Reset external list when starting a definition block */

    AslGbl_ExternalsListHead = NULL;

    Next = Op->Asl.Child;
    for (i = 0; i < 5; i++)
    {
        Next = Next->Asl.Next;
        if (i == 0)
        {
            /*
             * This is the table signature. Only the DSDT can be assumed
             * to be at the root of the namespace;  Therefore, namepath
             * optimization can only be performed on the DSDT.
             */
            if (!ACPI_COMPARE_NAMESEG (Next->Asl.Value.String, ACPI_SIG_DSDT))
            {
                AslGbl_ReferenceOptimizationFlag = FALSE;
            }
        }
    }

    AslGbl_FirstLevelInsertionNode = Next;
}


/*******************************************************************************
 *
 * FUNCTION:    TrDoSwitch
 *
 * PARAMETERS:  StartNode        - Parse node for SWITCH
 *
 * RETURN:      None
 *
 * DESCRIPTION: Translate ASL SWITCH statement to if/else pairs. There is
 *              no actual AML opcode for SWITCH -- it must be simulated.
 *
 ******************************************************************************/

static void
TrDoSwitch (
    ACPI_PARSE_OBJECT       *StartNode)
{
    ACPI_PARSE_OBJECT       *Next;
    ACPI_PARSE_OBJECT       *CaseOp = NULL;
    ACPI_PARSE_OBJECT       *CaseBlock = NULL;
    ACPI_PARSE_OBJECT       *DefaultOp = NULL;
    ACPI_PARSE_OBJECT       *CurrentParentNode;
    ACPI_PARSE_OBJECT       *Conditional = NULL;
    ACPI_PARSE_OBJECT       *Predicate;
    ACPI_PARSE_OBJECT       *Peer;
    ACPI_PARSE_OBJECT       *NewOp;
    ACPI_PARSE_OBJECT       *NewOp2;
    ACPI_PARSE_OBJECT       *MethodOp;
    ACPI_PARSE_OBJECT       *StoreOp;
    ACPI_PARSE_OBJECT       *BreakOp;
    ACPI_PARSE_OBJECT       *BufferOp;
    char                    *PredicateValueName;
    UINT16                  Index;
    UINT32                  Btype;


    /* Start node is the Switch() node */

    CurrentParentNode  = StartNode;

    /* Create a new temp name of the form _T_x */

    PredicateValueName = TrAmlGetNextTempName (StartNode, &AslGbl_TempCount);
    if (!PredicateValueName)
    {
        return;
    }

    /* First child is the Switch() predicate */

    Next = StartNode->Asl.Child;

    /*
     * Examine the return type of the Switch Value -
     * must be Integer/Buffer/String
     */
    Index = (UINT16) (Next->Asl.ParseOpcode - ASL_PARSE_OPCODE_BASE);
    Btype = AslKeywordMapping[Index].AcpiBtype;
    if ((Btype != ACPI_BTYPE_INTEGER) &&
        (Btype != ACPI_BTYPE_STRING)  &&
        (Btype != ACPI_BTYPE_BUFFER))
    {
        AslError (ASL_WARNING, ASL_MSG_SWITCH_TYPE, Next, NULL);
        Btype = ACPI_BTYPE_INTEGER;
    }

    /* CASE statements start at next child */

    Peer = Next->Asl.Next;
    while (Peer)
    {
        Next = Peer;
        Peer = Next->Asl.Next;

        if (Next->Asl.ParseOpcode == PARSEOP_CASE)
        {
            TrCheckForDuplicateCase (Next, Next->Asl.Child);

            if (CaseOp)
            {
                /* Add an ELSE to complete the previous CASE */

                NewOp = TrCreateLeafOp (PARSEOP_ELSE);
                NewOp->Asl.Parent = Conditional->Asl.Parent;
                TrAmlInitLineNumbers (NewOp, NewOp->Asl.Parent);

                /* Link ELSE node as a peer to the previous IF */

                TrAmlInsertPeer (Conditional, NewOp);
                CurrentParentNode = NewOp;
            }

            CaseOp = Next;
            Conditional = CaseOp;
            CaseBlock = CaseOp->Asl.Child->Asl.Next;
            Conditional->Asl.Child->Asl.Next = NULL;
            Predicate = CaseOp->Asl.Child;

            if ((Predicate->Asl.ParseOpcode == PARSEOP_PACKAGE) ||
                (Predicate->Asl.ParseOpcode == PARSEOP_VAR_PACKAGE))
            {
                /*
                 * Convert the package declaration to this form:
                 *
                 * If (LNotEqual (Match (Package(<size>){<data>},
                 *                       MEQ, _T_x, MTR, Zero, Zero), Ones))
                 */
                NewOp2              = TrCreateLeafOp (PARSEOP_MATCHTYPE_MEQ);
                Predicate->Asl.Next = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Conditional);

                NewOp               = NewOp2;
                NewOp2              = TrCreateValuedLeafOp (PARSEOP_NAMESTRING,
                                        (UINT64) ACPI_TO_INTEGER (PredicateValueName));
                NewOp->Asl.Next     = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Predicate);

                NewOp               = NewOp2;
                NewOp2              = TrCreateLeafOp (PARSEOP_MATCHTYPE_MTR);
                NewOp->Asl.Next     = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Predicate);

                NewOp               = NewOp2;
                NewOp2              = TrCreateLeafOp (PARSEOP_ZERO);
                NewOp->Asl.Next     = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Predicate);

                NewOp               = NewOp2;
                NewOp2              = TrCreateLeafOp (PARSEOP_ZERO);
                NewOp->Asl.Next     = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Predicate);

                NewOp2              = TrCreateLeafOp (PARSEOP_MATCH);
                NewOp2->Asl.Child   = Predicate;  /* PARSEOP_PACKAGE */
                TrAmlInitLineNumbers (NewOp2, Conditional);
                TrAmlSetSubtreeParent (Predicate, NewOp2);

                NewOp               = NewOp2;
                NewOp2              = TrCreateLeafOp (PARSEOP_ONES);
                NewOp->Asl.Next     = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Conditional);

                NewOp2              = TrCreateLeafOp (PARSEOP_LEQUAL);
                NewOp2->Asl.Child   = NewOp;
                NewOp->Asl.Parent   = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Conditional);
                TrAmlSetSubtreeParent (NewOp, NewOp2);

                NewOp               = NewOp2;
                NewOp2              = TrCreateLeafOp (PARSEOP_LNOT);
                NewOp2->Asl.Child   = NewOp;
                NewOp2->Asl.Parent  = Conditional;
                NewOp->Asl.Parent   = NewOp2;
                TrAmlInitLineNumbers (NewOp2, Conditional);

                Conditional->Asl.Child = NewOp2;
                NewOp2->Asl.Next = CaseBlock;
            }
            else
            {
                /*
                 * Integer and Buffer case.
                 *
                 * Change CaseOp() to:  If (LEqual (SwitchValue, CaseValue)) {...}
                 * Note: SwitchValue is first to allow the CaseValue to be implicitly
                 * converted to the type of SwitchValue if necessary.
                 *
                 * CaseOp->Child is the case value
                 * CaseOp->Child->Peer is the beginning of the case block
                 */
                NewOp = TrCreateValuedLeafOp (PARSEOP_NAMESTRING,
                    (UINT64) ACPI_TO_INTEGER (PredicateValueName));
                NewOp->Asl.Next = Predicate;
                TrAmlInitLineNumbers (NewOp, Predicate);

                NewOp2              = TrCreateLeafOp (PARSEOP_LEQUAL);
                NewOp2->Asl.Parent  = Conditional;
                NewOp2->Asl.Child   = NewOp;
                TrAmlInitLineNumbers (NewOp2, Conditional);

                TrAmlSetSubtreeParent (NewOp, NewOp2);

                Predicate           = NewOp2;
                Predicate->Asl.Next = CaseBlock;

                TrAmlSetSubtreeParent (Predicate, Conditional);
                Conditional->Asl.Child = Predicate;
            }

            /* Reinitialize the CASE node to an IF node */

            TrAmlInitNode (Conditional, PARSEOP_IF);

            /*
             * The first CASE(IF) is not nested under an ELSE.
             * All other CASEs are children of a parent ELSE.
             */
            if (CurrentParentNode == StartNode)
            {
                Conditional->Asl.Next = NULL;
            }
            else
            {
                /*
                 * The IF is a child of previous IF/ELSE. It
                 * is therefore without peer.
                 */
                CurrentParentNode->Asl.Child = Conditional;
                Conditional->Asl.Parent      = CurrentParentNode;
                Conditional->Asl.Next        = NULL;
            }
        }
        else if (Next->Asl.ParseOpcode == PARSEOP_DEFAULT)
        {
            if (DefaultOp)
            {
                /*
                 * More than one Default
                 * (Parser does not catch this, must check here)
                 */
                AslError (ASL_ERROR, ASL_MSG_MULTIPLE_DEFAULT, Next, NULL);
            }
            else
            {
                /* Save the DEFAULT node for later, after CASEs */

                DefaultOp = Next;
            }
        }
        else
        {
            /* Unknown peer opcode */

            AcpiOsPrintf ("Unknown parse opcode for switch statement: %s (%u)\n",
                Next->Asl.ParseOpName, Next->Asl.ParseOpcode);
        }
    }

    /* Add the default case at the end of the if/else construct */

    if (DefaultOp)
    {
        /* If no CASE statements, this is an error - see below */

        if (CaseOp)
        {
            /* Convert the DEFAULT node to an ELSE */

            TrAmlInitNode (DefaultOp, PARSEOP_ELSE);
            DefaultOp->Asl.Parent = Conditional->Asl.Parent;

            /* Link ELSE node as a peer to the previous IF */

            TrAmlInsertPeer (Conditional, DefaultOp);
        }
    }

    if (!CaseOp)
    {
        AslError (ASL_ERROR, ASL_MSG_NO_CASES, StartNode, NULL);
    }


    /*
     * Create a Name(_T_x, ...) statement. This statement must appear at the
     * method level, in case a loop surrounds the switch statement and could
     * cause the name to be created twice (error).
     */

    /* Create the Name node */

    Predicate = StartNode->Asl.Child;
    NewOp = TrCreateLeafOp (PARSEOP_NAME);
    TrAmlInitLineNumbers (NewOp, StartNode);

    /* Find the parent method */

    Next = StartNode;
    while ((Next->Asl.ParseOpcode != PARSEOP_METHOD) &&
           (Next->Asl.ParseOpcode != PARSEOP_DEFINITION_BLOCK))
    {
        Next = Next->Asl.Parent;
    }
    MethodOp = Next;

    NewOp->Asl.CompileFlags |= OP_COMPILER_EMITTED;
    NewOp->Asl.Parent = Next;

    /* Insert name after the method name and arguments */

    Next = Next->Asl.Child; /* Name */
    Next = Next->Asl.Next;  /* NumArgs */
    Next = Next->Asl.Next;  /* SerializeRule */

    /*
     * If method is not Serialized, we must make is so, because of the way
     * that Switch() must be implemented -- we cannot allow multiple threads
     * to execute this method concurrently since we need to create local
     * temporary name(s).
     */
    if (Next->Asl.ParseOpcode != PARSEOP_SERIALIZERULE_SERIAL)
    {
        AslError (ASL_REMARK, ASL_MSG_SERIALIZED, MethodOp,
            "Due to use of Switch operator");
        Next->Asl.ParseOpcode = PARSEOP_SERIALIZERULE_SERIAL;
    }

    Next = Next->Asl.Next;  /* SyncLevel */
    Next = Next->Asl.Next;  /* ReturnType */
    Next = Next->Asl.Next;  /* ParameterTypes */

    TrAmlInsertPeer (Next, NewOp);
    TrAmlInitLineNumbers (NewOp, Next);

    /* Create the NameSeg child for the Name node */

    NewOp2 = TrCreateValuedLeafOp (PARSEOP_NAMESEG,
        (UINT64) ACPI_TO_INTEGER (PredicateValueName));
    TrAmlInitLineNumbers (NewOp2, NewOp);
    NewOp2->Asl.CompileFlags |= OP_IS_NAME_DECLARATION;
    NewOp->Asl.Child  = NewOp2;

    /* Create the initial value for the Name. Btype was already validated above */

    switch (Btype)
    {
    case ACPI_BTYPE_INTEGER:

        NewOp2->Asl.Next = TrCreateValuedLeafOp (PARSEOP_ZERO,
            (UINT64) 0);
        TrAmlInitLineNumbers (NewOp2->Asl.Next, NewOp);
        break;

    case ACPI_BTYPE_STRING:

        NewOp2->Asl.Next = TrCreateValuedLeafOp (PARSEOP_STRING_LITERAL,
            (UINT64) ACPI_TO_INTEGER (""));
        TrAmlInitLineNumbers (NewOp2->Asl.Next, NewOp);
        break;

    case ACPI_BTYPE_BUFFER:

        (void) TrLinkPeerOp (NewOp2, TrCreateValuedLeafOp (PARSEOP_BUFFER,
            (UINT64) 0));
        Next = NewOp2->Asl.Next;
        TrAmlInitLineNumbers (Next, NewOp2);

        (void) TrLinkOpChildren (Next, 1, TrCreateValuedLeafOp (PARSEOP_ZERO,
            (UINT64) 1));
        TrAmlInitLineNumbers (Next->Asl.Child, Next);

        BufferOp = TrCreateValuedLeafOp (PARSEOP_DEFAULT_ARG, (UINT64) 0);
        TrAmlInitLineNumbers (BufferOp, Next->Asl.Child);
        (void) TrLinkPeerOp (Next->Asl.Child, BufferOp);

        TrAmlSetSubtreeParent (Next->Asl.Child, Next);
        break;

    default:

        break;
    }

    TrAmlSetSubtreeParent (NewOp2, NewOp);

    /*
     * Transform the Switch() into a While(One)-Break node.
     * And create a Store() node which will be used to save the
     * Switch() value. The store is of the form: Store (Value, _T_x)
     * where _T_x is the temp variable.
     */
    TrAmlInitNode (StartNode, PARSEOP_WHILE);
    NewOp = TrCreateLeafOp (PARSEOP_ONE);
    TrAmlInitLineNumbers (NewOp, StartNode);
    NewOp->Asl.Next = Predicate->Asl.Next;
    NewOp->Asl.Parent = StartNode;
    StartNode->Asl.Child = NewOp;

    /* Create a Store() node */

    StoreOp = TrCreateLeafOp (PARSEOP_STORE);
    TrAmlInitLineNumbers (StoreOp, NewOp);
    StoreOp->Asl.Parent = StartNode;
    TrAmlInsertPeer (NewOp, StoreOp);

    /* Complete the Store subtree */

    StoreOp->Asl.Child = Predicate;
    Predicate->Asl.Parent = StoreOp;

    NewOp = TrCreateValuedLeafOp (PARSEOP_NAMESEG,
        (UINT64) ACPI_TO_INTEGER (PredicateValueName));
    TrAmlInitLineNumbers (NewOp, StoreOp);
    NewOp->Asl.Parent    = StoreOp;
    Predicate->Asl.Next  = NewOp;

    /* Create a Break() node and insert it into the end of While() */

    Conditional = StartNode->Asl.Child;
    while (Conditional->Asl.Next)
    {
        Conditional = Conditional->Asl.Next;
    }

    BreakOp = TrCreateLeafOp (PARSEOP_BREAK);
    TrAmlInitLineNumbers (BreakOp, NewOp);
    BreakOp->Asl.Parent = StartNode;
    TrAmlInsertPeer (Conditional, BreakOp);
}


/*******************************************************************************
 *
 * FUNCTION:    TrCheckForDuplicateCase
 *
 * PARAMETERS:  CaseOp          - Parse node for first Case statement in list
 *              Predicate1      - Case value for the input CaseOp
 *
 * RETURN:      None
 *
 * DESCRIPTION: Check for duplicate case values. Currently, only handles
 *              Integers, Strings and Buffers. No support for Package objects.
 *
 ******************************************************************************/

static void
TrCheckForDuplicateCase (
    ACPI_PARSE_OBJECT       *CaseOp,
    ACPI_PARSE_OBJECT       *Predicate1)
{
    ACPI_PARSE_OBJECT       *Next;
    ACPI_PARSE_OBJECT       *Predicate2;


    /* Walk the list of CASE opcodes */

    Next = CaseOp->Asl.Next;
    while (Next)
    {
        if (Next->Asl.ParseOpcode == PARSEOP_CASE)
        {
            /* Emit error only once */

            if (Next->Asl.CompileFlags & OP_IS_DUPLICATE)
            {
                goto NextCase;
            }

            /* Check for a duplicate plain integer */

            Predicate2 = Next->Asl.Child;
            if ((Predicate1->Asl.ParseOpcode == PARSEOP_INTEGER) &&
                (Predicate2->Asl.ParseOpcode == PARSEOP_INTEGER))
            {
                if (Predicate1->Asl.Value.Integer == Predicate2->Asl.Value.Integer)
                {
                    goto FoundDuplicate;
                }
            }

            /* Check for pairs of the constants ZERO, ONE, ONES */

            else if (((Predicate1->Asl.ParseOpcode == PARSEOP_ZERO) &&
                (Predicate2->Asl.ParseOpcode == PARSEOP_ZERO)) ||
                ((Predicate1->Asl.ParseOpcode == PARSEOP_ONE) &&
                (Predicate2->Asl.ParseOpcode == PARSEOP_ONE)) ||
                ((Predicate1->Asl.ParseOpcode == PARSEOP_ONES) &&
                (Predicate2->Asl.ParseOpcode == PARSEOP_ONES)))
            {
                goto FoundDuplicate;
            }

            /* Check for a duplicate string constant (literal) */

            else if ((Predicate1->Asl.ParseOpcode == PARSEOP_STRING_LITERAL) &&
                (Predicate2->Asl.ParseOpcode == PARSEOP_STRING_LITERAL))
            {
                if (!strcmp (Predicate1->Asl.Value.String,
                        Predicate2->Asl.Value.String))
                {
                    goto FoundDuplicate;
                }
            }

            /* Check for a duplicate buffer constant */

            else if ((Predicate1->Asl.ParseOpcode == PARSEOP_BUFFER) &&
                (Predicate2->Asl.ParseOpcode == PARSEOP_BUFFER))
            {
                if (TrCheckForBufferMatch (Predicate1->Asl.Child,
                        Predicate2->Asl.Child))
                {
                    goto FoundDuplicate;
                }
            }
        }
        goto NextCase;

FoundDuplicate:
        /* Emit error message only once */

        Next->Asl.CompileFlags |= OP_IS_DUPLICATE;

        AslDualParseOpError (ASL_ERROR, ASL_MSG_DUPLICATE_CASE, Next,
            Next->Asl.Value.String, ASL_MSG_CASE_FOUND_HERE, CaseOp,
            CaseOp->Asl.ExternalName);

NextCase:
        Next = Next->Asl.Next;
    }
}

/*******************************************************************************
 *
 * FUNCTION:    TrBufferIsAllZero
 *
 * PARAMETERS:  Op          - Parse node for first opcode in buffer initializer
 *                            list
 *
 * RETURN:      TRUE if buffer contains all zeros or a DEFAULT_ARG
 *
 * DESCRIPTION: Check for duplicate Buffer case values.
 *
 ******************************************************************************/

static BOOLEAN
TrBufferIsAllZero (
    ACPI_PARSE_OBJECT       *Op)
{
    while (Op)
    {
        if (Op->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
        {
            return (TRUE);
        }
        else if (Op->Asl.Value.Integer != 0)
        {
            return (FALSE);
        }

        Op = Op->Asl.Next;
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    TrCheckForBufferMatch
 *
 * PARAMETERS:  Next1       - Parse node for first opcode in first buffer list
 *                              (The DEFAULT_ARG or INTEGER node)
 *              Next2       - Parse node for first opcode in second buffer list
 *                              (The DEFAULT_ARG or INTEGER node)
 *
 * RETURN:      TRUE if buffers match, FALSE otherwise
 *
 * DESCRIPTION: Check for duplicate Buffer case values.
 *
 ******************************************************************************/

static BOOLEAN
TrCheckForBufferMatch (
    ACPI_PARSE_OBJECT       *NextOp1,
    ACPI_PARSE_OBJECT       *NextOp2)
{
    /*
     * The buffer length can be a DEFAULT_ARG or INTEGER. If any of the nodes
     * are DEFAULT_ARG, it means that the length has yet to be computed.
     * However, the initializer list can be compared to determine if these two
     * buffers match.
     */
    if ((NextOp1->Asl.ParseOpcode == PARSEOP_INTEGER &&
        NextOp2->Asl.ParseOpcode == PARSEOP_INTEGER) &&
        NextOp1->Asl.Value.Integer != NextOp2->Asl.Value.Integer)
    {
        return (FALSE);
    }

    /*
     * Buffers that have explicit lengths but no initializer lists are
     * filled with zeros at runtime. This is equivalent to buffers that have the
     * same length that are filled with zeros.
     *
     * In other words, the following buffers are equivalent:
     *
     * Buffer(0x4) {}
     * Buffer() {0x0, 0x0, 0x0, 0x0}
     *
     * This statement checks for matches where one buffer does not have an
     * initializer list and another buffer contains all zeros.
     */
    if (NextOp1->Asl.ParseOpcode != NextOp2->Asl.ParseOpcode &&
        TrBufferIsAllZero (NextOp1->Asl.Next) &&
        TrBufferIsAllZero (NextOp2->Asl.Next))
    {
        return (TRUE);
    }

    /* Start at the BYTECONST initializer node list */

    NextOp1 = NextOp1->Asl.Next;
    NextOp2 = NextOp2->Asl.Next;

    /*
     * Walk both lists until either a mismatch is found, or one or more
     * end-of-lists are found
     */
    while (NextOp1 && NextOp2)
    {
        if ((NextOp1->Asl.ParseOpcode == PARSEOP_STRING_LITERAL) &&
            (NextOp2->Asl.ParseOpcode == PARSEOP_STRING_LITERAL))
        {
            if (!strcmp (NextOp1->Asl.Value.String, NextOp2->Asl.Value.String))
            {
                return (TRUE);
            }
            else
            {
                return (FALSE);
            }
        }
        if ((UINT8) NextOp1->Asl.Value.Integer != (UINT8) NextOp2->Asl.Value.Integer)
        {
            return (FALSE);
        }

        NextOp1 = NextOp1->Asl.Next;
        NextOp2 = NextOp2->Asl.Next;
    }

    /* Not a match if one of the lists is not at end-of-list */

    if (NextOp1 || NextOp2)
    {
        return (FALSE);
    }

    /* Otherwise, the buffers match */

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    TrDoMethod
 *
 * PARAMETERS:  Op               - Parse node for SWITCH
 *
 * RETURN:      None
 *
 * DESCRIPTION: Determine that parameter count of an ASL method node by
 *              translating the parameter count parse node from
 *              PARSEOP_DEFAULT_ARG to PARSEOP_BYTECONST.
 *
 ******************************************************************************/

static void
TrDoMethod (
    ACPI_PARSE_OBJECT       *Op)
{
    ACPI_PARSE_OBJECT           *ArgCountOp;
    UINT8                       ArgCount;
    ACPI_PARSE_OBJECT           *ParameterOp;


    /*
     * TBD: Zero the tempname (_T_x) count. Probably shouldn't be a global,
     * however
     */
    AslGbl_TempCount = 0;

    ArgCountOp = Op->Asl.Child->Asl.Next;
    if (ArgCountOp->Asl.ParseOpcode == PARSEOP_BYTECONST)
    {
        /*
         * Parameter count for this method has already been recorded in the
         * method declaration.
         */
        return;
    }

    /*
     * Parameter count has been omitted in the method declaration.
     * Count the amount of arguments here.
     */
    ParameterOp = ArgCountOp->Asl.Next->Asl.Next->Asl.Next->Asl.Next;
    if (ParameterOp->Asl.ParseOpcode == PARSEOP_DEFAULT_ARG)
    {
        ArgCount = 0;
        ParameterOp = ParameterOp->Asl.Child;

        while (ParameterOp)
        {
            ParameterOp = ParameterOp->Asl.Next;
            ArgCount++;
        }

        ArgCountOp->Asl.Value.Integer = ArgCount;
        ArgCountOp->Asl.ParseOpcode = PARSEOP_BYTECONST;
    }
    else
    {
        /*
         * Method parameters can be counted by analyzing the Parameter type
         * list. If the Parameter list contains more than 1 parameter, it
         * is nested under PARSEOP_DEFAULT_ARG. When there is only 1
         * parameter, the parse tree contains a single node representing
         * that type.
         */
        ArgCountOp->Asl.Value.Integer = 1;
        ArgCountOp->Asl.ParseOpcode = PARSEOP_BYTECONST;
    }
}
