/*	$NetBSD: mipsock.h,v 1.1.2.1 2008/02/22 02:53:33 keiichi Exp $	*/
/* $Id: mipsock.h,v 1.1.2.1 2008/02/22 02:53:33 keiichi Exp $ */

/*
 * Copyright (C) 2004 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NET_MIPSOCK_H_
#define _NET_MIPSOCK_H_

#include <netinet/in.h>

struct mip_msghdr {
 	u_int16_t miph_msglen;	/* message length */
	u_int8_t  miph_version;	/* version: future binary compatibility */
	u_int8_t  miph_type;	/* message type */
	pid_t     miph_pid;
	u_int32_t miph_seq;	/* for sender to identify action */
	u_int32_t miph_errno;	/* why failed */
};

struct mipm_bc_info {
	struct mip_msghdr mipmci_hdr;
#define mipmci_msglen 	mipmci_hdr.miph_msglen
#define mipmci_version 	mipmci_hdr.miph_version
#define mipmci_type 	mipmci_hdr.miph_type
#define mipmci_pid 	mipmci_hdr.miph_pid
#define mipmci_seq 	mipmci_hdr.miph_seq
#define mipmci_errno	mipmci_hdr.miph_errno

	int mipmci_lifetime;
	u_int16_t mipmci_flags;
	struct sockaddr mipmci_addr[0];
#define MIPC_HOA(mipc)	(&(mipc)->mipmci_addr[0])
#define MIPC_COA(mipc)	((struct sockaddr *)((char *)(MIPC_HOA(mipc)) \
				+ (MIPC_HOA(mipc))->sa_len))
#define MIPC_CNADDR(mipc)	((struct sockaddr *)((char *)(MIPC_COA(mipc)) \
				+ (MIPC_COA(mipc))->sa_len))
};

struct mipm_bul_info {
	struct mip_msghdr mipmui_hdr;
#define mipmui_msglen 	mipmui_hdr.miph_msglen
#define mipmui_version 	mipmui_hdr.miph_version
#define mipmui_type 	mipmui_hdr.miph_type
#define mipmui_pid 	mipmui_hdr.miph_pid
#define mipmui_seq 	mipmui_hdr.miph_seq
#define mipmui_errno	mipmui_hdr.miph_errno

	u_int16_t	mipmui_flags;
	u_int16_t	mipmui_hoa_ifindex;
	u_int16_t	mipmui_coa_ifindex;
	u_int8_t        mipmui_state;
	u_int8_t        mipmui_reserved[1];
	struct sockaddr mipmui_addr[0];
#define MIPU_HOA(mipu)	(&(mipu)->mipmui_addr[0])
#define MIPU_COA(mipu)	((struct sockaddr *)((char *)(MIPU_HOA(mipu)) \
				+ (MIPU_HOA(mipu))->sa_len))
#define MIPU_PEERADDR(mipu)	((struct sockaddr *)((char *)(MIPU_COA(mipu)) \
				+ (MIPU_COA(mipu))->sa_len))
};

struct mipm_nodetype_info {
	struct mip_msghdr mipmni_hdr;
#define mipmni_msglen 	mipmni_hdr.miph_msglen
#define mipmni_version 	mipmni_hdr.miph_version
#define mipmni_type 	mipmni_hdr.miph_type
#define mipmni_pid 	mipmni_hdr.miph_pid
#define mipmni_seq 	mipmni_hdr.miph_seq
#define mipmni_errno	mipmni_hdr.miph_errno

	u_int8_t mipmni_nodetype;
	u_int8_t mipmni_enable; /* set 1 to enable, 0 to disable */
};

struct mipm_home_hint {
	struct mip_msghdr mipmhh_hdr;
#define mipmhh_msglen 	mipmhh_hdr.miph_msglen
#define mipmhh_version 	mipmhh_hdr.miph_version
#define mipmhh_type 	mipmhh_hdr.miph_type
#define mipmhh_pid 	mipmhh_hdr.miph_pid
#define mipmhh_seq 	mipmhh_hdr.miph_seq
#define mipmhh_errno	mipmhh_hdr.miph_errno

	u_int16_t mipmhh_ifindex;		/* ifindex of interface
						   which received RA */
	u_int16_t mipmhh_prefixlen;		/* Prefix Length */
	struct sockaddr mipmhh_prefix[0];	/* received prefix */
};

/*
 * Usage: 
 * 
 * switch (command) {
 * case MIPM_MD_REREG:
 *    + mandate field: 
 *         'mipm_md_newcoa' (MUST set to a care-of address(s) to send BU)
 *    + options field: 
 *         'mipm_md_ifindex or mipm_md_hoa' (if an option(s) is
 *         defined, the new coa is applied only to the specified
 *         target (i.e. either mip virtual interface or HoA, or both))
 *
 * case MIPM_MD_DEREGHOME:
 *    +	mandate fields: 
 *          'mipm_md_newcoa' (MUST set to the home address)
 *          'mipm_md_ifindex or mipm_md_hoa' (MUST set either a mip
 *          virtual interface or a HoA (can be both) which is now
 *          returned to home)
 *
 * case MIPM_MD_DEREGFOREIGN:
 *    +	mandate fields: 
 *           'mipm_md_newcoa' (MUST set to a CoA to send dereg BU) 
 *           'mipm_md_ifindex or mipm_md_hoa' (MUST set either a mip
 *           virtual interface or a HoA (can be both) which is now
 *           returned to home) 
 * } 
 */
struct mipm_md_info {
	struct mip_msghdr mipmmi_hdr;
#define mipmmi_msglen 	mipmmi_hdr.miph_msglen
#define mipmmi_version 	mipmmi_hdr.miph_version
#define mipmmi_type 	mipmmi_hdr.miph_type
#define mipmmi_pid 	mipmmi_hdr.miph_pid
#define mipmmi_seq 	mipmmi_hdr.miph_seq
#define mipmmi_errno	mipmmi_hdr.miph_errno

	u_char mipmmi_command;
#define MIPM_MD_REREG 		0x01
#define MIPM_MD_DEREGHOME 	0x02
#define MIPM_MD_DEREGFOREIGN 	0x03
#define MIPM_MD_SCAN            0x04
	
	u_char mipmmi_hint;
#define MIPM_MD_INDEX 		0x01
#define MIPM_MD_ADDR 		0x02
#define MIPM_MD_HOME 		0x03

	u_int16_t mipmmi_ifindex;
	struct sockaddr mipmmi_addr[0];
#define MIPD_HOA(mipd)	(&(mipd)->mipmmi_addr[0])
#define MIPD_COA(mipd)	((struct sockaddr *)((char *)(MIPD_HOA(mipd)) \
				+ (MIPD_HOA(mipd))->sa_len))
#define MIPD_COA2(mipd)	((struct sockaddr *)((char *)(MIPD_COA(mipd)) \
				+ (MIPD_COA(mipd))->sa_len))
};

struct mipm_rr_hint {
	struct mip_msghdr mipmrh_hdr;
#define mipmrh_msglen 	mipmrh_hdr.miph_msglen
#define mipmrh_version 	mipmrh_hdr.miph_version
#define mipmrh_type 	mipmrh_hdr.miph_type
#define mipmrh_pid 	mipmrh_hdr.miph_pid
#define mipmrh_seq 	mipmrh_hdr.miph_seq
#define mipmrh_errno	mipmrh_hdr.miph_errno

	struct sockaddr mipmrh_addr[0];
};
#define MIPMRH_HOA(mipmrh) ((mipmrh)->mipmrh_addr)
#define MIPMRH_PEERADDR(mipmrh)				\
    ((struct sockaddr *)((char *)(MIPMRH_HOA(mipmrh))	\
    + (MIPMRH_HOA(mipmrh))->sa_len))

struct mipm_be_hint {
	struct mip_msghdr mipmbeh_hdr;
#define mipmbeh_msglen 	mipmbeh_hdr.miph_msglen
#define mipmbeh_version mipmbeh_hdr.miph_version
#define mipmbeh_type 	mipmbeh_hdr.miph_type
#define mipmbeh_pid 	mipmbeh_hdr.miph_pid
#define mipmbeh_seq 	mipmbeh_hdr.miph_seq
#define mipmbeh_errno	mipmbeh_hdr.miph_errno

	u_int8_t mipmbeh_status;
	u_int8_t mipmbeh_reserved[3];
	struct sockaddr mipmbeh_addr[0];
};
#define MIPMBEH_PEERADDR(mipmbeh) ((mipmbeh)->mipmbeh_addr)
#define MIPMBEH_COA(mipmbeh)					\
    ((struct sockaddr *)((char *)(MIPMBEH_PEERADDR(mipmbeh))	\
    + (MIPMBEH_PEERADDR(mipmbeh))->sa_len))
#define MIPMBEH_HOA(mipmbeh)					\
    ((struct sockaddr *)((char *)(MIPMBEH_COA(mipmbeh))	\
    + (MIPMBEH_COA(mipmbeh))->sa_len))

struct mipm_dad {
	struct mip_msghdr mipmdad_hdr;
#define mipmdad_msglen 	mipmdad_hdr.miph_msglen
#define mipmdad_version mipmdad_hdr.miph_version
#define mipmdad_type 	mipmdad_hdr.miph_type
#define mipmdad_pid 	mipmdad_hdr.miph_pid
#define mipmdad_seq 	mipmdad_hdr.miph_seq
#define mipmdad_errno	mipmdad_hdr.miph_errno

	u_int16_t mipmdad_message;
/* Under 128 are issued by userland, above 128 are issued by kernel */
#define MIPM_DAD_DO		0	/* u to k */
#define MIPM_DAD_STOP		1
#define MIPM_DAD_LINKLOCAL	2	/* play dad for link local addr */
#define MIPM_DAD_SUCCESS	128	/* k to u */
#define MIPM_DAD_FAIL		129
	u_int16_t mipmdad_ifindex;
	struct in6_addr mipmdad_addr6;
};

#define MIP_VERSION	1

#define MIPM_NODETYPE_INFO	1
#define MIPM_BC_ADD		2
/*#define MIPM_BC_UPDATE		3*/
#define MIPM_BC_REMOVE		4
#define MIPM_BC_FLUSH		5
#define MIPM_BUL_ADD		6
/*#define MIPM_BUL_UPDATE		7*/
#define MIPM_BUL_REMOVE		8
#define MIPM_BUL_FLUSH		9
#define MIPM_MD_INFO		10
#define MIPM_HOME_HINT		11
#define MIPM_RR_HINT		12
#define MIPM_BE_HINT		13
#define MIPM_DAD		14

#if defined(__NetBSD__)
int mips_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
    struct mbuf *,
#if __NetBSD_Version__ >= 400000000
    struct lwp *
#else
    struct proc *
#endif /* __NetBSD__Version__ >= 400000000 */
    );
#elif defined(__OpenBSD__)
int mips_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
    struct mbuf*);
#endif /* __OpenBSD__ */
void mips_notify_home_hint(u_int16_t, struct sockaddr *, u_int16_t);
void mips_notify_rr_hint(struct sockaddr *, struct sockaddr *);
void mips_notify_be_hint(struct sockaddr *, struct sockaddr *,
    struct sockaddr *, u_int8_t);
void mips_notify_dad_result(int, struct in6_addr *, int);

#endif /* !_NET_MIPSOCK_H_ */
