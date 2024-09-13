/******************************************************************************
 *
 * Module Name: aczephyr.h - OS specific defines, etc.
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

#ifndef __ACZEPHYR_H__
#define __ACZEPHYR_H__

#if defined(__x86_64__)
#define ACPI_MACHINE_WIDTH          64
#else
#define ACPI_32BIT_PHYSICAL_ADDRESS
#define ACPI_MACHINE_WIDTH          32
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64
#endif

#define COMPILER_DEPENDENT_INT64    long long
#define COMPILER_DEPENDENT_UINT64   unsigned long long

#define ACPI_NO_ERROR_MESSAGES
#undef ACPI_DEBUG_OUTPUT
#define ACPI_USE_SYSTEM_CLIBRARY
#undef ACPI_DBG_TRACK_ALLOCATIONS
#define ACPI_SINGLE_THREADED
#define ACPI_USE_NATIVE_RSDP_POINTER

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>


/******************************************************************************
 *
 * FUNCTION:    AcpiEnableDbgPrint
 *
 * PARAMETERS:  Enable, 	            - Enable/Disable debug print
 *
 * RETURN:      None
 *
 * DESCRIPTION: Enable/disable debug print
 *
 *****************************************************************************/

void AcpiEnableDbgPrint (
    bool Enable);
#endif
