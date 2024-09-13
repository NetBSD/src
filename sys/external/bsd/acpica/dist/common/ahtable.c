/******************************************************************************
 *
 * Module Name: ahtable - Table of known ACPI tables with descriptions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2023, Intel Corp.
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

#include "acpi.h"
#include "accommon.h"


/* Local prototypes */

const AH_TABLE *
AcpiAhGetTableInfo (
    char                    *Signature);

extern const AH_TABLE      AcpiGbl_SupportedTables[];


/*******************************************************************************
 *
 * FUNCTION:    AcpiAhGetTableInfo
 *
 * PARAMETERS:  Signature           - ACPI signature (4 chars) to match
 *
 * RETURN:      Pointer to a valid AH_TABLE. Null if no match found.
 *
 * DESCRIPTION: Find a match in the "help" table of supported ACPI tables
 *
 ******************************************************************************/

const AH_TABLE *
AcpiAhGetTableInfo (
    char                    *Signature)
{
    const AH_TABLE      *Info;


    for (Info = AcpiGbl_SupportedTables; Info->Signature; Info++)
    {
        if (ACPI_COMPARE_NAMESEG (Signature, Info->Signature))
        {
            return (Info);
        }
    }

    return (NULL);
}


/*
 * Note: Any tables added here should be duplicated within AcpiDmTableData
 * in the file common/dmtable.c
 */
const AH_TABLE      AcpiGbl_SupportedTables[] =
{
    {ACPI_SIG_AEST, "Arm Error Source Table"},
    {ACPI_SIG_AGDI, "Arm Generic Diagnostic Dump and Reset Device Interface Table"},
    {ACPI_SIG_ASF,  "Alert Standard Format Table"},
    {ACPI_SIG_ASPT, "AMD Secure Processor Table"},
    {ACPI_SIG_BDAT, "BIOS Data ACPI Table"},
    {ACPI_SIG_BERT, "Boot Error Record Table"},
    {ACPI_SIG_BGRT, "Boot Graphics Resource Table"},
    {ACPI_SIG_BOOT, "Simple Boot Flag Table"},
    {ACPI_SIG_CCEL, "CC-Event Log Table"},
    {ACPI_SIG_CDAT, "Coherent Device Attribute Table"},
    {ACPI_SIG_CEDT, "CXL Early Discovery Table"},
    {ACPI_SIG_CPEP, "Corrected Platform Error Polling Table"},
    {ACPI_SIG_CSRT, "Core System Resource Table"},
    {ACPI_SIG_DBG2, "Debug Port Table type 2"},
    {ACPI_SIG_DBGP, "Debug Port Table"},
    {ACPI_SIG_DMAR, "DMA Remapping Table"},
    {ACPI_SIG_DRTM, "Dynamic Root of Trust for Measurement Table"},
    {ACPI_SIG_DSDT, "Differentiated System Description Table (AML table)"},
    {ACPI_SIG_ECDT, "Embedded Controller Boot Resources Table"},
    {ACPI_SIG_EINJ, "Error Injection Table"},
    {ACPI_SIG_ERST, "Error Record Serialization Table"},
    {ACPI_SIG_FACS, "Firmware ACPI Control Structure"},
    {ACPI_SIG_FADT, "Fixed ACPI Description Table (FADT)"},
    {ACPI_SIG_FPDT, "Firmware Performance Data Table"},
    {ACPI_SIG_GTDT, "Generic Timer Description Table"},
    {ACPI_SIG_HEST, "Hardware Error Source Table"},
    {ACPI_SIG_HMAT, "Heterogeneous Memory Attributes Table"},
    {ACPI_SIG_HPET, "High Precision Event Timer Table"},
    {ACPI_SIG_IORT, "IO Remapping Table"},
    {ACPI_SIG_IVRS, "I/O Virtualization Reporting Structure"},
    {ACPI_SIG_LPIT, "Low Power Idle Table"},
    {ACPI_SIG_MADT, "Multiple APIC Description Table (MADT)"},
    {ACPI_SIG_MCFG, "Memory Mapped Configuration Table"},
    {ACPI_SIG_MCHI, "Management Controller Host Interface Table"},
    {ACPI_SIG_MPAM, "Memory System Resource Partitioning and Monitoring Table"},
    {ACPI_SIG_MPST, "Memory Power State Table"},
    {ACPI_SIG_MSCT, "Maximum System Characteristics Table"},
    {ACPI_SIG_MSDM, "Microsoft Data Management Table"},
    {ACPI_SIG_NFIT, "NVDIMM Firmware Interface Table"},
    {ACPI_SIG_NHLT, "Non HD Audio Link Table"},
    {ACPI_SIG_PCCT, "Platform Communications Channel Table"},
    {ACPI_SIG_PDTT, "Platform Debug Trigger Table"},
    {ACPI_SIG_PHAT, "Platform Health Assessment Table"},
    {ACPI_SIG_PMTT, "Platform Memory Topology Table"},
    {ACPI_SIG_PPTT, "Processor Properties Topology Table"},
    {ACPI_SIG_PRMT, "Platform Runtime Mechanism Table"},
    {ACPI_SIG_RASF, "RAS Features Table"},
    {ACPI_SIG_RAS2, "RAS2 Features Table"},
    {ACPI_SIG_RHCT, "RISC-V Hart Capabilities Table"},
    {ACPI_SIG_RGRT, "Regulatory Graphics Resource Table"},
    {ACPI_RSDP_NAME,"Root System Description Pointer"},
    {ACPI_SIG_RSDT, "Root System Description Table"},
    {ACPI_SIG_S3PT, "S3 Performance Table"},
    {ACPI_SIG_SBST, "Smart Battery Specification Table"},
    {ACPI_SIG_SDEI, "Software Delegated Exception Interface Table"},
    {ACPI_SIG_SDEV, "Secure Devices Table"},
    {ACPI_SIG_SLIC, "Software Licensing Description Table"},
    {ACPI_SIG_SLIT, "System Locality Information Table"},
    {ACPI_SIG_SPCR, "Serial Port Console Redirection Table"},
    {ACPI_SIG_SPMI, "Server Platform Management Interface Table"},
    {ACPI_SIG_SRAT, "System Resource Affinity Table"},
    {ACPI_SIG_SSDT, "Secondary System Description Table (AML table)"},
    {ACPI_SIG_STAO, "Status Override Table"},
    {ACPI_SIG_SVKL, "Storage Volume Key Location Table"},
    {ACPI_SIG_TCPA, "Trusted Computing Platform Alliance Table"},
    {ACPI_SIG_TDEL, "TD-Event Log Table"},
    {ACPI_SIG_TPM2, "Trusted Platform Module hardware interface Table"},
    {ACPI_SIG_UEFI, "UEFI Boot Optimization Table"},
    {ACPI_SIG_VIOT, "Virtual I/O Translation Table"},
    {ACPI_SIG_WAET, "Windows ACPI Emulated Devices Table"},
    {ACPI_SIG_WDAT, "Watchdog Action Table"},
    {ACPI_SIG_WDDT, "Watchdog Description Table"},
    {ACPI_SIG_WDRT, "Watchdog Resource Table"},
    {ACPI_SIG_WPBT, "Windows Platform Binary Table"},
    {ACPI_SIG_WSMT, "Windows SMM Security Mitigations Table"},
    {ACPI_SIG_XENV, "Xen Environment Table"},
    {ACPI_SIG_XSDT, "Extended System Description Table"},
    {NULL,          NULL}
};
