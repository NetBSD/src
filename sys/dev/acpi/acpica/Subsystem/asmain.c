
/******************************************************************************
 *
 * Module Name: asmain - Main module for the acpi source processor utility
 *              $Revision: 57 $
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


#include "acpisrc.h"
#include "acapps.h"


/* Globals */

UINT32                  Gbl_Tabs = 0;
UINT32                  Gbl_MissingBraces = 0;
UINT32                  Gbl_NonAnsiComments = 0;
UINT32                  Gbl_Files = 0;
UINT32                  Gbl_WhiteLines = 0;
UINT32                  Gbl_CommentLines = 0;
UINT32                  Gbl_SourceLines = 0;
UINT32                  Gbl_LongLines = 0;
UINT32                  Gbl_TotalLines = 0;

struct stat             Gbl_StatBuf;
char                    *Gbl_FileBuffer;
UINT32                  Gbl_FileSize;
UINT32                  Gbl_FileType;
BOOLEAN                 Gbl_VerboseMode = FALSE;
BOOLEAN                 Gbl_BatchMode = FALSE;
BOOLEAN                 Gbl_DebugStatementsMode = FALSE;
BOOLEAN                 Gbl_MadeChanges = FALSE;
BOOLEAN                 Gbl_Overwrite = FALSE;
BOOLEAN                 Gbl_WidenDeclarations = FALSE;


/******************************************************************************
 *
 * Standard/Common translation tables
 *
 ******************************************************************************/


ACPI_STRING_TABLE           StandardDataTypes[] = {

    /* Declarations first */

    {"UINT32_BIT  ",     "unsigned int",     REPLACE_SUBSTRINGS},

    {"UINT32      ",     "unsigned int",     REPLACE_SUBSTRINGS},
    {"UINT16        ",   "unsigned short",   REPLACE_SUBSTRINGS},
    {"UINT8        ",    "unsigned char",    REPLACE_SUBSTRINGS},
    {"BOOLEAN      ",    "unsigned char",    REPLACE_SUBSTRINGS},

    /* Now do embedded typecasts */

    {"UINT32",           "unsigned int",     REPLACE_SUBSTRINGS},
    {"UINT16",           "unsigned short",   REPLACE_SUBSTRINGS},
    {"UINT8",            "unsigned char",    REPLACE_SUBSTRINGS},
    {"BOOLEAN",          "unsigned char",    REPLACE_SUBSTRINGS},

    {"INT32  ",          "int    ",          REPLACE_SUBSTRINGS},
    {"INT32",            "int",              REPLACE_SUBSTRINGS},
    {"INT16",            "short",            REPLACE_SUBSTRINGS},
    {"INT8",             "char",             REPLACE_SUBSTRINGS},

    /* Put back anything we broke (such as anything with _INT32_ in it) */

    {"_int_",            "_INT32_",          REPLACE_SUBSTRINGS},
    {"_unsigned int_",   "_UINT32_",         REPLACE_SUBSTRINGS},
    {NULL,               NULL,               0}
};


/******************************************************************************
 *
 * Linux-specific translation tables
 *
 ******************************************************************************/

char                        LinuxHeader[] =
"/*\n"
" *  Copyright (C) 2000 - 2003, R. Byron Moore\n"
" *\n"
" *  This program is free software; you can redistribute it and/or modify\n"
" *  it under the terms of the GNU General Public License as published by\n"
" *  the Free Software Foundation; either version 2 of the License, or\n"
" *  (at your option) any later version.\n"
" *\n"
" *  This program is distributed in the hope that it will be useful,\n"
" *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
" *  GNU General Public License for more details.\n"
" *\n"
" *  You should have received a copy of the GNU General Public License\n"
" *  along with this program; if not, write to the Free Software\n"
" *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
" */\n";


ACPI_STRING_TABLE           LinuxDataTypes[] = {

    /* Declarations first */

    {"UINT32_BIT  ",             "u32                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"INT64       ",             "s64                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"UINT64      ",             "u64                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"UINT32      ",             "u32                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"INT32       ",             "s32                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"UINT16      ",             "u16                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"INT16       ",             "s16                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"UINT8       ",             "u8                  ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"BOOLEAN     ",             "u8                  ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"char        ",             "char                ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"void        ",             "void                ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"char *      ",             "char *              ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"void *      ",             "void *              ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},
    {"int         ",             "int                 ",     REPLACE_WHOLE_WORD | EXTRA_INDENT_C},

    /* Now do embedded typecasts */

    {"UINT64",                   "u64",                      REPLACE_WHOLE_WORD},
    {"UINT32",                   "u32",                      REPLACE_WHOLE_WORD},
    {"UINT16",                   "u16",                      REPLACE_WHOLE_WORD},
    {"UINT8",                    "u8",                       REPLACE_WHOLE_WORD},
    {"BOOLEAN",                  "u8",                       REPLACE_WHOLE_WORD},

    {"INT64  ",                  "s64    ",                  REPLACE_WHOLE_WORD},
    {"INT64",                    "s64",                      REPLACE_WHOLE_WORD},
    {"INT32  ",                  "s32    ",                  REPLACE_WHOLE_WORD},
    {"INT32",                    "s32",                      REPLACE_WHOLE_WORD},
    {"INT16  ",                  "s16    ",                  REPLACE_WHOLE_WORD},
    {"INT8   ",                  "s8     ",                  REPLACE_WHOLE_WORD},
    {"INT16",                    "s16",                      REPLACE_WHOLE_WORD},
    {"INT8",                     "s8",                       REPLACE_WHOLE_WORD},

    {NULL,                       NULL,                       0},
};


ACPI_TYPED_IDENTIFIER_TABLE           AcpiIdentifiers[] = {

    {"ACPI_ADR_SPACE_HANDLER",           SRC_TYPE_SIMPLE},
    {"ACPI_ADR_SPACE_SETUP",             SRC_TYPE_SIMPLE},
    {"ACPI_ADR_SPACE_TYPE",              SRC_TYPE_SIMPLE},
    {"ACPI_AML_OPERANDS",                SRC_TYPE_UNION},
    {"ACPI_BIT_REGISTER_INFO",           SRC_TYPE_STRUCT},
    {"ACPI_BUFFER",                      SRC_TYPE_STRUCT},
    {"ACPI_BUS_ATTRIBUTE",               SRC_TYPE_STRUCT},
    {"ACPI_COMMON_FACS",                 SRC_TYPE_STRUCT},
    {"ACPI_COMMON_STATE",                SRC_TYPE_STRUCT},
    {"ACPI_CONTROL_STATE",               SRC_TYPE_STRUCT},
    {"ACPI_CREATE_FIELD_INFO",           SRC_TYPE_STRUCT},
    {"ACPI_DB_METHOD_INFO",              SRC_TYPE_STRUCT},
    {"ACPI_DEBUG_MEM_BLOCK",             SRC_TYPE_STRUCT},
    {"ACPI_DEBUG_MEM_HEADER",            SRC_TYPE_STRUCT},
    {"ACPI_DEBUG_PRINT_INFO",            SRC_TYPE_STRUCT},
    {"ACPI_DESCRIPTOR",                  SRC_TYPE_UNION},
    {"ACPI_DEVICE_ID",                   SRC_TYPE_STRUCT},
    {"ACPI_DEVICE_INFO",                 SRC_TYPE_STRUCT},
    {"ACPI_DEVICE_WALK_INFO",            SRC_TYPE_STRUCT},
    {"ACPI_EVENT_HANDLER",               SRC_TYPE_SIMPLE},
    {"ACPI_EVENT_STATUS",                SRC_TYPE_SIMPLE},
    {"ACPI_EVENT_TYPE",                  SRC_TYPE_SIMPLE},
    {"ACPI_FIELD_INFO",                  SRC_TYPE_STRUCT},
    {"ACPI_FIND_CONTEXT",                SRC_TYPE_STRUCT},
    {"ACPI_FIXED_EVENT_HANDLER",         SRC_TYPE_STRUCT},
    {"ACPI_FIXED_EVENT_INFO",            SRC_TYPE_STRUCT},
    {"ACPI_GENERIC_ADDRESS",             SRC_TYPE_STRUCT},
    {"ACPI_GENERIC_STATE",               SRC_TYPE_UNION},
    {"ACPI_GET_DEVICES_INFO",            SRC_TYPE_STRUCT},
    {"ACPI_GPE_BLOCK_INFO",              SRC_TYPE_STRUCT},
    {"ACPI_GPE_HANDLER",                 SRC_TYPE_SIMPLE},
    {"ACPI_GPE_INDEX_INFO",              SRC_TYPE_STRUCT},
    {"ACPI_GPE_NUMBER_INFO",             SRC_TYPE_STRUCT},
    {"ACPI_GPE_REGISTER_INFO",           SRC_TYPE_STRUCT},
    {"ACPI_HANDLE",                      SRC_TYPE_SIMPLE},
    {"ACPI_INIT_HANDLER",                SRC_TYPE_SIMPLE},
    {"ACPI_INIT_WALK_INFO",              SRC_TYPE_STRUCT},
    {"ACPI_INTEGER",                     SRC_TYPE_SIMPLE},
    {"ACPI_INTEGRITY_INFO",              SRC_TYPE_STRUCT},
    {"ACPI_INTERPRETER_MODE",            SRC_TYPE_SIMPLE},
    {"ACPI_IO_ADDRESS",                  SRC_TYPE_SIMPLE},
    {"ACPI_IO_ATTRIBUTE",                SRC_TYPE_STRUCT},
    {"ACPI_MEM_SPACE_CONTEXT",           SRC_TYPE_STRUCT},
    {"ACPI_MEMORY_ATTRIBUTE",            SRC_TYPE_STRUCT},
    {"ACPI_MEMORY_LIST",                 SRC_TYPE_STRUCT},
    {"ACPI_MUTEX",                       SRC_TYPE_SIMPLE},
    {"ACPI_MUTEX_HANDLE",                SRC_TYPE_SIMPLE},
    {"ACPI_MUTEX_INFO",                  SRC_TYPE_STRUCT},
    {"ACPI_NAME",                        SRC_TYPE_SIMPLE},
    {"ACPI_NAME_UNION",                  SRC_TYPE_UNION},
    {"ACPI_NAMESPACE_NODE",              SRC_TYPE_STRUCT},
    {"ACPI_NAMESTRING_INFO",             SRC_TYPE_STRUCT},
    {"ACPI_NATIVE_INT",                  SRC_TYPE_SIMPLE},
    {"ACPI_NATIVE_UINT",                 SRC_TYPE_SIMPLE},
    {"ACPI_NOTIFY_HANDLER",              SRC_TYPE_SIMPLE},
    {"ACPI_NOTIFY_INFO",                 SRC_TYPE_STRUCT},
    {"ACPI_NS_SEARCH_DATA",              SRC_TYPE_STRUCT},
    {"ACPI_OBJ_INFO_HEADER",             SRC_TYPE_STRUCT},
    {"ACPI_OBJECT",                      SRC_TYPE_UNION},
    {"ACPI_OBJECT_ADDR_HANDLER",         SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_BANK_FIELD",           SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_BUFFER",               SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_BUFFER_FIELD",         SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_CACHE_LIST",           SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_COMMON",               SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_DATA",                 SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_DEVICE",               SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_EVENT",                SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_EXTRA",                SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_FIELD_COMMON",         SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_HANDLER",              SRC_TYPE_SIMPLE},
    {"ACPI_OBJECT_INDEX_FIELD",          SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_INTEGER",              SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_LIST",                 SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_METHOD",               SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_MUTEX",                SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_NOTIFY_COMMON",        SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_NOTIFY_HANDLER",       SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_PACKAGE",              SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_POWER_RESOURCE",       SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_PROCESSOR",            SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_REFERENCE",            SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_REGION",               SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_REGION_FIELD",         SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_STRING",               SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_THERMAL_ZONE",         SRC_TYPE_STRUCT},
    {"ACPI_OBJECT_TYPE",                 SRC_TYPE_SIMPLE},
    {"ACPI_OBJECT_TYPE8",                SRC_TYPE_SIMPLE},
    {"ACPI_OP_WALK_INFO",                SRC_TYPE_STRUCT},
    {"ACPI_OPCODE_INFO",                 SRC_TYPE_STRUCT},
    {"ACPI_OPERAND_OBJECT",              SRC_TYPE_UNION},
    {"ACPI_OWNER_ID",                    SRC_TYPE_SIMPLE},
    {"ACPI_PARSE_DOWNWARDS",             SRC_TYPE_SIMPLE},
    {"ACPI_PARSE_OBJ_ASL",               SRC_TYPE_STRUCT},
    {"ACPI_PARSE_OBJ_COMMON",            SRC_TYPE_STRUCT},
    {"ACPI_PARSE_OBJ_NAMED",             SRC_TYPE_STRUCT},
    {"ACPI_PARSE_OBJECT",                SRC_TYPE_UNION},
    {"ACPI_PARSE_STATE",                 SRC_TYPE_STRUCT},
    {"ACPI_PARSE_UPWARDS",               SRC_TYPE_SIMPLE},
    {"ACPI_PARSE_VALUE",                 SRC_TYPE_UNION},
    {"ACPI_PCI_ID",                      SRC_TYPE_STRUCT},
    {"ACPI_PCI_ROUTING_TABLE",           SRC_TYPE_STRUCT},
    {"ACPI_PHYSICAL_ADDRESS",            SRC_TYPE_SIMPLE},
    {"ACPI_PKG_CALLBACK",                SRC_TYPE_SIMPLE},
    {"ACPI_PKG_INFO",                    SRC_TYPE_STRUCT},
    {"ACPI_PKG_STATE",                   SRC_TYPE_STRUCT},
    {"ACPI_POINTER",                     SRC_TYPE_STRUCT},
    {"ACPI_POINTERS",                    SRC_TYPE_UNION},
    {"ACPI_PREDEFINED_NAMES",            SRC_TYPE_STRUCT},
    {"ACPI_PSCOPE_STATE",                SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE",                    SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_ADDRESS16",          SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_ADDRESS32",          SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_ADDRESS64",          SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_ATTRIBUTE",          SRC_TYPE_UNION},
    {"ACPI_RESOURCE_DATA",               SRC_TYPE_UNION},
    {"ACPI_RESOURCE_DMA",                SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_END_TAG",            SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_EXT_IRQ",            SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_FIXED_IO",           SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_FIXED_MEM32",        SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_IO",                 SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_IRQ",                SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_MEM24",              SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_MEM32",              SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_SOURCE",             SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_START_DPF",          SRC_TYPE_STRUCT},
    {"ACPI_RESOURCE_TYPE",               SRC_TYPE_SIMPLE},
    {"ACPI_RESOURCE_VENDOR",             SRC_TYPE_STRUCT},
    {"ACPI_RESULT_VALUES",               SRC_TYPE_STRUCT},
    {"ACPI_SCOPE_STATE",                 SRC_TYPE_STRUCT},
    {"ACPI_SIGNAL_FATAL_INFO",           SRC_TYPE_STRUCT},
    {"ACPI_SIZE",                        SRC_TYPE_SIMPLE},
    {"ACPI_STATUS",                      SRC_TYPE_SIMPLE},
    {"ACPI_STRING",                      SRC_TYPE_SIMPLE},
    {"ACPI_SYSTEM_INFO",                 SRC_TYPE_STRUCT},
    {"ACPI_TABLE_DESC",                  SRC_TYPE_STRUCT},
    {"ACPI_TABLE_HEADER",                SRC_TYPE_STRUCT},
    {"ACPI_TABLE_INFO",                  SRC_TYPE_STRUCT},
    {"ACPI_TABLE_PTR",                   SRC_TYPE_SIMPLE},
    {"ACPI_TABLE_SUPPORT",               SRC_TYPE_STRUCT},
    {"ACPI_TABLE_TYPE",                  SRC_TYPE_SIMPLE},
    {"ACPI_THREAD_STATE",                SRC_TYPE_STRUCT},
    {"ACPI_UPDATE_STATE",                SRC_TYPE_STRUCT},
    {"ACPI_WALK_CALLBACK",               SRC_TYPE_SIMPLE},
    {"ACPI_WALK_INFO",                   SRC_TYPE_STRUCT},
    {"ACPI_WALK_STATE",                  SRC_TYPE_STRUCT},
    {"APIC_HEADER",                      SRC_TYPE_STRUCT},
    {"ARGUMENT_INFO",                    SRC_TYPE_STRUCT},
    {"ASL_ANALYSIS_WALK_INFO",           SRC_TYPE_STRUCT},
    {"ASL_DMA_FORMAT_DESC",              SRC_TYPE_STRUCT},
    {"ASL_DWORD_ADDRESS_DESC",           SRC_TYPE_STRUCT},
    {"ASL_END_DEPENDENT_DESC",           SRC_TYPE_STRUCT},
    {"ASL_END_TAG_DESC",                 SRC_TYPE_STRUCT},
    {"ASL_ERROR_MSG",                    SRC_TYPE_STRUCT},
    {"ASL_EVENT_INFO",                   SRC_TYPE_STRUCT},
    {"ASL_EXTENDED_XRUPT_DESC",          SRC_TYPE_STRUCT},
    {"ASL_FILE_INFO",                    SRC_TYPE_STRUCT},
    {"ASL_FIXED_IO_PORT_DESC",           SRC_TYPE_STRUCT},
    {"ASL_FIXED_MEMORY_32_DESC",         SRC_TYPE_STRUCT},
    {"ASL_GENERAL_REGISTER_DESC",        SRC_TYPE_STRUCT},
    {"ASL_IO_PORT_DESC",                 SRC_TYPE_STRUCT},
    {"ASL_IRQ_FORMAT_DESC",              SRC_TYPE_STRUCT},
    {"ASL_IRQ_NOFLAGS_DESC",             SRC_TYPE_STRUCT},
    {"ASL_LARGE_VENDOR_DESC",            SRC_TYPE_STRUCT},
    {"ASL_LISTING_NODE",                 SRC_TYPE_STRUCT},
    {"ASL_MAPPING_ENTRY",                SRC_TYPE_STRUCT},
    {"ASL_MEMORY_24_DESC",               SRC_TYPE_STRUCT},
    {"ASL_MEMORY_32_DESC",               SRC_TYPE_STRUCT},
    {"ASL_METHOD_INFO",                  SRC_TYPE_STRUCT},
    {"ASL_QWORD_ADDRESS_DESC",           SRC_TYPE_STRUCT},
    {"ASL_RESERVED_INFO",                SRC_TYPE_STRUCT},
    {"ASL_RESOURCE_DESC",                SRC_TYPE_UNION},
    {"ASL_RESOURCE_NODE",                SRC_TYPE_STRUCT},
    {"ASL_SMALL_VENDOR_DESC",            SRC_TYPE_STRUCT},
    {"ASL_START_DEPENDENT_DESC",         SRC_TYPE_STRUCT},
    {"ASL_START_DEPENDENT_NOPRIO_DESC",  SRC_TYPE_STRUCT},
    {"ASL_WALK_CALLBACK",                SRC_TYPE_SIMPLE},
    {"ASL_WORD_ADDRESS_DESC",            SRC_TYPE_STRUCT},
    {"COMMAND_INFO",                     SRC_TYPE_STRUCT},
/*    {"FACS_DESCRIPTOR",                  SRC_TYPE_SIMPLE}, */
    {"FACS_DESCRIPTOR_REV1",             SRC_TYPE_STRUCT},
    {"FACS_DESCRIPTOR_REV2",             SRC_TYPE_STRUCT},
/*    {"FADT_DESCRIPTOR",                  SRC_TYPE_SIMPLE}, */
    {"FADT_DESCRIPTOR_REV1",             SRC_TYPE_STRUCT},
    {"FADT_DESCRIPTOR_REV2",             SRC_TYPE_STRUCT},
    {"RSDP_DESCRIPTOR",                  SRC_TYPE_STRUCT},
/*    {"RSDT_DESCRIPTOR",                  SRC_TYPE_SIMPLE}, */
    {"RSDT_DESCRIPTOR_REV1",             SRC_TYPE_STRUCT},
    {"RSDT_DESCRIPTOR_REV2",             SRC_TYPE_STRUCT},
    {"UINT32_STRUCT",                    SRC_TYPE_STRUCT},
    {"UINT64_OVERLAY",                   SRC_TYPE_UNION},
    {"UINT64_STRUCT",                    SRC_TYPE_STRUCT},
/*    {"XSDT_DESCRIPTOR",                  SRC_TYPE_SIMPLE}, */
    {"XSDT_DESCRIPTOR_REV2",             SRC_TYPE_STRUCT},

    {NULL}
};

ACPI_IDENTIFIER_TABLE       LinuxAddStruct[] = {
    {"acpi_namespace_node"},
    {"acpi_parse_object"},
    {"acpi_table_desc"},
    {"acpi_walk_state"},
    {NULL}
};


ACPI_IDENTIFIER_TABLE       LinuxEliminateMacros[] = {

    {"ACPI_GET_ADDRESS"},
    {"ACPI_VALID_ADDRESS"},
    {NULL}
};


ACPI_IDENTIFIER_TABLE       LinuxEliminateLines_C[] = {

    {"#define __"},
    {"$Revision"},
    {NULL}
};


ACPI_IDENTIFIER_TABLE       LinuxEliminateLines_H[] = {

    {"$Revision"},
    {NULL}
};


ACPI_IDENTIFIER_TABLE       LinuxConditionalIdentifiers[] = {

//    {"ACPI_USE_STANDARD_HEADERS"},
    {"WIN32"},
    {"_MSC_VER"},
    {NULL}
};

ACPI_CONVERSION_TABLE       LinuxConversionTable = {

    LinuxHeader,
    FLG_NO_CARRIAGE_RETURNS | FLG_LOWERCASE_DIRNAMES,

    AcpiIdentifiers,

    /* C source files */

    LinuxDataTypes,
    LinuxEliminateLines_C,
    NULL,
    LinuxEliminateMacros,
    AcpiIdentifiers,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES |
     CVT_CHECK_BRACES | CVT_TRIM_LINES | CVT_BRACES_ON_SAME_LINE |
     CVT_MIXED_CASE_TO_UNDERSCORES | CVT_LOWER_CASE_IDENTIFIERS |
     CVT_REMOVE_DEBUG_MACROS | CVT_TRIM_WHITESPACE |
     CVT_REMOVE_EMPTY_BLOCKS | CVT_SPACES_TO_TABS8),

    /* C header files */

    LinuxDataTypes,
    LinuxEliminateLines_H,
    LinuxConditionalIdentifiers,
    NULL,
    AcpiIdentifiers,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES |
     CVT_TRIM_LINES | CVT_MIXED_CASE_TO_UNDERSCORES |
     CVT_LOWER_CASE_IDENTIFIERS | CVT_TRIM_WHITESPACE |
     CVT_REMOVE_EMPTY_BLOCKS| CVT_REDUCE_TYPEDEFS | CVT_SPACES_TO_TABS8),
};


/******************************************************************************
 *
 * Code cleanup translation tables
 *
 ******************************************************************************/


ACPI_CONVERSION_TABLE       CleanupConversionTable = {

    NULL,
    FLG_DEFAULT_FLAGS,
    NULL,
    /* C source files */

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES |
     CVT_CHECK_BRACES | CVT_TRIM_LINES | CVT_TRIM_WHITESPACE),

    /* C header files */

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES |
     CVT_TRIM_LINES | CVT_TRIM_WHITESPACE),
};


ACPI_CONVERSION_TABLE       StatsConversionTable = {

    NULL,
    FLG_NO_FILE_OUTPUT,
    NULL,

    /* C source files */

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES),

    /* C header files */

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES),
};


/******************************************************************************
 *
 * Customizable translation tables
 *
 ******************************************************************************/

ACPI_STRING_TABLE           CustomReplacements[] = {

    {"(c) 1999 - 2003",      "(c) 1999 - 2003",          REPLACE_WHOLE_WORD},

#if 0
    "ACPI_NATIVE_UINT",     "ACPI_NATIVE_UINT",         REPLACE_WHOLE_WORD,
    "ACPI_NATIVE_UINT *",   "ACPI_NATIVE_UINT *",       REPLACE_WHOLE_WORD,
    "ACPI_NATIVE_UINT",     "ACPI_NATIVE_UINT",         REPLACE_WHOLE_WORD,
    "ACPI_NATIVE_INT",      "ACPI_NATIVE_INT",          REPLACE_WHOLE_WORD,
    "ACPI_NATIVE_INT *",    "ACPI_NATIVE_INT *",        REPLACE_WHOLE_WORD,
    "ACPI_NATIVE_INT",      "ACPI_NATIVE_INT",          REPLACE_WHOLE_WORD,
#endif

    {NULL,                    NULL, 0}
};


ACPI_CONVERSION_TABLE       CustomConversionTable = {

    NULL,
    FLG_DEFAULT_FLAGS,
    NULL,

    /* C source files */

    CustomReplacements,
    NULL,
    NULL,
    NULL,
    NULL,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES |
     CVT_TRIM_LINES | CVT_TRIM_WHITESPACE),

    /* C header files */

    CustomReplacements,
    NULL,
    NULL,
    NULL,
    NULL,
    (CVT_COUNT_TABS | CVT_COUNT_NON_ANSI_COMMENTS | CVT_COUNT_LINES |
     CVT_TRIM_LINES | CVT_TRIM_WHITESPACE),
};


/******************************************************************************
 *
 * FUNCTION:    AsExaminePaths
 *
 * DESCRIPTION: Source and Target pathname verification and handling
 *
 ******************************************************************************/

int
AsExaminePaths (
    ACPI_CONVERSION_TABLE   *ConversionTable,
    char                    *Source,
    char                    *Target,
    UINT32                  *SourceFileType)
{
    int                     Status;
    char                    Response;


    Status = stat (Source, &Gbl_StatBuf);
    if (Status)
    {
        printf ("Source path \"%s\" does not exist\n", Source);
        return -1;
    }

    /* Return the filetype -- file or a directory */

    *SourceFileType = 0;
    if (Gbl_StatBuf.st_mode & S_IFDIR)
    {
        *SourceFileType = S_IFDIR;
    }

    /*
     * If we are in no-output mode or in batch mode, we are done
     */
    if ((ConversionTable->Flags & FLG_NO_FILE_OUTPUT) ||
        (Gbl_BatchMode))
    {
        return 0;
    }

    if (!stricmp (Source, Target))
    {
        printf ("Target path is the same as the source path, overwrite?\n");
        scanf ("%c", &Response);

        /* Check response */

        if ((char) Response != 'y')
        {
            return -1;
        }

        Gbl_Overwrite = TRUE;
    }
    else
    {
        Status = stat (Target, &Gbl_StatBuf);
        if (!Status)
        {
            printf ("Target path already exists, overwrite?\n");
            scanf ("%c", &Response);

            /* Check response */

            if ((char) Response != 'y')
            {
                return -1;
            }
        }
    }

    return 0;
}


/******************************************************************************
 *
 * FUNCTION:    AsDisplayStats
 *
 * DESCRIPTION: Display global statistics gathered during translation
 *
 ******************************************************************************/

void
AsDisplayStats (void)
{

    printf ("\nAcpiSrc statistics:\n\n");
    printf ("%6d Files processed\n", Gbl_Files);
    printf ("%6d Tabs found\n", Gbl_Tabs);
    printf ("%6d Missing if/else braces\n", Gbl_MissingBraces);
    printf ("%6d Non-ANSI comments found\n", Gbl_NonAnsiComments);
    printf ("%6d Total Lines\n", Gbl_TotalLines);
    printf ("%6d Lines of code\n", Gbl_SourceLines);
    printf ("%6d Lines of non-comment whitespace\n", Gbl_WhiteLines);
    printf ("%6d Lines of comments\n", Gbl_CommentLines);
    printf ("%6d Long lines found\n", Gbl_LongLines);
    printf ("%6.1f Ratio of code to whitespace\n", ((float) Gbl_SourceLines / (float) Gbl_WhiteLines));
    printf ("%6.1f Ratio of code to comments\n", ((float) Gbl_SourceLines / (float) Gbl_CommentLines));
    return;
}


/******************************************************************************
 *
 * FUNCTION:    AsDisplayUsage
 *
 * DESCRIPTION: Usage message
 *
 ******************************************************************************/

void
AsDisplayUsage (void)
{

    printf ("\n");
    printf ("Usage: acpisrc [-c|l|u] [-dsvy] <SourceDir> <DestinationDir>\n\n");
    printf ("Where: -c          Generate cleaned version of the source\n");
    printf ("       -l          Generate Linux version of the source\n");
    printf ("       -u          Generate Custom source translation\n");
    printf ("\n");
    printf ("       -d          Leave debug statements in code\n");
    printf ("       -s          Generate source statistics only\n");
    printf ("       -v          Verbose mode\n");
    printf ("       -y          Suppress file overwrite prompts\n");
    printf ("\n");
    return;
}


/******************************************************************************
 *
 * FUNCTION:    main
 *
 * DESCRIPTION: C main function
 *
 ******************************************************************************/

int ACPI_SYSTEM_XFACE
main (
    int                     argc,
    char                    *argv[])
{
    int                     j;
    ACPI_CONVERSION_TABLE   *ConversionTable = NULL;
    char                    *SourcePath;
    char                    *TargetPath;
    UINT32                  FileType;


    printf ("ACPI Source Code Conversion Utility");
    printf (" version %8.8X", ((UINT32) ACPI_CA_VERSION));
    printf (" [%s]\n\n",  __DATE__);

    if (argc < 2)
    {
        AsDisplayUsage ();
        return 0;
    }

    /* Command line options */

    while ((j = AcpiGetopt (argc, argv, "lcsuvyd")) != EOF) switch(j)
    {
    case 'l':
        /* Linux code generation */

        printf ("Creating Linux source code\n");
        ConversionTable = &LinuxConversionTable;
        Gbl_WidenDeclarations = TRUE;
        break;

    case 'c':
        /* Cleanup code */

        printf ("Code cleanup\n");
        ConversionTable = &CleanupConversionTable;
        break;

    case 's':
        /* Statistics only */

        break;

    case 'u':
        /* custom conversion  */

        printf ("Custom source translation\n");
        ConversionTable = &CustomConversionTable;
        break;

    case 'v':
        /* Verbose mode */

        Gbl_VerboseMode = TRUE;
        break;

    case 'y':
        /* Batch mode */

        Gbl_BatchMode = TRUE;
        break;

    case 'd':
        /* Leave debug statements in */

        Gbl_DebugStatementsMode = TRUE;
        break;

    default:
        AsDisplayUsage ();
        return -1;
    }


    SourcePath = argv[AcpiGbl_Optind];
    if (!SourcePath)
    {
        printf ("Missing source path\n");
        AsDisplayUsage ();
        return -1;
    }

    TargetPath = argv[AcpiGbl_Optind+1];

    if (!ConversionTable)
    {
        /* Just generate statistics.  Ignore target path */

        TargetPath = SourcePath;

        printf ("Source code statistics only\n");
        ConversionTable = &StatsConversionTable;
    }
    else if (!TargetPath)
    {
        TargetPath = SourcePath;
    }

    if (Gbl_DebugStatementsMode)
    {
        ConversionTable->SourceFunctions &= ~CVT_REMOVE_DEBUG_MACROS;
    }

    /* Check source and target paths and files */

    if (AsExaminePaths (ConversionTable, SourcePath, TargetPath, &FileType))
    {
        return -1;
    }

    /* Source/target can be either directories or a files */

    if (FileType == S_IFDIR)
    {
        /* Process the directory tree */

        AsProcessTree (ConversionTable, SourcePath, TargetPath);
    }
    else
    {
        /* Process a single file */

        /* Differentiate between source and header files */

        if (strstr (SourcePath, ".h"))
        {
            AsProcessOneFile (ConversionTable, NULL, TargetPath, 0, SourcePath, FILE_TYPE_HEADER);
        }
        else
        {
            AsProcessOneFile (ConversionTable, NULL, TargetPath, 0, SourcePath, FILE_TYPE_SOURCE);
        }
    }

    /* Always display final summary and stats */

    AsDisplayStats ();

    return 0;
}
