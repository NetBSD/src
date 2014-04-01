/*	$NetBSD: error.c,v 1.1.1.1 2014/04/01 16:16:06 jakllsch Exp $	*/

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    error.c

Abstract:




Revision History

--*/

#include "lib.h"


struct {
    EFI_STATUS      Code;
    WCHAR	    *Desc;
} ErrorCodeTable[] = {
	{  EFI_SUCCESS,                L"Success"},
	{  EFI_LOAD_ERROR,             L"Load Error"},
	{  EFI_INVALID_PARAMETER,      L"Invalid Parameter"},
	{  EFI_UNSUPPORTED,            L"Unsupported"},
	{  EFI_BAD_BUFFER_SIZE,        L"Bad Buffer Size"},
	{  EFI_BUFFER_TOO_SMALL,       L"Buffer Too Small"},
	{  EFI_NOT_READY,              L"Not Ready"},
	{  EFI_DEVICE_ERROR,           L"Device Error"},
	{  EFI_WRITE_PROTECTED,        L"Write Protected"},
	{  EFI_OUT_OF_RESOURCES,       L"Out of Resources"},
	{  EFI_VOLUME_CORRUPTED,       L"Volume Corrupt"},
	{  EFI_VOLUME_FULL,            L"Volume Full"},
	{  EFI_NO_MEDIA,               L"No Media"},
	{  EFI_MEDIA_CHANGED,          L"Media changed"},
	{  EFI_NOT_FOUND,              L"Not Found"},
	{  EFI_ACCESS_DENIED,          L"Access Denied"},
	{  EFI_NO_RESPONSE,            L"No Response"},
	{  EFI_NO_MAPPING,             L"No mapping"},
	{  EFI_TIMEOUT,                L"Time out"},
	{  EFI_NOT_STARTED,            L"Not started"},
	{  EFI_ALREADY_STARTED,        L"Already started"},
	{  EFI_ABORTED,                L"Aborted"},
	{  EFI_ICMP_ERROR,             L"ICMP Error"},
	{  EFI_TFTP_ERROR,             L"TFTP Error"},
	{  EFI_PROTOCOL_ERROR,         L"Protocol Error"},

	// warnings
	{  EFI_WARN_UNKOWN_GLYPH,      L"Warning Unknown Glyph"},
	{  EFI_WARN_DELETE_FAILURE,    L"Warning Delete Failure"},
	{  EFI_WARN_WRITE_FAILURE,     L"Warning Write Failure"},
	{  EFI_WARN_BUFFER_TOO_SMALL,  L"Warning Buffer Too Small"},
	{  0, NULL}
} ;


VOID
StatusToString (
    OUT CHAR16          *Buffer,
    IN EFI_STATUS       Status
    )
{
    UINTN           Index;

    for (Index = 0; ErrorCodeTable[Index].Desc; Index +=1) {
        if (ErrorCodeTable[Index].Code == Status) {
	    StrCpy (Buffer, ErrorCodeTable[Index].Desc);
            return;
        }
    }

    SPrint (Buffer, 0, L"%X", Status);
}
