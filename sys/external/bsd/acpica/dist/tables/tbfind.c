/******************************************************************************
 *
 * Module Name: tbfind   - find table
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2011, Intel Corp.
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

#define __TBFIND_C__

#include "acpi.h"
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbfind")


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbFindTable
 *
 * PARAMETERS:  Signature           - String with ACPI table signature
 *              OemId               - String with the table OEM ID
 *              OemTableId          - String with the OEM Table ID
 *              TableIndex          - Where the table index is returned
 *
 * RETURN:      Status and table index
 *
 * DESCRIPTION: Find an ACPI table (in the RSDT/XSDT) that matches the
 *              Signature, OEM ID and OEM Table ID. Returns an index that can
 *              be used to get the table header or entire table.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbFindTable (
    char                    *Signature,
    char                    *OemId,
    char                    *OemTableId,
    UINT32                  *TableIndex)
{
    UINT32                  i;
    ACPI_STATUS             Status;
    ACPI_TABLE_HEADER       Header;


    ACPI_FUNCTION_TRACE (TbFindTable);


    /* Normalize the input strings */

    ACPI_MEMSET (&Header, 0, sizeof (ACPI_TABLE_HEADER));
    ACPI_STRNCPY (Header.Signature, Signature, ACPI_NAME_SIZE);
    ACPI_STRNCPY (Header.OemId, OemId, ACPI_OEM_ID_SIZE);
    ACPI_STRNCPY (Header.OemTableId, OemTableId, ACPI_OEM_TABLE_ID_SIZE);

    /* Search for the table */

    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; ++i)
    {
        if (ACPI_MEMCMP (&(AcpiGbl_RootTableList.Tables[i].Signature),
                            Header.Signature, ACPI_NAME_SIZE))
        {
            /* Not the requested table */

            continue;
        }

        /* Table with matching signature has been found */

        if (!AcpiGbl_RootTableList.Tables[i].Pointer)
        {
            /* Table is not currently mapped, map it */

            Status = AcpiTbVerifyTable (&AcpiGbl_RootTableList.Tables[i]);
            if (ACPI_FAILURE (Status))
            {
                return_ACPI_STATUS (Status);
            }

            if (!AcpiGbl_RootTableList.Tables[i].Pointer)
            {
                continue;
            }
        }

        /* Check for table match on all IDs */

        if (!ACPI_MEMCMP (AcpiGbl_RootTableList.Tables[i].Pointer->Signature,
                            Header.Signature, ACPI_NAME_SIZE) &&
            (!OemId[0] ||
             !ACPI_MEMCMP (AcpiGbl_RootTableList.Tables[i].Pointer->OemId,
                             Header.OemId, ACPI_OEM_ID_SIZE)) &&
            (!OemTableId[0] ||
             !ACPI_MEMCMP (AcpiGbl_RootTableList.Tables[i].Pointer->OemTableId,
                             Header.OemTableId, ACPI_OEM_TABLE_ID_SIZE)))
        {
            *TableIndex = i;

            ACPI_DEBUG_PRINT ((ACPI_DB_TABLES, "Found table [%4.4s]\n",
                Header.Signature));
            return_ACPI_STATUS (AE_OK);
        }
    }

    return_ACPI_STATUS (AE_NOT_FOUND);
}
