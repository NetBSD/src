
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2007
 *
 */

#ifndef _TSP_DELEGATE_H_
#define _TSP_DELEGATE_H_

TSS_RESULT      do_delegate_manage(TSS_HTPM hTpm, UINT32 familyID, UINT32 opFlag,
				   UINT32 opDataSize, BYTE *opData, UINT32 *outDataSize, BYTE **outData);
TSS_RESULT	create_key_delegation(TSS_HKEY, BYTE, UINT32, TSS_HPCRS, TSS_HDELFAMILY, TSS_HPOLICY);
TSS_RESULT	create_owner_delegation(TSS_HTPM, BYTE, UINT32, TSS_HPCRS, TSS_HDELFAMILY, TSS_HPOLICY);

TSS_RESULT	update_delfamily_object(TSS_HTPM, UINT32);
TSS_RESULT	get_delegate_index(TSS_HCONTEXT, UINT32, TPM_DELEGATE_PUBLIC *);
TSS_RESULT	__tspi_build_delegate_public_info(BYTE, TSS_HPCRS, TSS_HDELFAMILY, TSS_HPOLICY, UINT32 *, BYTE **);

#endif
