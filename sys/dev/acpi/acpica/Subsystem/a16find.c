/******************************************************************************
 *
 * Module Name: a16find - 16-bit (real mode) routines to find ACPI
 *                        tables in memory
 *              $Revision: 23 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2003, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acpi.h"
#include "amlcode.h"
#include "acparser.h"
#include "actables.h"

#include "16bit.h"


#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("a16find")

/*
 * This module finds ACPI tables located in memory.
 * It is only generated for the 16-bit (real-mode) version of the utility.
 */

#define ACPI_TABLE_FILE_SUFFIX      ".dat"


#if ACPI_MACHINE_WIDTH == 16


char                        FilenameBuf[20];
ACPI_TABLE_HEADER           AcpiTblHeader;
UINT32                      TableSize;

UINT32                      AcpiGbl_SystemFlags;
UINT32                      AcpiGbl_RsdpOriginalLocation;

#if 0
/******************************************************************************
 *
 * FUNCTION:    AfWriteBuffer
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Open a file and write out a single buffer
 *
 ******************************************************************************/

ACPI_NATIVE_INT
AfWriteBuffer (
    char                *Filename,
    char                *Buffer,
    UINT32              Length)
{
    FILE                *fp;
    ACPI_NATIVE_INT     Actual;


    fp = fopen (Filename, "wb");
    if (!fp)
    {
        printf ("Couldn't open %s\n", Filename);
        return -1;
    }

    Actual = fwrite (Buffer, (size_t) Length, 1, fp);
    fclose (fp);
    return Actual;
}


/******************************************************************************
 *
 * FUNCTION:    AfGenerateFilename
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Build an output filename from an ACPI table ID string
 *
 ******************************************************************************/

char *
AfGenerateFilename (char *TableId)
{
    ACPI_NATIVE_UINT         i;


    for (i = 0; i < 8 && TableId[i] != ' ' && TableId[i] != 0; i++)
    {
        FilenameBuf [i] = TableId[i];
    }

    FilenameBuf [i] = 0;
    strcat (FilenameBuf, ACPI_TABLE_FILE_SUFFIX);
    return FilenameBuf;
}


/******************************************************************************
 *
 * FUNCTION:    AfDumpTables
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Dump the loaded tables to a file (or files)
 *
 ******************************************************************************/

void
AfDumpTables (void)
{
    char                    *Filename;


    if (!AcpiGbl_DSDT)
    {
        AcpiOsPrintf ("No DSDT!\n");
        return;
    }


    Filename = AfGenerateFilename (AcpiGbl_DSDT->OemTableId);
        AfWriteBuffer (Filename,
            (char *) AcpiGbl_DSDT, AcpiGbl_DSDT->Length);

    AcpiOsPrintf ("DSDT AML written to \"%s\"\n", Filename);
}

#endif

/******************************************************************************
 *
 * FUNCTION:   CopyExtendedToReal
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Copy memory above 1meg
 *
 ******************************************************************************/

int
CopyExtendedToReal (
    void        *Destination,
    UINT32      PhysicalSource,
    UINT32      Size)
{
    int         RetVal;


    RetVal = FlatMove (GET_PHYSICAL_ADDRESS (Destination), PhysicalSource, (UINT16) Size);
    return (RetVal);
}


/******************************************************************************
 *
 * FUNCTION:    AfFindRsdp
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION: Scan memory to find the RSDP
 *
 ******************************************************************************/

BOOLEAN
AfFindRsdp (RSDP_DESCRIPTOR **RSDP)
{
    PTR_OVL             Rove;


    PTR_OVL_BUILD_PTR (Rove, 0, 0);


    /* Scan low memory */

    do
    {
        if (strncmp (Rove.ptr, RSDP_SIG, (size_t) 8) == 0)
        {

            /* TBD: Checksum check is invalid for X descriptor */

/*            if (AcpiTbChecksum (Rove.ptr, sizeof(RSDP_DESCRIPTOR)) != 0)
            {
 */
            AcpiOsPrintf ("RSDP found at %p (Lo block)\n", Rove.ptr);
            *RSDP = Rove.ptr;
            return (TRUE);
        }

        Rove.ovl.base++;
    }
    while (Rove.ovl.base < 0x3F);

    /* Scan high memory */

    PTR_OVL_BUILD_PTR (Rove, 0xE000, 0);
    do
    {
        if (strncmp (Rove.ptr, RSDP_SIG, (size_t) 8) == 0)
        {
             AcpiOsPrintf ("RSDP found at %p (Hi block)\n", Rove.ptr);
            *RSDP = Rove.ptr;
            return (TRUE);
        }

        Rove.ovl.base++;
    }
    while (Rove.ovl.base < 0xFFFF);

    return (FALSE);
}


/******************************************************************************
 *
 * FUNCTION:    AfRecognizeTable
 *
 * PARAMETERS:  TablePtr            - Input buffer pointer, optional
 *              TableInfo           - Return value from AcpiTbGetTable
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check a table signature for a match against known table types
 *
 ******************************************************************************/

ACPI_STATUS
AfRecognizeTable (
    char                    *TablePtr,
    UINT32                  PhysAddr,
    ACPI_TABLE_DESC         *TableInfo)
{
    ACPI_TABLE_HEADER       *TableHeader;
    ACPI_STATUS             Status;
    ACPI_TABLE_TYPE         TableType = 0;
    void                    **TableGlobalPtr;
    UINT32                  i;


    ACPI_FUNCTION_TRACE ("AfRecognizeTable");


    /* Ensure that we have a valid table pointer */

    TableHeader = (ACPI_TABLE_HEADER *) TableInfo->Pointer;
    if (!TableHeader)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Search for a signature match among the known table types */

    Status = AE_SUPPORT;
    for (i = 1; i < NUM_ACPI_TABLES; i++)       /* Start at one -> Skip RSDP */
    {
        if (!ACPI_STRNCMP (TableHeader->Signature,
                    AcpiGbl_AcpiTableData[i].Signature,
                    AcpiGbl_AcpiTableData[i].SigLength))
        {
            AcpiOsPrintf ("Found table [%s] Length 0x%X\n",
                AcpiGbl_AcpiTableData[i].Signature, (UINT32) TableHeader->Length);

            TableType       = i;
            TableGlobalPtr  = AcpiGbl_AcpiTableData[i].GlobalPtr;
            Status          = AE_OK;
            break;
        }
    }

    /* Return the table type via the info struct */

    TableInfo->Type     = (UINT8) TableType;
    TableInfo->Length   = (ACPI_SIZE) TableHeader->Length;

    /*
     * An AE_SUPPORT means that the table was not recognized.
     * We ignore this table;  just print a debug message
     */
    if (Status == AE_SUPPORT)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
            "Unsupported table %s (Type %d) was found and discarded\n",
            AcpiGbl_AcpiTableData[TableType].Name, (UINT32) TableType));

        return_ACPI_STATUS (Status);
    }

   Status = AcpiTbValidateTableHeader (TableHeader);
    if (ACPI_FAILURE (Status))
    {
        /* Table failed verification, map all errors to BAD_DATA */

        ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Invalid table header found in table named %s (Type %d)\n",
                            AcpiGbl_AcpiTableData[TableType].Name, (UINT32) TableType));

        return_ACPI_STATUS (AE_BAD_DATA);
    }

    /* Now get the rest of the table */

    *TableGlobalPtr = ACPI_MEM_ALLOCATE (AcpiTblHeader.Length);
    if (!*TableGlobalPtr)
    {
        AcpiOsPrintf ("Could not allocate buffer for Acpi table of length 0x%X\n",
                        (UINT32) ((FACS_DESCRIPTOR *) &AcpiTblHeader)->Length);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    TableInfo->Pointer      = *TableGlobalPtr;

    CopyExtendedToReal (*TableGlobalPtr, PhysAddr, AcpiTblHeader.Length);

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "%s=%p, (TableGlobalPtr=%p)\n", AcpiGbl_AcpiTableData[TableType].Signature,
                    *TableGlobalPtr, TableGlobalPtr));

    if (AcpiGbl_DbOpt_verbose)
    {
        AcpiUtDumpBuffer (*TableGlobalPtr, 48, 0, 0);
    }

    /*
     * Validate checksum for _most_ tables,
     * even the ones whose signature we don't recognize
     */
    if (TableType != ACPI_TABLE_FACS)
    {
        /* But don't abort if the checksum is wrong */
        /* TBD: make this a configuration option? */

        AcpiTbVerifyTableChecksum (*TableGlobalPtr);
    }

    return_ACPI_STATUS (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AfGetAllTables
 *
 * PARAMETERS:  Root            - Root of the parse tree
 *
 * RETURN:      None
 *
 * DESCRIPTION:
 *
 *****************************************************************************/

ACPI_STATUS
AfGetAllTables (
    UINT32                  NumberOfTables,
    char                    *TablePtr)
{
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Index;
    ACPI_TABLE_DESC         TableInfo;


    ACPI_FUNCTION_TRACE ("AfGetAllTables");


    if (AcpiGbl_DbOpt_verbose)
    {
        AcpiOsPrintf ("Number of tables: %d\n", (UINT32) NumberOfTables);
    }

    /*
     * Loop through all table pointers found in RSDT.
     * This will NOT include the FACS and DSDT - we must get
     * them after the loop
     */
    for (Index = 0; Index < NumberOfTables; Index++)
    {
        /* Get the table via the RSDT */

        CopyExtendedToReal (&AcpiTblHeader, ACPI_GET_ADDRESS (AcpiGbl_XSDT->TableOffsetEntry[Index]),
                            sizeof (ACPI_TABLE_HEADER));

        TableInfo.Pointer       = &AcpiTblHeader;
        TableInfo.Length        = (ACPI_SIZE) AcpiTblHeader.Length;
        TableInfo.Allocation    = ACPI_MEM_ALLOCATED;

        ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Table pointer: %X\n",
            (UINT32) ACPI_GET_ADDRESS (AcpiGbl_XSDT->TableOffsetEntry[Index])));

        Status = AfRecognizeTable (NULL, ACPI_GET_ADDRESS (AcpiGbl_XSDT->TableOffsetEntry[Index]), &TableInfo);
        if (ACPI_SUCCESS (Status))
        {
            AcpiTbInitTableDescriptor (TableInfo.Type, &TableInfo);
        }

       /* Ignore errors, just move on to next table */
    }

    if (!AcpiGbl_FADT)
    {
        AcpiOsPrintf ("FADT was not found, cannot obtain FACS and DSDT!\n");
        return (AE_NO_ACPI_TABLES);
    }

    Status = AcpiTbConvertTableFadt ();
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Get the FACS
     */
    CopyExtendedToReal (&AcpiTblHeader, ACPI_GET_ADDRESS (AcpiGbl_FADT->XFirmwareCtrl),
                        sizeof (ACPI_TABLE_HEADER));
    AcpiGbl_FACS = ACPI_MEM_ALLOCATE (AcpiTblHeader.Length);
    if (!AcpiGbl_FACS)
    {
        AcpiOsPrintf ("Could not allocate buffer for FADT length 0x%X\n",
                        (UINT32) AcpiTblHeader.Length);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    CopyExtendedToReal (AcpiGbl_FACS, ACPI_GET_ADDRESS (AcpiGbl_FADT->XFirmwareCtrl), AcpiTblHeader.Length);

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "FACS at %p (Phys %8.8X) length %X FADT at%p\n",
                    AcpiGbl_FACS, (UINT32) ACPI_GET_ADDRESS (AcpiGbl_FADT->XFirmwareCtrl),
                    (UINT32) AcpiTblHeader.Length, AcpiGbl_FADT));
    if (AcpiGbl_DbOpt_verbose)
    {
        AcpiUtDumpBuffer ((char *) AcpiGbl_FADT, sizeof (ACPI_TABLE_HEADER), 0, 0);
    }

    TableInfo.Type          = ACPI_TABLE_FADT;
    TableInfo.Pointer       = (void *) AcpiGbl_FADT;
    TableInfo.Length        = (ACPI_SIZE) AcpiTblHeader.Length;
    TableInfo.Allocation    = ACPI_MEM_ALLOCATED;

    /* There is no checksum for the FACS, nothing to verify */

    AcpiTbInitTableDescriptor (TableInfo.Type, &TableInfo);

    AcpiTbBuildCommonFacs (&TableInfo);

    /*
     * Get the DSDT
     */
    CopyExtendedToReal (&AcpiTblHeader, ACPI_GET_ADDRESS (AcpiGbl_FADT->XDsdt), sizeof (ACPI_TABLE_HEADER));
    AcpiGbl_DSDT = ACPI_MEM_ALLOCATE (AcpiTblHeader.Length);
    if (!AcpiGbl_DSDT)
    {
        AcpiOsPrintf ("Could not allocate buffer for DSDT length 0x%X\n", (UINT32) AcpiTblHeader.Length);
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    CopyExtendedToReal (AcpiGbl_DSDT, ACPI_GET_ADDRESS (AcpiGbl_FADT->XDsdt), AcpiTblHeader.Length);

    AcpiOsPrintf ("DSDT at %p (Phys %8.8X) length %X FADT at %p\n",
                    AcpiGbl_DSDT, (UINT32) ACPI_GET_ADDRESS (AcpiGbl_FADT->XDsdt),
                    (UINT32) AcpiTblHeader.Length, AcpiGbl_FADT);
    if (AcpiGbl_DbOpt_verbose)
    {
        AcpiUtDumpBuffer ((char *) AcpiGbl_DSDT, sizeof (ACPI_TABLE_HEADER), 0, 0);
    }

    TableInfo.Type          = ACPI_TABLE_DSDT;
    TableInfo.Pointer       = (void *) AcpiGbl_DSDT;
    TableInfo.Length        = (ACPI_SIZE) AcpiTblHeader.Length;
    TableInfo.Allocation    = ACPI_MEM_ALLOCATED;

    AcpiTbInitTableDescriptor (TableInfo.Type, &TableInfo);
    AcpiTbVerifyTableChecksum ((ACPI_TABLE_HEADER *) AcpiGbl_DSDT);

    return_ACPI_STATUS (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AfFindDsdt
 *
 * PARAMETERS:
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load the DSDT from the file pointer
 *
 *****************************************************************************/

ACPI_STATUS
AfFindTable(
    char                    *TableName,
    UINT8                   **TablePtr,
    UINT32                  *TableLength)
{
    ACPI_TABLE_DESC         TableInfo;
    BOOLEAN                 Found;
    ACPI_STATUS             Status;
    UINT32                  PhysicalAddress;
    UINT32                  SignatureLength;
    char                    *TableSignature;


    ACPI_FUNCTION_TRACE ("AfFindTable");


    if (!AcpiGbl_RSDP)
    {
        Found = AfFindRsdp (&AcpiGbl_RSDP);
        if (!Found)
        {
            AcpiOsPrintf ("Could not find RSDP in the low megabyte\n");
            return (AE_NO_ACPI_TABLES);
        }

        /*
         * For RSDP revision 0 or 1, we use the RSDT.
         * For RSDP revision 2 (and above), we use the XSDT
         */
        if (AcpiGbl_RSDP->Revision < 2)
        {
            PhysicalAddress = AcpiGbl_RSDP->RsdtPhysicalAddress;
            TableSignature = RSDT_SIG;
            SignatureLength = sizeof (RSDT_SIG) -1;

            ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Found ACPI 1.0 RSDP\n"));
        }
        else
        {
            PhysicalAddress = ACPI_GET_ADDRESS (AcpiGbl_RSDP->XsdtPhysicalAddress);
            TableSignature = XSDT_SIG;
            SignatureLength = sizeof (XSDT_SIG) -1;

            ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "Found ACPI 2.0 RSDP\n"));
        }

        if (AcpiGbl_DbOpt_verbose)
        {
            AcpiUtDumpBuffer ((char *) AcpiGbl_RSDP, sizeof (RSDP_DESCRIPTOR), 0, ACPI_UINT32_MAX);
        }

        /* Get the RSDT/XSDT header to determine the table length */

        CopyExtendedToReal (&AcpiTblHeader, PhysicalAddress, sizeof (ACPI_TABLE_HEADER));

        ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "RSDT/XSDT at %8.8X\n", (UINT32) PhysicalAddress));
        if (AcpiGbl_DbOpt_verbose)
        {
            AcpiUtDumpBuffer ((char *) &AcpiTblHeader, sizeof (ACPI_TABLE_HEADER), 0, ACPI_UINT32_MAX);
        }

        /* Validate the table header */

        Status = AcpiTbValidateTableHeader (&AcpiTblHeader);
        if (ACPI_FAILURE (Status))
        {
            /* Table failed verification, map all errors to BAD_DATA */

            ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Invalid RSDT table header\n"));
            return (AE_BAD_DATA);
        }

        /* Allocate a buffer for the entire table */

        AcpiGbl_XSDT = (void *) malloc ((size_t) AcpiTblHeader.Length);
        if (!AcpiGbl_XSDT)
        {
            AcpiOsPrintf ("Could not allocate buffer for RSDT length 0x%X\n",
                (UINT32) AcpiTblHeader.Length);
            return AE_NO_MEMORY;
        }

        /* Get the entire RSDT/XSDT */

        CopyExtendedToReal (AcpiGbl_XSDT, PhysicalAddress, AcpiTblHeader.Length);
        AcpiOsPrintf ("%s at %p (Phys %8.8X)\n",
            TableSignature, AcpiGbl_XSDT, (UINT32) PhysicalAddress);

        if (AcpiGbl_DbOpt_verbose)
        {
            AcpiUtDumpBuffer ((char *) &AcpiTblHeader, sizeof (ACPI_TABLE_HEADER), 0, 0);
        }

        /* Convert to common format XSDT */

        TableInfo.Pointer       = (ACPI_TABLE_HEADER *) AcpiGbl_XSDT;
        TableInfo.Length        = (ACPI_SIZE) AcpiTblHeader.Length;
        TableInfo.Allocation    = ACPI_MEM_ALLOCATED;

        AcpiGbl_RsdtTableCount = AcpiTbGetTableCount (AcpiGbl_RSDP, TableInfo.Pointer);

        Status = AcpiTbConvertToXsdt (&TableInfo);
        if (ACPI_FAILURE (Status))
        {
            goto ErrorExit;
        }

        AcpiGbl_XSDT = (XSDT_DESCRIPTOR *) TableInfo.Pointer;

        /* Get the rest of the required tables (DSDT, FADT) */

        Status = AfGetAllTables (AcpiGbl_RsdtTableCount, NULL);
        if (ACPI_FAILURE (Status))
        {
            goto ErrorExit;
        }

        ACPI_DEBUG_PRINT ((ACPI_DB_INIT, "ACPI Tables successfully acquired\n"));
    }


    if (!ACPI_STRNCMP (TableName, DSDT_SIG, ACPI_NAME_SIZE))
    {
        *TablePtr = (UINT8 *) AcpiGbl_DSDT;
        *TableLength = AcpiGbl_DSDT->Length;
    }
    else if (!ACPI_STRNCMP (TableName, FADT_SIG, ACPI_NAME_SIZE))
    {
        *TablePtr = (UINT8 *) AcpiGbl_FADT;
        *TableLength = AcpiGbl_FADT->Header.Length;
    }
    else if (!ACPI_STRNCMP (TableName, FACS_SIG, ACPI_NAME_SIZE))
    {
        *TablePtr = (UINT8 *) AcpiGbl_FACS;
        *TableLength = AcpiGbl_FACS->Length;
    }
    else if (!ACPI_STRNCMP (TableName, RSDT_SIG, ACPI_NAME_SIZE))
    {
        *TablePtr = (UINT8 *) AcpiGbl_XSDT;
        *TableLength = AcpiGbl_XSDT->Header.Length;
    }
    else
    {
        AcpiOsPrintf ("Unsupported table signature: [%4.4s]\n", TableName);
        *TablePtr = NULL;
        return AE_SUPPORT;
    }

    return AE_OK;

ErrorExit:

    ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Failure during ACPI Table initialization: %s\n",
            AcpiFormatException (Status)));
    return (Status);
}


#endif /* IA16 */
