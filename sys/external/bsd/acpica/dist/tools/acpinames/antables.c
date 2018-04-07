/******************************************************************************
 *
 * Module Name: antables - ACPI table setup/install for AcpiNames utility
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

#include "acpinames.h"

#define _COMPONENT          ACPI_TOOLS
        ACPI_MODULE_NAME    ("antables")

/* Local prototypes */

static void
AnInitializeTableHeader (
    ACPI_TABLE_HEADER       *Header,
    char                    *Signature,
    UINT32                  Length);


/* Non-AML tables that are constructed locally and installed */

static ACPI_TABLE_RSDP          LocalRSDP;
static ACPI_TABLE_FACS          LocalFACS;

/*
 * We need a local FADT so that the hardware subcomponent will function,
 * even though the underlying OSD HW access functions don't do anything.
 */
static ACPI_TABLE_FADT          LocalFADT;

/*
 * Use XSDT so that both 32- and 64-bit versions of this utility will
 * function automatically.
 */
static ACPI_TABLE_XSDT          *LocalXSDT;

#define BASE_XSDT_TABLES        1
#define BASE_XSDT_SIZE          (sizeof (ACPI_TABLE_XSDT) + \
                                    ((BASE_XSDT_TABLES -1) * sizeof (UINT64)))


/******************************************************************************
 *
 * FUNCTION:    AnInitializeTableHeader
 *
 * PARAMETERS:  Header          - A valid standard ACPI table header
 *              Signature       - Signature to insert
 *              Length          - Length of the table
 *
 * RETURN:      None. Header is modified.
 *
 * DESCRIPTION: Initialize the table header for a local ACPI table.
 *
 *****************************************************************************/

static void
AnInitializeTableHeader (
    ACPI_TABLE_HEADER       *Header,
    char                    *Signature,
    UINT32                  Length)
{

    ACPI_MOVE_NAME (Header->Signature, Signature);
    Header->Length = Length;

    Header->OemRevision = 0x1001;
    memcpy (Header->OemId, "Intel ", ACPI_OEM_ID_SIZE);
    memcpy (Header->OemTableId, "AcpiExec", ACPI_OEM_TABLE_ID_SIZE);
    ACPI_MOVE_NAME (Header->AslCompilerId, "INTL");
    Header->AslCompilerRevision = ACPI_CA_VERSION;

    /* Set the checksum, must set to zero first */

    Header->Checksum = 0;
    Header->Checksum = (UINT8) -AcpiTbChecksum (
        (void *) Header, Header->Length);
}


/******************************************************************************
 *
 * FUNCTION:    AnBuildLocalTables
 *
 * PARAMETERS:  TableCount      - Number of tables on the command line
 *              TableList       - List of actual tables from files
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Build a complete ACPI table chain, with a local RSDP, XSDT,
 *              FADT, FACS, and the input DSDT/SSDT.
 *
 *****************************************************************************/

ACPI_STATUS
AnBuildLocalTables (
    ACPI_NEW_TABLE_DESC     *TableList)
{
    UINT32                  TableCount = 0;
    ACPI_PHYSICAL_ADDRESS   DsdtAddress = 0;
    UINT32                  XsdtSize;
    ACPI_NEW_TABLE_DESC     *NextTable;
    UINT32                  NextIndex;
    ACPI_TABLE_FADT         *ExternalFadt = NULL;


    /*
     * Update the table count. For the DSDT, it is not put into the XSDT.
     * For the FADT, this table is already accounted for since we usually
     * install a local FADT.
     */
    NextTable = TableList;
    while (NextTable)
    {
        if (!ACPI_COMPARE_NAME (NextTable->Table->Signature, ACPI_SIG_DSDT) &&
            !ACPI_COMPARE_NAME (NextTable->Table->Signature, ACPI_SIG_FADT))
        {
            TableCount++;
        }

        NextTable = NextTable->Next;
    }

    XsdtSize = BASE_XSDT_SIZE + (TableCount * sizeof (UINT64));

    /* Build an XSDT */

    LocalXSDT = AcpiOsAllocate (XsdtSize);
    if (!LocalXSDT)
    {
        return (AE_NO_MEMORY);
    }

    memset (LocalXSDT, 0, XsdtSize);
    LocalXSDT->TableOffsetEntry[0] = ACPI_PTR_TO_PHYSADDR (&LocalFADT);

    /*
     * Install the user tables. The DSDT must be installed in the FADT.
     * All other tables are installed directly into the XSDT.
     *
     * Note: The tables are loaded in reverse order from the incoming
     * input, which makes it match the command line order.
     */
    NextIndex = BASE_XSDT_TABLES;
    NextTable = TableList;
    while (NextTable)
    {
        /*
         * Incoming DSDT or FADT are special cases. All other tables are
         * just immediately installed into the XSDT.
         */
        if (ACPI_COMPARE_NAME (NextTable->Table->Signature, ACPI_SIG_DSDT))
        {
            if (DsdtAddress)
            {
                printf ("Already found a DSDT, only one allowed\n");
                return (AE_ALREADY_EXISTS);
            }

            /* The incoming user table is a DSDT */

            DsdtAddress = ACPI_PTR_TO_PHYSADDR (NextTable->Table);
        }
        else if (ACPI_COMPARE_NAME (NextTable->Table->Signature, ACPI_SIG_FADT))
        {
            ExternalFadt =
                ACPI_CAST_PTR (ACPI_TABLE_FADT, NextTable->Table);
            LocalXSDT->TableOffsetEntry[0] =
                ACPI_PTR_TO_PHYSADDR (NextTable->Table);
        }
        else
        {
            /* Install the table in the XSDT */

            LocalXSDT->TableOffsetEntry[TableCount - NextIndex + 1] =
                ACPI_PTR_TO_PHYSADDR (NextTable->Table);
            NextIndex++;
        }

        NextTable = NextTable->Next;
    }

    /* Build an RSDP. Contains a valid XSDT only, no RSDT */

    memset (&LocalRSDP, 0, sizeof (ACPI_TABLE_RSDP));
    ACPI_MAKE_RSDP_SIG (LocalRSDP.Signature);
    memcpy (LocalRSDP.OemId, "Intel", 6);

    LocalRSDP.Revision = 2;
    LocalRSDP.XsdtPhysicalAddress = ACPI_PTR_TO_PHYSADDR (LocalXSDT);
    LocalRSDP.Length = sizeof (ACPI_TABLE_XSDT);

    /* Set checksums for both XSDT and RSDP */

    AnInitializeTableHeader ((void *) LocalXSDT, ACPI_SIG_XSDT, XsdtSize);

    LocalRSDP.Checksum = 0;
    LocalRSDP.Checksum = (UINT8) -AcpiTbChecksum (
        (void *) &LocalRSDP, ACPI_RSDP_CHECKSUM_LENGTH);

    if (!DsdtAddress)
    {
        return (AE_SUPPORT);
    }

    /*
     * Build an FADT. There are two options for the FADT:
     * 1) Incoming external FADT specified on the command line
     * 2) A fully featured local FADT
     */
    memset (&LocalFADT, 0, sizeof (ACPI_TABLE_FADT));

    if (ExternalFadt)
    {
        /*
         * Use the external FADT, but we must update the DSDT/FACS
         * addresses as well as the checksum
         */
        ExternalFadt->Dsdt = (UINT32) DsdtAddress;
        ExternalFadt->Facs = ACPI_PTR_TO_PHYSADDR (&LocalFACS);

        /*
         * If there room in the FADT for the XDsdt and XFacs 64-bit
         * pointers, use them.
         */
        if (ExternalFadt->Header.Length > ACPI_PTR_DIFF (
            &ExternalFadt->XDsdt, ExternalFadt))
        {
            ExternalFadt->Dsdt = 0;
            ExternalFadt->Facs = 0;
            ExternalFadt->XDsdt = DsdtAddress;
            ExternalFadt->XFacs = ACPI_PTR_TO_PHYSADDR (&LocalFACS);
        }

        /* Complete the external FADT with the checksum */

        ExternalFadt->Header.Checksum = 0;
        ExternalFadt->Header.Checksum = (UINT8) -AcpiTbChecksum (
            (void *) ExternalFadt, ExternalFadt->Header.Length);
    }
    else
    {
        /*
         * Build a local FADT so we can test the hardware/event init
         */
        LocalFADT.Header.Revision = 5;

        /* Setup FADT header and DSDT/FACS addresses */

        LocalFADT.Dsdt = 0;
        LocalFADT.Facs = 0;

        LocalFADT.XDsdt = DsdtAddress;
        LocalFADT.XFacs = ACPI_PTR_TO_PHYSADDR (&LocalFACS);

        /* Miscellaneous FADT fields */

        LocalFADT.Gpe0BlockLength = 16;
        LocalFADT.Gpe0Block = 0x00001234;

        LocalFADT.Gpe1BlockLength = 6;
        LocalFADT.Gpe1Block = 0x00005678;
        LocalFADT.Gpe1Base = 96;

        LocalFADT.Pm1EventLength = 4;
        LocalFADT.Pm1aEventBlock = 0x00001aaa;
        LocalFADT.Pm1bEventBlock = 0x00001bbb;

        LocalFADT.Pm1ControlLength = 2;
        LocalFADT.Pm1aControlBlock = 0xB0;

        LocalFADT.PmTimerLength = 4;
        LocalFADT.PmTimerBlock = 0xA0;

        LocalFADT.Pm2ControlBlock = 0xC0;
        LocalFADT.Pm2ControlLength = 1;

        /* Setup one example X-64 field */

        LocalFADT.XPm1bEventBlock.SpaceId = ACPI_ADR_SPACE_SYSTEM_IO;
        LocalFADT.XPm1bEventBlock.Address = LocalFADT.Pm1bEventBlock;
        LocalFADT.XPm1bEventBlock.BitWidth = (UINT8)
            ACPI_MUL_8 (LocalFADT.Pm1EventLength);
    }

    AnInitializeTableHeader ((void *) &LocalFADT,
        ACPI_SIG_FADT, sizeof (ACPI_TABLE_FADT));

    /* Build a FACS */

    memset (&LocalFACS, 0, sizeof (ACPI_TABLE_FACS));
    ACPI_MOVE_NAME (LocalFACS.Signature, ACPI_SIG_FACS);

    LocalFACS.Length = sizeof (ACPI_TABLE_FACS);
    LocalFACS.GlobalLock = 0x11AA0011;
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetRootPointer
 *
 * PARAMETERS:  None
 *
 * RETURN:      Address of the RSDP
 *
 * DESCRIPTION: Return a local RSDP, used to dynamically load tables via the
 *              standard ACPI mechanism.
 *
 *****************************************************************************/

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{

    return (ACPI_PTR_TO_PHYSADDR (&LocalRSDP));
}
