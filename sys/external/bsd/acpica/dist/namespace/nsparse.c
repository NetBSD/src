/******************************************************************************
 *
 * Module Name: nsparse - namespace interface to AML parser
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2015, Intel Corp.
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

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "acparser.h"
#include "acdispat.h"
#include "actables.h"


#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsparse")


/*******************************************************************************
 *
 * FUNCTION:    NsOneCompleteParse
 *
 * PARAMETERS:  PassNumber              - 1 or 2
 *              TableDesc               - The table to be parsed.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Perform one complete parse of an ACPI/AML table.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsOneCompleteParse (
    UINT32                  PassNumber,
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *StartNode)
{
    ACPI_PARSE_OBJECT       *ParseRoot;
    ACPI_STATUS             Status;
    UINT32                  AmlLength;
    UINT8                   *AmlStart;
    ACPI_WALK_STATE         *WalkState;
    ACPI_TABLE_HEADER       *Table;
    ACPI_OWNER_ID           OwnerId;


    ACPI_FUNCTION_TRACE (NsOneCompleteParse);


    Status = AcpiTbGetOwnerId (TableIndex, &OwnerId);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Create and init a Root Node */

    ParseRoot = AcpiPsCreateScopeOp ();
    if (!ParseRoot)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Create and initialize a new walk state */

    WalkState = AcpiDsCreateWalkState (OwnerId, NULL, NULL, NULL);
    if (!WalkState)
    {
        AcpiPsFreeOp (ParseRoot);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    Status = AcpiGetTableByIndex (TableIndex, &Table);
    if (ACPI_FAILURE (Status))
    {
        AcpiDsDeleteWalkState (WalkState);
        AcpiPsFreeOp (ParseRoot);
        return_ACPI_STATUS (Status);
    }

    /* Table must consist of at least a complete header */

    if (Table->Length < sizeof (ACPI_TABLE_HEADER))
    {
        Status = AE_BAD_HEADER;
    }
    else
    {
        AmlStart = (UINT8 *) Table + sizeof (ACPI_TABLE_HEADER);
        AmlLength = Table->Length - sizeof (ACPI_TABLE_HEADER);
        Status = AcpiDsInitAmlWalk (WalkState, ParseRoot, NULL,
                    AmlStart, AmlLength, NULL, (UINT8) PassNumber);
    }

    if (ACPI_FAILURE (Status))
    {
        AcpiDsDeleteWalkState (WalkState);
        goto Cleanup;
    }

    /* StartNode is the default location to load the table  */

    if (StartNode && StartNode != AcpiGbl_RootNode)
    {
        Status = AcpiDsScopeStackPush (StartNode, ACPI_TYPE_METHOD, WalkState);
        if (ACPI_FAILURE (Status))
        {
            AcpiDsDeleteWalkState (WalkState);
            goto Cleanup;
        }
    }

    /* Parse the AML */

    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "*PARSE* pass %u parse\n", PassNumber));
    Status = AcpiPsParseAml (WalkState);

Cleanup:
    AcpiPsDeleteParseTree (ParseRoot);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsParseTable
 *
 * PARAMETERS:  TableDesc       - An ACPI table descriptor for table to parse
 *              StartNode       - Where to enter the table into the namespace
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse AML within an ACPI table and return a tree of ops
 *
 ******************************************************************************/

ACPI_STATUS
AcpiNsParseTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *StartNode)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (NsParseTable);


    /*
     * AML Parse, pass 1
     *
     * In this pass, we load most of the namespace. Control methods
     * are not parsed until later. A parse tree is not created. Instead,
     * each Parser Op subtree is deleted when it is finished. This saves
     * a great deal of memory, and allows a small cache of parse objects
     * to service the entire parse. The second pass of the parse then
     * performs another complete parse of the AML.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "**** Start pass 1\n"));
    Status = AcpiNsOneCompleteParse (ACPI_IMODE_LOAD_PASS1,
                TableIndex, StartNode);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * AML Parse, pass 2
     *
     * In this pass, we resolve forward references and other things
     * that could not be completed during the first pass.
     * Another complete parse of the AML is performed, but the
     * overhead of this is compensated for by the fact that the
     * parse objects are all cached.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "**** Start pass 2\n"));
    Status = AcpiNsOneCompleteParse (ACPI_IMODE_LOAD_PASS2,
                TableIndex, StartNode);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    return_ACPI_STATUS (Status);
}
