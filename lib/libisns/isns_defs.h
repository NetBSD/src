/*	$NetBSD: isns_defs.h,v 1.1.1.1 2011/01/16 01:22:50 agc Exp $	*/

/*-
 * Copyright (c) 2004,2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ISNS_DEFS_H_
#define	_ISNS_DEFS_H_

/*
 * enum of iSNS Registration, query, and response types
 */

typedef enum {
	isnsp_DevAttrReg = 1,
	isnsp_DevAttrQry,
	isnsp_DevGetNext,
	isnsp_DevDereg,
	isnsp_SCNReg,
	isnsp_SCNDereg,
	isnsp_SCNEvent,
	isnsp_SCN,
	isnsp_DDReg,
	isnsp_DDDereg,
	isnsp_DDSReg,
	isnsp_DDSDereg,
	isnsp_ESI,
	isnsp_Heartbeat,	/* 0x000e */

	/* Next few are iFCP only */
	isnsp_RqstDomId = 0x0011,
	isnsp_RlseDomId,
	isnsp_GetDomId,

	isnsp_DevAttrRegRsp = 0x8001,
	isnsp_DevAttrQryRsp,
	isnsp_DevGetNextRsp,
	isnsp_DevDeregRsp,
	isnsp_SCNregRsp,
	isnsp_SCNDeregRsp,
	isnsp_SCNeventRsp,
	isnsp_SCNRsp,
	isnsp_DDRegRsp,
	isnsp_DDDeregRsp,
	isnsp_DDSRegRsp,
	isnsp_DDSDeregRsp,
	isnsp_ESIRsp,		/* 0x800d */

	/* Next few are iFCP only */
	isnsp_RqstDomIdRsp = 0x8011,
	isnsp_RlseDomIdRsp,
	isnsp_GetDomIdRsp
} isnsp_func_id_t;

/*
 * enum of iSNS tag types
 */

typedef enum {			/*  Len      Reg Key	Query Key	 Val */
	isnst_Delimiter = 0,	/*   0		N/A	N/A		   0 */
	isnst_EID,		/* 4-256	 1	1|2|16&17|32|64	   1 */
	isnst_EntProtocol,	/*   4		 1	1|2|16&17|32|64	   2 */
	isnst_MgtIPAddr,	/*  16		 1	1|2|16&17|32|64	   3 */
	isnst_Timestamp,	/*   8		--	1|2|16&17|32|64	   4 */
	isnst_ProtVersRange,	/*   4		 1	1|2|16&17|32|64	   5 */
	isnst_RegPeriod,	/*   4		 1	1|2|16&17|32|64	   6 */
	isnst_EntityIndex,	/*   4		 1	1|2|16&17|32|64	   7 */
	isnst_EntityNextIndex,	/*   8		 1	1|2|16&17|32|64	   8 */
				/*					     */
	isnst_EntISAKMP_P1= 11,	/*  var		 1	1|2|16&17|32|64	  11 */
	isnst_Certificate,	/*  var		 1	1|2|16&17|32|64	  12 */
				/*					     */
	isnst_PortalIPAddr= 16,	/*  16		 1	1|2|16&17|32|64	  16 */
	isnst_PortalPort,	/*   4		 1	1|2|16&17|32|64	  17 */
	isnst_SymbName, 	/* 4-256       16&17	1|16&17|32|64	  18 */
	isnst_ESIIntval,	/*   4	       16&17	1|16&17|32|64	  19 */
	isnst_ESIPort,		/*   4	       16&17	1|16&17|32|64	  20 */
				/*					     */
	isnst_PortalIndex=22,	/*   4	       16&17	1|16&17|32|64	  22 */
	isnst_SCNPort,		/*   4	       16&17	1|16&17|32|64	  23 */
	isnst_PortalNextIndex,	/*   4		--	1|16&17|32|64	  24 */
				/*					     */
	isnst_PortalSecBmap=27,	/*   4	       16&17	1|16&17|32|64	  27 */
	isnst_PortalISAKMP_P1,	/*  var	       16&17	1|16&17|32|64	  28 */
	isnst_PortalISAKMP_P2,	/*  var	       16&17	1|16&17|32|64	  29 */
				/*					     */
	isnst_PortalCert = 31,	/*  var	       16&17	1|16&17|32|64	  31 */
	isnst_iSCSIName,	/* 4-224	 1	1|16&17|32|33	  32 */
	isnst_iSCSINodeType,	/*   4		32	1|16&17|32	  33 */
	isnst_iSCSIAlias,	/* 4-256	32	1|16&17|32	  34 */
	isnst_iSCSISCNBmap,	/*   4		32	1|16&17|32	  35 */
	isnst_iSCSINodeIndex,	/*   4		32	1|16&17|32	  36 */
	isnst_WWNNToken,	/*   8		32	1|16&17|32	  37 */
	isnst_iSCSINodeNextIdx, /*   4		--	1|16&17|32	  38 */
				/*					     */
	isnst_iSCSIAuthMethod=42,/* var		32	1|16&17|32	  42 */
	isnst_iSCSINodeCert,	/*  var		32	1|16&17|32	  43 */
				/*					     */
	isnst_PGiSCSIName=48,	/* 4-224     32|16&17	1|16&17|32|52	  48 */
	isnst_PGPortIPAddr,	/*  16	     32|16&17	1|16&17|32|52	  49 */
	isnst_PGPortIPPort,	/*   4	     32|16&17	1|16&17|32|52	  50 */
	isnst_PGTag,		/*   4	     32|16&17	1|16&17|32|52	  51 */
	isnst_PGIndex,		/*   4	     32|16&17	1|16&17|32|52	  52 */
	isnst_PGNextIndex,	/*   4		--	1|16&17|32|52	  53 */
				/*					     */
	isnst_FCPortNameWWPN=64,/*   8		 1	1|16&17|64|66|96|128
									  64 */
	isnst_FCPortID, 	/*   4		64	1|16&17|64	  65 */
	isnst_FCPortType,	/*   4		64	1|16&17|64	  66 */
	isnst_FCSymbPortName,	/* 4-256	64	1|16&17|64	  67 */
	isnst_FCFabricPortName, /*   8		64	1|16&17|64	  68 */
	isnst_FCHardAddr,	/*   4		64	1|16&17|64	  69 */
	isnst_FCPortIPAddr,	/*  16		64	1|16&17|64	  70 */
	isnst_FCClassOService,	/*   4		64	1|16&17|64	  71 */
	isnst_FC4Types, 	/*  32		64	1|16&17|64	  72 */
	isnst_FC4Descr, 	/* 4-256	64	1|16&17|64	  73 */
	isnst_FC4Features,	/*  128		64	1|16&17|64	  74 */
	isnst_iFCPSCNBmap,	/*   4		64	1|16&17|64	  75 */
	isnst_iFCPPortRole,	/*   4		64	1|16&17|64	  76 */
	isnst_PermPortName,	/*   8		--	1|16&17|64	  77 */
				/*					     */
	isnst_PortCert = 83, 	/*  var		64	1|16&17|64	  83 */
				/*					     */
	isnst_FC4TypeCode = 95,	/*   4		--	1|16&17|64	  95 */
	isnst_FCNodeNameWWNN,	/*   8		64	1|16&17|64|96	  96 */
	isnst_SymbNodeName,	/* 4-256	96	64|96		  97 */
	isnst_NodeIPAddr,	/*  16		96	64|96		  98 */
	isnst_NodeIPA,		/*   8		96	64|96		  99 */
	isnst_NodeCert, 	/*  var 	96	64|96		 100 */
	isnst_ProxyiSCSIName,	/* 4-256	96	64|96		 101 */
				/* Note: above really should be 4-224
				 * in the iSNS spec, but isn't		     */
				/*					     */
	isnst_SwitchName = 128, /*   8	       128	128		 128 */
	isnst_PrefID,		/*   4	       128	128		 129 */
	isnst_AssignedID,	/*   4	       128	128		 130 */
	isnst_VirtFabricID,	/* 4-256       128	128		 131 */
				/*					     */
	isnst_iSNSSrvrVndOUI=256,/*  4		--	SOURCE Attr	 256 */
				/*					     */
	isnst_DDS_ID=2049,	/*   4	      2049	1|32|64|2049|2065
									2049 */
	isnst_DDS_SymName,	/* 4-256      2049	2049		2050 */
	isnst_DDS_Status,	/*   4	      2049	2049		2051 */
	isnst_DDS_Next_ID,	/*   4		--	2049		2052 */
				/*					     */
	isnst_DD_ID = 2065,	/*   4	      2049	1|32|64|2049|2065
									2065 */
	isnst_DD_SymName,	/* 4-256      2065	2065		2066 */
	isnst_DD_iSCSIIndex,	/*   4	      2065	2065		2067 */
	isnst_DD_iSCSIName,	/* 4-224      2065	2065		2068 */
	isnst_DD_iFCPNode,	/*   8	      2065	2065		2069 */
	isnst_DD_PortIndex,	/*   4	      2065	2065		2070 */
	isnst_DD_PortIPAddr,	/*  16	      2065	2065		2071 */
	isnst_DD_PortPort,	/*   4	      2065	2065		2072 */
	isnst_DD_Features=2078, /*   4	      2065	2065		2078 */
	isnst_DD_Next_ID	/*   4		--	2065		2079 */
} isnst_tag_type_t;

/*
 * iSNS PDU header flags
 */

#define ISNS_FLAG_FIRST_PDU    (0x0400)
#define ISNS_FLAG_LAST_PDU     (0x0800)
#define ISNS_FLAG_REPLACE_REG  (0x1000)
#define ISNS_FLAG_AUTH         (0x2000)
#define ISNS_FLAG_SND_SERVER   (0x4000)
#define ISNS_FLAG_SND_CLIENT   (0x8000)


#endif /* _ISNS_DEFS_H_ */
