/******************************************************************************
 *
 * Module Name: oszephyr - Zephyr OSL
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
#include "acapps.h"
#include "aslcompiler.h"
#include <zephyr/arch/x86/x86_acpi_osal.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/dt-bindings/interrupt-controller/intel-ioapic.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(acpica, LOG_LEVEL_ERR);

typedef void (*zephyr_irq_t)(const void *);

/* Global variables use from acpica lib. */
BOOLEAN                     AslGbl_DoTemplates = FALSE;
BOOLEAN                     AslGbl_VerboseTemplates = FALSE;

char                        AslGbl_MsgBuffer[ASL_MSG_BUFFER_SIZE];
static BOOLEAN              EnDbgPrint;
static ACPI_PHYSICAL_ADDRESS RsdpPhyAdd;

/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadable
 *
 * PARAMETERS:  Pointer             - Area to be verified
 *              Length              - Size of area
 *
 * RETURN:      TRUE if readable for entire Length
 *
 * DESCRIPTION: Verify that a pointer is valid for reading
 *
 *****************************************************************************/

BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    return (TRUE);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiEnableDbgPrint
 *
 * PARAMETERS:  en, 	            - Enable/Disable debug print
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output
 *
 *****************************************************************************/

void
AcpiEnableDbgPrint (
    bool                    Enable)
{
    if (Enable)
    {
        EnDbgPrint = TRUE;
    }
    else
    {
        EnDbgPrint = FALSE;
    }
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPrintf
 *
 * PARAMETERS:  Fmt, ...            - Standard printf format
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output
 *
 *****************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Fmt,
    ...)
{
    va_list                 args;

    va_start (args, Fmt);

    if (EnDbgPrint)
    {
        vprintk (Fmt, args);
    }

    va_end (args);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetLine
 *
 * PARAMETERS:  Buffer              - Where to return the command line
 *              BufferLength        - Maximum Length of Buffer
 *              BytesRead           - Where the actual byte count is returned
 *
 * RETURN:      Status and actual bytes read
 *
 * DESCRIPTION: Formatted input with argument list pointer
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsGetLine (
    char                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  *BytesRead)
{
    return (-1);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocate
 *
 * PARAMETERS:  Size                - Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation. Null on error.
 *
 * DESCRIPTION: Allocate memory. Algorithm is dependent on the OS.
 *
 *****************************************************************************/

void *
AcpiOsAllocate (
    ACPI_SIZE               Size)
{
    return (k_malloc (Size));
}


#ifdef USE_NATIVE_ALLOCATE_ZEROED
/******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocateZeroed
 *
 * PARAMETERS:  Size                - Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation. Null on error.
 *
 * DESCRIPTION: Allocate and zero memory. Algorithm is dependent on the OS.
 *
 *****************************************************************************/

void *
AcpiOsAllocateZeroed (
    ACPI_SIZE               Size)
{
    void *mem;

    mem = AcpiOsAllocate (Size);

    if (mem)
    {
        memset (mem, 0, Size);
    }

    return (mem);
}
#endif


/******************************************************************************
 *
 * FUNCTION:    AcpiOsFree
 *
 * PARAMETERS:  Mem                 - Pointer to previously allocated memory
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free memory allocated via AcpiOsAllocate
 *
 *****************************************************************************/

void
AcpiOsFree (
    void                    *Mem)
{
    k_free (Mem);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadMemory
 *
 * PARAMETERS:  Address             - Physical Memory Address to read
 *              Value               - Where Value is placed
 *              Width               - Number of bits (8,16,32, or 64)
 *
 * RETURN:      Value read from physical memory Address. Always returned
 *              as a 64-bit integer, regardless of the read Width.
 *
 * DESCRIPTION: Read data from a physical memory Address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{
    switch (Width)
    {
    case 8:

        *((UINT8 *) Value) = sys_read8 (Address);
        break;

    case 16:

        *((UINT16 *) Value) = sys_read16 (Address);
        break;

    case 32:

        *((UINT32 *) Value) = sys_read32 (Address);
        break;
#if defined(__x86_64__)
    case 64:

        *((UINT64 *) Value) = sys_read64 (Address);
        break;
#endif
    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWriteMemory
 *
 * PARAMETERS:  Address             - Physical Memory Address to write
 *              Value               - Value to write
 *              Width               - Number of bits (8,16,32, or 64)
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to a physical memory Address
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
    switch (Width)
    {
    case 8:

        sys_write8 ((UINT8) Value, Address);
        break;

    case 16:

        sys_write16 ((UINT16) Value, Address);
        break;

    case 32:

        sys_write32 ((UINT32) Value, Address);
        break;
#if defined(__x86_64__)
    case 64:

        sys_write64 ((UINT64) Value, Address);
        break;
#endif
    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPort
 *
 * PARAMETERS:  Address             - Address of I/O port/register to read
 *              Value               - Where Value is placed
 *              Width               - Number of bits
 *
 * RETURN:      Value read from port
 *
 * DESCRIPTION: Read data from an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{

    switch (Width)
    {
    case 8:

        *((UINT8 *) Value) = sys_in8 (Address);
        break;

    case 16:

        *((UINT16 *) Value) = sys_in16 (Address);
        break;

    case 32:

        *((UINT32 *) Value) = sys_in32 (Address);
        break;

    case 64:

        *((UINT32 *) Value) = sys_in32 (Address);
        *((UINT32 *) Value + 4) = sys_in32 (Address + 4);
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePort
 *
 * PARAMETERS:  Address             - Address of I/O port/register to write
 *              Value               - Value to write
 *              Width               - Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to an I/O port or register
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{

    switch (Width)
    {
    case 8:

        sys_out8 ((UINT8) Value, Address);
        break;

    case 16:

        sys_out16 ((UINT16) Value, Address);
        break;

    case 32:

        sys_out32 ((UINT32) Value, Address);
        break;

    case 64:

        sys_out32 ((UINT32) Value, Address);
        sys_out32 ((UINT32) (Value + 4), (Address + 4));
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePciConfiguration
 *
 * PARAMETERS:  PciId               - Seg/Bus/Dev
 *              Register            - Device Register
 *              Value               - Value to be written
 *              Width               - Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write data to PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    UINT64                  Value,
    UINT32                  Width)
{
    UINT32                  value32;
    pcie_bdf_t              bdf = PCIE_BDF (PciId->Bus, PciId->Device, PciId->Function);


    switch (Width)
    {
    case 8:

        value32 = pcie_conf_read (bdf, Register);
        value32 = (value32 & 0xffffff00) | (UINT8) Value;
        pcie_conf_write (bdf, Register, value32);
        break;

    case 16:

        value32 = pcie_conf_read (bdf, Register);
        value32 = (value32 & 0xffff0000) | (UINT16) Value;
        pcie_conf_write (bdf, Register, value32);
        break;

    case 32:

        pcie_conf_write (bdf, Register, (UINT32) Value);
        break;

    case 64:

        pcie_conf_write (bdf, Register, (UINT32) Value);
        pcie_conf_write (bdf, (Register + 4), (UINT32) (Value >> 32));
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPciConfiguration
 *
 * PARAMETERS:  PciId               - Seg/Bus/Dev
 *              Register            - Device Register
 *              Value               - Buffer Where Value is placed
 *              Width               - Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read data from PCI configuration space
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Register,
    UINT64                  *Value,
    UINT32                  Width)
{

    pcie_bdf_t bdf = PCIE_BDF (PciId->Bus, PciId->Device, PciId->Function);

    switch (Width)
    {
    case 8:

        *((UINT8 *) Value) = (UINT8) pcie_conf_read (bdf, Register);
        break;

    case 16:

        *((UINT16 *) Value) = (UINT16) pcie_conf_read (bdf, Register);
        break;

    case 32:

        *((UINT32 *) Value) = (UINT32) pcie_conf_read (bdf, Register);
        break;

    case 64:

        *((UINT32 *) Value) = (UINT32) pcie_conf_read (bdf, Register);
        *((UINT32 *) Value + 1) = (UINT32) pcie_conf_read (bdf, (Register + 4));
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRedirectOutput
 *
 * PARAMETERS:  Destination         - An open file handle/pointer
 *
 * RETURN:      None
 *
 * DESCRIPTION: Causes redirect of AcpiOsPrintf and AcpiOsVprintf
 *
 *****************************************************************************/

void
AcpiOsRedirectOutput (
    void                    *Destination)
{

}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPredefinedOverride
 *
 * PARAMETERS:  InitVal             - Initial Value of the predefined object
 *              NewVal              - The new Value for the object
 *
 * RETURN:      Status, pointer to Value. Null pointer returned if not
 *              overriding.
 *
 * DESCRIPTION: Allow the OS to override predefined names
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{

    if (!InitVal || !NewVal)
    {
        return (AE_BAD_PARAMETER);
    }

    *NewVal = NULL;

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsTableOverride
 *
 * PARAMETERS:  ExistingTable       - Header of current table (probably firmware)
 *              NewTable            - Where an entire new table is returned.
 *
 * RETURN:      Status, pointer to new table. Null pointer returned if no
 *              table is available to override
 *
 * DESCRIPTION: Return a different version of a table if one is available
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{

    if (!ExistingTable || !NewTable)
    {
        return (AE_BAD_PARAMETER);
    }

    *NewTable = NULL;

    return (AE_NO_ACPI_TABLES);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetRootPointer
 *
 * PARAMETERS:  None
 *
 * RETURN:      RSDP physical Address
 *
 * DESCRIPTION: Gets the root pointer (RSDP)
 *
 *****************************************************************************/

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
	 LOG_DBG ("");

	if(RsdpPhyAdd)
	{
		return RsdpPhyAdd;
	}

	RsdpPhyAdd = (ACPI_PHYSICAL_ADDRESS)acpi_rsdp_get();

	return RsdpPhyAdd;
}

#ifndef ACPI_USE_NATIVE_MEMORY_MAPPING
/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  Where               - Physical Address of memory to be mapped
 *              Length              - How much memory to map
 *
 * RETURN:      Pointer to mapped memory. Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's Address space
 *
 *****************************************************************************/

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
    uint8_t                 *VirtlAdd;

    LOG_DBG ("");
    z_phys_map (&VirtlAdd, Where, Length, K_MEM_PERM_RW);
    return ((void *) VirtlAdd);
}
#endif


/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  Where               - Logical Address of memory to be unmapped
 *              Length              - How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping. Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/

void
AcpiOsUnmapMemory (
    void                    *Where,
    ACPI_SIZE               Length)
{
    LOG_DBG ("");
    z_phys_unmap (Where, Length);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPhysicalTableOverride
 *
 * PARAMETERS:  ExistingTable       - Header of current table (probably firmware)
 *              NewAddress          - Where new table Address is returned
 *                                    (Physical Address)
 *              NewTableLength      - Where new table Length is returned
 *
 * RETURN:      Status, Address/Length of new table. Null pointer returned
 *              if no table is available to override.
 *
 * DESCRIPTION: Returns AE_SUPPORT, function not used in user space.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{

    LOG_DBG ("");
    return (AE_SUPPORT);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsInitialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Init this OSL
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsInitialize (
    void)
{
    LOG_DBG ("");
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsStall
 *
 * PARAMETERS:  Microseconds        - Time to stall
 *
 * RETURN:      None. Blocks until stall is completed.
 *
 * DESCRIPTION: Sleep at microsecond granularity
 *
 *****************************************************************************/

void
AcpiOsStall (
    UINT32                  Microseconds)
{
    k_busy_wait (Microseconds);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSleep
 *
 * PARAMETERS:  Milliseconds        - Time to sleep
 *
 * RETURN:      None. Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at millisecond granularity
 *
 *****************************************************************************/

void
AcpiOsSleep (
    UINT64                  Milliseconds)
{
    k_msleep ((UINT32) Milliseconds);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsEnterSleep
 *
 * PARAMETERS:  SleepState          - Which sleep state to enter
 *              RegaValue           - Register A Value
 *              RegbValue           - Register B Value
 *
 * RETURN:      Status
 *
 * DESCRIPTION: A hook before writing sleep registers to enter the sleep
 *              state. Return AE_CTRL_SKIP to skip further sleep register
 *              writes.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsEnterSleep (
    UINT8                   SleepState,
    UINT32                  RegaValue,
    UINT32                  RegbValue)
{
    __ASSERT (FALSE, "function Not implemented");
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTimer
 *
 * PARAMETERS:  None
 *
 * RETURN:      Current ticks in 100-nanosecond units
 *
 * DESCRIPTION: Get the Value of a system timer
 *
 ******************************************************************************/

UINT64
AcpiOsGetTimer (
    void)
{
    return acpi_timer_get();
}

/******************************************************************************
 *
 * FUNCTION:    AcpiOsInstallInterruptHandler
 *
 * PARAMETERS:  InterruptNumber     - Level handler should respond to.
 *              ServiceRoutine      - Address of the ACPI interrupt handler
 *              Context             - User context
 *
 * RETURN:      Handle to the newly installed handler.
 *
 * DESCRIPTION: Install an interrupt handler. Used to install the ACPI
 *              OS-independent handler.
 *
 *****************************************************************************/

UINT32
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
    LOG_DBG ("");
    irq_connect_dynamic (InterruptNumber, 3, (zephyr_irq_t) ServiceRoutine, Context,
        IRQ_TYPE_LOWEST_LEVEL_LOW);
    irq_enable (InterruptNumber);
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRemoveInterruptHandler
 *
 * PARAMETERS:  Handle              - Returned when handler was installed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Uninstalls an interrupt handler.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{

    LOG_DBG ("");
    irq_disable (InterruptNumber);
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignal
 *
 * PARAMETERS:  Function            - ACPICA signal function code
 *              Info                - Pointer to function-dependent structure
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Miscellaneous functions. Example implementation only.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
    switch (Function)
    {
    case ACPI_SIGNAL_FATAL:
        LOG_DBG ("ACPI_SIGNAL_FATAL error");
        break;

    case ACPI_SIGNAL_BREAKPOINT:
        LOG_DBG ("ACPI_SIGNAL_BREAKPOINT");
        break;

    default:
        break;
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Spinlock/Semaphore interfaces
 *
 * DESCRIPTION: Map these interfaces to semaphore interfaces
 *
 *****************************************************************************/

#ifdef ACPI_SINGLE_THREADED
ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
    LOG_DBG ("");

    return (AE_OK);
}

void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
    LOG_DBG ("");
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle)
{
    LOG_DBG ("");
    return (0);
}

void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
    LOG_DBG ("");
}

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32              MaxUnits,
    UINT32              InitialUnits,
    ACPI_HANDLE         *OutHandle)
{
    *OutHandle = (ACPI_HANDLE) 1;
    return (AE_OK);
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_HANDLE         Handle)
{
    return (AE_OK);
}

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units,
    UINT16              Timeout)
{
    return (AE_OK);
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_HANDLE         Handle,
    UINT32              Units)
{
    return (AE_OK);
}

ACPI_THREAD_ID
AcpiOsGetThreadId (
    void)
{
    LOG_DBG ("");
    return (1);
}

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
    Function (Context);
    return (AE_OK);
}
#endif
