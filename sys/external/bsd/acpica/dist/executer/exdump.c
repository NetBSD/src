/******************************************************************************
 *
 * Module Name: exdump - Interpreter debug output routines
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2013, Intel Corp.
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

#define __EXDUMP_C__

#include "acpi.h"
#include "accommon.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_EXECUTER
        ACPI_MODULE_NAME    ("exdump")

/*
 * The following routines are used for debug output only
 */
#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

/* Local prototypes */

static void
AcpiExOutString (
    const char              *Title,
    const char              *Value);

static void
AcpiExOutPointer (
    const char              *Title,
    void                    *Value);

static void
AcpiExDumpObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_EXDUMP_INFO        *Info);

static void
AcpiExDumpReferenceObj (
    ACPI_OPERAND_OBJECT     *ObjDesc);

static void
AcpiExDumpPackageObj (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  Level,
    UINT32                  Index);


/*******************************************************************************
 *
 * Object Descriptor info tables
 *
 * Note: The first table entry must be an INIT opcode and must contain
 * the table length (number of table entries)
 *
 ******************************************************************************/

static ACPI_EXDUMP_INFO     AcpiExDumpInteger[2] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpInteger),        NULL},
    {ACPI_EXD_UINT64,   ACPI_EXD_OFFSET (Integer.Value),                "Value"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpString[4] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpString),         NULL},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (String.Length),                "Length"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (String.Pointer),               "Pointer"},
    {ACPI_EXD_STRING,   0,                                              NULL}
};

static ACPI_EXDUMP_INFO     AcpiExDumpBuffer[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpBuffer),         NULL},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (Buffer.Length),                "Length"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Buffer.Pointer),               "Pointer"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Buffer.Node),                  "Parent Node"},
    {ACPI_EXD_BUFFER,   0,                                              NULL}
};

static ACPI_EXDUMP_INFO     AcpiExDumpPackage[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpPackage),        NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Package.Flags),                "Flags"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (Package.Count),                "Elements"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Package.Elements),             "Element List"},
    {ACPI_EXD_PACKAGE,  0,                                              NULL}
};

static ACPI_EXDUMP_INFO     AcpiExDumpDevice[4] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpDevice),         NULL},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Device.Handler),               "Handler"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Device.NotifyList[0]),         "System Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Device.NotifyList[1]),         "Device Notify"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpEvent[2] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpEvent),          NULL},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Event.OsSemaphore),            "OsSemaphore"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpMethod[9] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpMethod),         NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Method.InfoFlags),             "Info Flags"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Method.ParamCount),            "Parameter Count"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Method.SyncLevel),             "Sync Level"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Method.Mutex),                 "Mutex"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Method.OwnerId),               "Owner Id"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Method.ThreadCount),           "Thread Count"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (Method.AmlLength),             "Aml Length"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Method.AmlStart),              "Aml Start"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpMutex[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpMutex),          NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Mutex.SyncLevel),              "Sync Level"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Mutex.OwnerThread),            "Owner Thread"},
    {ACPI_EXD_UINT16,   ACPI_EXD_OFFSET (Mutex.AcquisitionDepth),       "Acquire Depth"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Mutex.OsMutex),                "OsMutex"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpRegion[7] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpRegion),         NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Region.SpaceId),               "Space Id"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Region.Flags),                 "Flags"},
    {ACPI_EXD_ADDRESS,  ACPI_EXD_OFFSET (Region.Address),               "Address"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (Region.Length),                "Length"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Region.Handler),               "Handler"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Region.Next),                  "Next"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpPower[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpPower),          NULL},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (PowerResource.SystemLevel),    "System Level"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (PowerResource.ResourceOrder),  "Resource Order"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (PowerResource.NotifyList[0]),  "System Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (PowerResource.NotifyList[1]),  "Device Notify"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpProcessor[7] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpProcessor),      NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Processor.ProcId),             "Processor ID"},
    {ACPI_EXD_UINT8 ,   ACPI_EXD_OFFSET (Processor.Length),             "Length"},
    {ACPI_EXD_ADDRESS,  ACPI_EXD_OFFSET (Processor.Address),            "Address"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Processor.NotifyList[0]),      "System Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Processor.NotifyList[1]),      "Device Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Processor.Handler),            "Handler"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpThermal[4] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpThermal),        NULL},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (ThermalZone.NotifyList[0]),    "System Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (ThermalZone.NotifyList[1]),    "Device Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (ThermalZone.Handler),          "Handler"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpBufferField[3] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpBufferField),    NULL},
    {ACPI_EXD_FIELD,    0,                                              NULL},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (BufferField.BufferObj),        "Buffer Object"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpRegionField[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpRegionField),    NULL},
    {ACPI_EXD_FIELD,    0,                                              NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Field.AccessLength),           "AccessLength"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Field.RegionObj),              "Region Object"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Field.ResourceBuffer),         "ResourceBuffer"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpBankField[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpBankField),      NULL},
    {ACPI_EXD_FIELD,    0,                                              NULL},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (BankField.Value),              "Value"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (BankField.RegionObj),          "Region Object"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (BankField.BankObj),            "Bank Object"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpIndexField[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpBankField),      NULL},
    {ACPI_EXD_FIELD,    0,                                              NULL},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (IndexField.Value),             "Value"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (IndexField.IndexObj),          "Index Object"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (IndexField.DataObj),           "Data Object"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpReference[8] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpReference),       NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Reference.Class),              "Class"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Reference.TargetType),         "Target Type"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (Reference.Value),              "Value"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Reference.Object),             "Object Desc"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Reference.Node),               "Node"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Reference.Where),              "Where"},
    {ACPI_EXD_REFERENCE,0,                                              NULL}
};

static ACPI_EXDUMP_INFO     AcpiExDumpAddressHandler[6] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpAddressHandler), NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (AddressSpace.SpaceId),         "Space Id"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (AddressSpace.Next),            "Next"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (AddressSpace.RegionList),      "Region List"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (AddressSpace.Node),            "Node"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (AddressSpace.Context),         "Context"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpNotify[7] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpNotify),         NULL},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Notify.Node),                  "Node"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (Notify.HandlerType),           "Handler Type"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Notify.Handler),               "Handler"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Notify.Context),               "Context"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Notify.Next[0]),               "Next System Notify"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (Notify.Next[1]),               "Next Device Notify"}
};


/* Miscellaneous tables */

static ACPI_EXDUMP_INFO     AcpiExDumpCommon[4] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpCommon),         NULL},
    {ACPI_EXD_TYPE ,    0,                                              NULL},
    {ACPI_EXD_UINT16,   ACPI_EXD_OFFSET (Common.ReferenceCount),        "Reference Count"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (Common.Flags),                 "Flags"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpFieldCommon[7] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpFieldCommon),    NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (CommonField.FieldFlags),       "Field Flags"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (CommonField.AccessByteWidth),  "Access Byte Width"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (CommonField.BitLength),        "Bit Length"},
    {ACPI_EXD_UINT8,    ACPI_EXD_OFFSET (CommonField.StartFieldBitOffset),"Field Bit Offset"},
    {ACPI_EXD_UINT32,   ACPI_EXD_OFFSET (CommonField.BaseByteOffset),   "Base Byte Offset"},
    {ACPI_EXD_POINTER,  ACPI_EXD_OFFSET (CommonField.Node),             "Parent Node"}
};

static ACPI_EXDUMP_INFO     AcpiExDumpNode[5] =
{
    {ACPI_EXD_INIT,     ACPI_EXD_TABLE_SIZE (AcpiExDumpNode),           NULL},
    {ACPI_EXD_UINT8,    ACPI_EXD_NSOFFSET (Flags),                      "Flags"},
    {ACPI_EXD_UINT8,    ACPI_EXD_NSOFFSET (OwnerId),                    "Owner Id"},
    {ACPI_EXD_POINTER,  ACPI_EXD_NSOFFSET (Child),                      "Child List"},
    {ACPI_EXD_POINTER,  ACPI_EXD_NSOFFSET (Peer),                       "Next Peer"}
};


/* Dispatch table, indexed by object type */

static ACPI_EXDUMP_INFO     *AcpiExDumpInfo[] =
{
    NULL,
    AcpiExDumpInteger,
    AcpiExDumpString,
    AcpiExDumpBuffer,
    AcpiExDumpPackage,
    NULL,
    AcpiExDumpDevice,
    AcpiExDumpEvent,
    AcpiExDumpMethod,
    AcpiExDumpMutex,
    AcpiExDumpRegion,
    AcpiExDumpPower,
    AcpiExDumpProcessor,
    AcpiExDumpThermal,
    AcpiExDumpBufferField,
    NULL,
    NULL,
    AcpiExDumpRegionField,
    AcpiExDumpBankField,
    AcpiExDumpIndexField,
    AcpiExDumpReference,
    NULL,
    NULL,
    AcpiExDumpNotify,
    AcpiExDumpAddressHandler,
    NULL,
    NULL,
    NULL
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpObject
 *
 * PARAMETERS:  ObjDesc             - Descriptor to dump
 *              Info                - Info table corresponding to this object
 *                                    type
 *
 * RETURN:      None
 *
 * DESCRIPTION: Walk the info table for this object
 *
 ******************************************************************************/

static void
AcpiExDumpObject (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_EXDUMP_INFO        *Info)
{
    UINT8                   *Target;
    char                    *Name;
    const char              *ReferenceName;
    UINT8                   Count;


    if (!Info)
    {
        AcpiOsPrintf (
            "ExDumpObject: Display not implemented for object type %s\n",
            AcpiUtGetObjectTypeName (ObjDesc));
        return;
    }

    /* First table entry must contain the table length (# of table entries) */

    Count = Info->Offset;

    while (Count)
    {
        Target = ACPI_ADD_PTR (UINT8, ObjDesc, Info->Offset);
        Name = __UNCONST(Info->Name);

        switch (Info->Opcode)
        {
        case ACPI_EXD_INIT:

            break;

        case ACPI_EXD_TYPE:

            AcpiExOutString  ("Type", AcpiUtGetObjectTypeName (ObjDesc));
            break;

        case ACPI_EXD_UINT8:

            AcpiOsPrintf ("%20s : %2.2X\n", Name, *Target);
            break;

        case ACPI_EXD_UINT16:

            AcpiOsPrintf ("%20s : %4.4X\n", Name, ACPI_GET16 (Target));
            break;

        case ACPI_EXD_UINT32:

            AcpiOsPrintf ("%20s : %8.8X\n", Name, ACPI_GET32 (Target));
            break;

        case ACPI_EXD_UINT64:

            AcpiOsPrintf ("%20s : %8.8X%8.8X\n", "Value",
                ACPI_FORMAT_UINT64 (ACPI_GET64 (Target)));
            break;

        case ACPI_EXD_POINTER:
        case ACPI_EXD_ADDRESS:

            AcpiExOutPointer (Name, *ACPI_CAST_PTR (void *, Target));
            break;

        case ACPI_EXD_STRING:

            AcpiUtPrintString (ObjDesc->String.Pointer, ACPI_UINT8_MAX);
            AcpiOsPrintf ("\n");
            break;

        case ACPI_EXD_BUFFER:

            ACPI_DUMP_BUFFER (ObjDesc->Buffer.Pointer, ObjDesc->Buffer.Length);
            break;

        case ACPI_EXD_PACKAGE:

            /* Dump the package contents */

            AcpiOsPrintf ("\nPackage Contents:\n");
            AcpiExDumpPackageObj (ObjDesc, 0, 0);
            break;

        case ACPI_EXD_FIELD:

            AcpiExDumpObject (ObjDesc, AcpiExDumpFieldCommon);
            break;

        case ACPI_EXD_REFERENCE:

            ReferenceName = AcpiUtGetReferenceName (ObjDesc);
            AcpiExOutString ("Class Name", ACPI_CAST_PTR (char, ReferenceName));
            AcpiExDumpReferenceObj (ObjDesc);
            break;

        default:

            AcpiOsPrintf ("**** Invalid table opcode [%X] ****\n",
                Info->Opcode);
            return;
        }

        Info++;
        Count--;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpOperand
 *
 * PARAMETERS:  *ObjDesc        - Pointer to entry to be dumped
 *              Depth           - Current nesting depth
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump an operand object
 *
 ******************************************************************************/

void
AcpiExDumpOperand (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  Depth)
{
    UINT32                  Length;
    UINT32                  Index;


    ACPI_FUNCTION_NAME (ExDumpOperand)


    /* Check if debug output enabled */

    if (!ACPI_IS_DEBUG_ENABLED (ACPI_LV_EXEC, _COMPONENT))
    {
        return;
    }

    if (!ObjDesc)
    {
        /* This could be a null element of a package */

        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Null Object Descriptor\n"));
        return;
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) == ACPI_DESC_TYPE_NAMED)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "%p Namespace Node: ", ObjDesc));
        ACPI_DUMP_ENTRY (ObjDesc, ACPI_LV_EXEC);
        return;
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) != ACPI_DESC_TYPE_OPERAND)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
            "%p is not a node or operand object: [%s]\n",
            ObjDesc, AcpiUtGetDescriptorName (ObjDesc)));
        ACPI_DUMP_BUFFER (ObjDesc, sizeof (ACPI_OPERAND_OBJECT));
        return;
    }

    /* ObjDesc is a valid object */

    if (Depth > 0)
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "%*s[%u] %p ",
            Depth, " ", Depth, ObjDesc));
    }
    else
    {
        ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "%p ", ObjDesc));
    }

    /* Decode object type */

    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_LOCAL_REFERENCE:

        AcpiOsPrintf ("Reference: [%s] ", AcpiUtGetReferenceName (ObjDesc));

        switch (ObjDesc->Reference.Class)
        {
        case ACPI_REFCLASS_DEBUG:

            AcpiOsPrintf ("\n");
            break;

        case ACPI_REFCLASS_INDEX:

            AcpiOsPrintf ("%p\n", ObjDesc->Reference.Object);
            break;

        case ACPI_REFCLASS_TABLE:

            AcpiOsPrintf ("Table Index %X\n", ObjDesc->Reference.Value);
            break;

        case ACPI_REFCLASS_REFOF:

            AcpiOsPrintf ("%p [%s]\n", ObjDesc->Reference.Object,
                AcpiUtGetTypeName (((ACPI_OPERAND_OBJECT *)
                    ObjDesc->Reference.Object)->Common.Type));
            break;

        case ACPI_REFCLASS_NAME:

            AcpiOsPrintf ("- [%4.4s]\n", ObjDesc->Reference.Node->Name.Ascii);
            break;

        case ACPI_REFCLASS_ARG:
        case ACPI_REFCLASS_LOCAL:

            AcpiOsPrintf ("%X\n", ObjDesc->Reference.Value);
            break;

        default:    /* Unknown reference class */

            AcpiOsPrintf ("%2.2X\n", ObjDesc->Reference.Class);
            break;
        }
        break;

    case ACPI_TYPE_BUFFER:

        AcpiOsPrintf ("Buffer length %.2X @ %p\n",
            ObjDesc->Buffer.Length, ObjDesc->Buffer.Pointer);

        /* Debug only -- dump the buffer contents */

        if (ObjDesc->Buffer.Pointer)
        {
            Length = ObjDesc->Buffer.Length;
            if (Length > 128)
            {
                Length = 128;
            }

            AcpiOsPrintf ("Buffer Contents: (displaying length 0x%.2X)\n",
                Length);
            ACPI_DUMP_BUFFER (ObjDesc->Buffer.Pointer, Length);
        }
        break;

    case ACPI_TYPE_INTEGER:

        AcpiOsPrintf ("Integer %8.8X%8.8X\n",
            ACPI_FORMAT_UINT64 (ObjDesc->Integer.Value));
        break;

    case ACPI_TYPE_PACKAGE:

        AcpiOsPrintf ("Package [Len %X] ElementArray %p\n",
            ObjDesc->Package.Count, ObjDesc->Package.Elements);

        /*
         * If elements exist, package element pointer is valid,
         * and debug_level exceeds 1, dump package's elements.
         */
        if (ObjDesc->Package.Count &&
            ObjDesc->Package.Elements &&
            AcpiDbgLevel > 1)
        {
            for (Index = 0; Index < ObjDesc->Package.Count; Index++)
            {
                AcpiExDumpOperand (ObjDesc->Package.Elements[Index], Depth+1);
            }
        }
        break;

    case ACPI_TYPE_REGION:

        AcpiOsPrintf ("Region %s (%X)",
            AcpiUtGetRegionName (ObjDesc->Region.SpaceId),
            ObjDesc->Region.SpaceId);

        /*
         * If the address and length have not been evaluated,
         * don't print them.
         */
        if (!(ObjDesc->Region.Flags & AOPOBJ_DATA_VALID))
        {
            AcpiOsPrintf ("\n");
        }
        else
        {
            AcpiOsPrintf (" base %8.8X%8.8X Length %X\n",
                ACPI_FORMAT_NATIVE_UINT (ObjDesc->Region.Address),
                ObjDesc->Region.Length);
        }
        break;

    case ACPI_TYPE_STRING:

        AcpiOsPrintf ("String length %X @ %p ",
            ObjDesc->String.Length,
            ObjDesc->String.Pointer);

        AcpiUtPrintString (ObjDesc->String.Pointer, ACPI_UINT8_MAX);
        AcpiOsPrintf ("\n");
        break;

    case ACPI_TYPE_LOCAL_BANK_FIELD:

        AcpiOsPrintf ("BankField\n");
        break;

    case ACPI_TYPE_LOCAL_REGION_FIELD:

        AcpiOsPrintf ("RegionField: Bits=%X AccWidth=%X Lock=%X Update=%X at "
            "byte=%X bit=%X of below:\n",
            ObjDesc->Field.BitLength,
            ObjDesc->Field.AccessByteWidth,
            ObjDesc->Field.FieldFlags & AML_FIELD_LOCK_RULE_MASK,
            ObjDesc->Field.FieldFlags & AML_FIELD_UPDATE_RULE_MASK,
            ObjDesc->Field.BaseByteOffset,
            ObjDesc->Field.StartFieldBitOffset);

        AcpiExDumpOperand (ObjDesc->Field.RegionObj, Depth+1);
        break;

    case ACPI_TYPE_LOCAL_INDEX_FIELD:

        AcpiOsPrintf ("IndexField\n");
        break;

    case ACPI_TYPE_BUFFER_FIELD:

        AcpiOsPrintf ("BufferField: %X bits at byte %X bit %X of\n",
            ObjDesc->BufferField.BitLength,
            ObjDesc->BufferField.BaseByteOffset,
            ObjDesc->BufferField.StartFieldBitOffset);

        if (!ObjDesc->BufferField.BufferObj)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "*NULL*\n"));
        }
        else if ((ObjDesc->BufferField.BufferObj)->Common.Type !=
                    ACPI_TYPE_BUFFER)
        {
            AcpiOsPrintf ("*not a Buffer*\n");
        }
        else
        {
            AcpiExDumpOperand (ObjDesc->BufferField.BufferObj, Depth+1);
        }
        break;

    case ACPI_TYPE_EVENT:

        AcpiOsPrintf ("Event\n");
        break;

    case ACPI_TYPE_METHOD:

        AcpiOsPrintf ("Method(%X) @ %p:%X\n",
            ObjDesc->Method.ParamCount,
            ObjDesc->Method.AmlStart,
            ObjDesc->Method.AmlLength);
        break;

    case ACPI_TYPE_MUTEX:

        AcpiOsPrintf ("Mutex\n");
        break;

    case ACPI_TYPE_DEVICE:

        AcpiOsPrintf ("Device\n");
        break;

    case ACPI_TYPE_POWER:

        AcpiOsPrintf ("Power\n");
        break;

    case ACPI_TYPE_PROCESSOR:

        AcpiOsPrintf ("Processor\n");
        break;

    case ACPI_TYPE_THERMAL:

        AcpiOsPrintf ("Thermal\n");
        break;

    default:

        /* Unknown Type */

        AcpiOsPrintf ("Unknown Type %X\n", ObjDesc->Common.Type);
        break;
    }

    return;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpOperands
 *
 * PARAMETERS:  Operands            - A list of Operand objects
 *              OpcodeName          - AML opcode name
 *              NumOperands         - Operand count for this opcode
 *
 * DESCRIPTION: Dump the operands associated with the opcode
 *
 ******************************************************************************/

void
AcpiExDumpOperands (
    ACPI_OPERAND_OBJECT     **Operands,
    const char              *OpcodeName,
    UINT32                  NumOperands)
{
    ACPI_FUNCTION_NAME (ExDumpOperands);


    if (!OpcodeName)
    {
        OpcodeName = "UNKNOWN";
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "**** Start operand dump for opcode [%s], %u operands\n",
        OpcodeName, NumOperands));

    if (NumOperands == 0)
    {
        NumOperands = 1;
    }

    /* Dump the individual operands */

    while (NumOperands)
    {
        AcpiExDumpOperand (*Operands, 0);
        Operands++;
        NumOperands--;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC,
        "**** End operand dump for [%s]\n", OpcodeName));
    return;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOut* functions
 *
 * PARAMETERS:  Title               - Descriptive text
 *              Value               - Value to be displayed
 *
 * DESCRIPTION: Object dump output formatting functions. These functions
 *              reduce the number of format strings required and keeps them
 *              all in one place for easy modification.
 *
 ******************************************************************************/

static void
AcpiExOutString (
    const char              *Title,
    const char              *Value)
{
    AcpiOsPrintf ("%20s : %s\n", Title, Value);
}

static void
AcpiExOutPointer (
    const char              *Title,
    void                    *Value)
{
    AcpiOsPrintf ("%20s : %p\n", Title, Value);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpNamespaceNode
 *
 * PARAMETERS:  Node                - Descriptor to dump
 *              Flags               - Force display if TRUE
 *
 * DESCRIPTION: Dumps the members of the given.Node
 *
 ******************************************************************************/

void
AcpiExDumpNamespaceNode (
    ACPI_NAMESPACE_NODE     *Node,
    UINT32                  Flags)
{

    ACPI_FUNCTION_ENTRY ();


    if (!Flags)
    {
        /* Check if debug output enabled */

        if (!ACPI_IS_DEBUG_ENABLED (ACPI_LV_OBJECTS, _COMPONENT))
        {
            return;
        }
    }

    AcpiOsPrintf ("%20s : %4.4s\n", "Name", AcpiUtGetNodeName (Node));
    AcpiExOutString  ("Type", AcpiUtGetTypeName (Node->Type));
    AcpiExOutPointer ("Attached Object", AcpiNsGetAttachedObject (Node));
    AcpiExOutPointer ("Parent", Node->Parent);

    AcpiExDumpObject (ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, Node),
        AcpiExDumpNode);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpReferenceObj
 *
 * PARAMETERS:  Object              - Descriptor to dump
 *
 * DESCRIPTION: Dumps a reference object
 *
 ******************************************************************************/

static void
AcpiExDumpReferenceObj (
    ACPI_OPERAND_OBJECT     *ObjDesc)
{
    ACPI_BUFFER             RetBuf;
    ACPI_STATUS             Status;


    RetBuf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

    if (ObjDesc->Reference.Class == ACPI_REFCLASS_NAME)
    {
        AcpiOsPrintf (" %p ", ObjDesc->Reference.Node);

        Status = AcpiNsHandleToPathname (ObjDesc->Reference.Node, &RetBuf);
        if (ACPI_FAILURE (Status))
        {
            AcpiOsPrintf (" Could not convert name to pathname\n");
        }
        else
        {
           AcpiOsPrintf ("%s\n", (char *) RetBuf.Pointer);
           ACPI_FREE (RetBuf.Pointer);
        }
    }
    else if (ObjDesc->Reference.Object)
    {
        if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) == ACPI_DESC_TYPE_OPERAND)
        {
            AcpiOsPrintf (" Target: %p", ObjDesc->Reference.Object);
            if (ObjDesc->Reference.Class == ACPI_REFCLASS_TABLE)
            {
                AcpiOsPrintf (" Table Index: %X\n", ObjDesc->Reference.Value);
            }
            else
            {
                AcpiOsPrintf (" Target: %p [%s]\n", ObjDesc->Reference.Object,
                    AcpiUtGetTypeName (((ACPI_OPERAND_OBJECT *)
                        ObjDesc->Reference.Object)->Common.Type));
            }
        }
        else
        {
            AcpiOsPrintf (" Target: %p\n", ObjDesc->Reference.Object);
        }
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpPackageObj
 *
 * PARAMETERS:  ObjDesc             - Descriptor to dump
 *              Level               - Indentation Level
 *              Index               - Package index for this object
 *
 * DESCRIPTION: Dumps the elements of the package
 *
 ******************************************************************************/

static void
AcpiExDumpPackageObj (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  Level,
    UINT32                  Index)
{
    UINT32                  i;


    /* Indentation and index output */

    if (Level > 0)
    {
        for (i = 0; i < Level; i++)
        {
            AcpiOsPrintf ("  ");
        }

        AcpiOsPrintf ("[%.2d] ", Index);
    }

    AcpiOsPrintf ("%p ", ObjDesc);

    /* Null package elements are allowed */

    if (!ObjDesc)
    {
        AcpiOsPrintf ("[Null Object]\n");
        return;
    }

    /* Packages may only contain a few object types */

    switch (ObjDesc->Common.Type)
    {
    case ACPI_TYPE_INTEGER:

        AcpiOsPrintf ("[Integer] = %8.8X%8.8X\n",
            ACPI_FORMAT_UINT64 (ObjDesc->Integer.Value));
        break;

    case ACPI_TYPE_STRING:

        AcpiOsPrintf ("[String]  Value: ");
        AcpiUtPrintString (ObjDesc->String.Pointer, ACPI_UINT8_MAX);
        AcpiOsPrintf ("\n");
        break;

    case ACPI_TYPE_BUFFER:

        AcpiOsPrintf ("[Buffer] Length %.2X = ", ObjDesc->Buffer.Length);
        if (ObjDesc->Buffer.Length)
        {
            AcpiUtDebugDumpBuffer (ACPI_CAST_PTR (UINT8, ObjDesc->Buffer.Pointer),
                ObjDesc->Buffer.Length, DB_DWORD_DISPLAY, _COMPONENT);
        }
        else
        {
            AcpiOsPrintf ("\n");
        }
        break;

    case ACPI_TYPE_PACKAGE:

        AcpiOsPrintf ("[Package] Contains %u Elements:\n",
            ObjDesc->Package.Count);

        for (i = 0; i < ObjDesc->Package.Count; i++)
        {
            AcpiExDumpPackageObj (ObjDesc->Package.Elements[i], Level+1, i);
        }
        break;

    case ACPI_TYPE_LOCAL_REFERENCE:

        AcpiOsPrintf ("[Object Reference] Type [%s] %2.2X",
            AcpiUtGetReferenceName (ObjDesc),
            ObjDesc->Reference.Class);
        AcpiExDumpReferenceObj (ObjDesc);
        break;

    default:

        AcpiOsPrintf ("[Unknown Type] %X\n", ObjDesc->Common.Type);
        break;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExDumpObjectDescriptor
 *
 * PARAMETERS:  ObjDesc             - Descriptor to dump
 *              Flags               - Force display if TRUE
 *
 * DESCRIPTION: Dumps the members of the object descriptor given.
 *
 ******************************************************************************/

void
AcpiExDumpObjectDescriptor (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    UINT32                  Flags)
{
    ACPI_FUNCTION_TRACE (ExDumpObjectDescriptor);


    if (!ObjDesc)
    {
        return_VOID;
    }

    if (!Flags)
    {
        /* Check if debug output enabled */

        if (!ACPI_IS_DEBUG_ENABLED (ACPI_LV_OBJECTS, _COMPONENT))
        {
            return_VOID;
        }
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) == ACPI_DESC_TYPE_NAMED)
    {
        AcpiExDumpNamespaceNode ((ACPI_NAMESPACE_NODE *) ObjDesc, Flags);

        AcpiOsPrintf ("\nAttached Object (%p):\n",
            ((ACPI_NAMESPACE_NODE *) ObjDesc)->Object);

        AcpiExDumpObjectDescriptor (
            ((ACPI_NAMESPACE_NODE *) ObjDesc)->Object, Flags);
        return_VOID;
    }

    if (ACPI_GET_DESCRIPTOR_TYPE (ObjDesc) != ACPI_DESC_TYPE_OPERAND)
    {
        AcpiOsPrintf (
            "ExDumpObjectDescriptor: %p is not an ACPI operand object: [%s]\n",
            ObjDesc, AcpiUtGetDescriptorName (ObjDesc));
        return_VOID;
    }

    if (ObjDesc->Common.Type > ACPI_TYPE_NS_NODE_MAX)
    {
        return_VOID;
    }

    /* Common Fields */

    AcpiExDumpObject (ObjDesc, AcpiExDumpCommon);

    /* Object-specific fields */

    AcpiExDumpObject (ObjDesc, AcpiExDumpInfo[ObjDesc->Common.Type]);
    return_VOID;
}

#endif
