/*	$NetBSD: ieee1394var.h,v 1.19 2003/06/28 14:21:36 darrenr Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_IEEE1394_IEEE1394VAR_H_
#define	_DEV_IEEE1394_IEEE1394VAR_H_

struct ieee1394_softc;
struct ieee1394_node;

struct fwiso_header;		/* XXX */
struct selinfo;			/* XXX */

/* for isochronous receive */
typedef void *ieee1394_ir_tag_t;

/* for isochronous transmit */
typedef void *ieee1394_it_tag_t;
struct ieee1394_it_datalist;

/* These buffers have no reference counting.  It is assumed that 
 * the upper level buffer (struct buf or struct mbuf) will have the
 * requisite reference counting.
 */
struct ieee1394_abuf {
	struct ieee1394_softc *ab_req;		/* requestee */
	u_int32_t *ab_data;
	struct uio *ab_uio;
	u_int64_t ab_addr;
	u_int64_t ab_retaddr;
	u_int8_t ab_tcode;
	u_int8_t ab_tlabel;
	u_int32_t ab_length; 
	u_int32_t ab_retlen;			/* length returned from read. */
	u_int32_t ab_retries;
	u_int8_t ab_subok;
	
	void (*ab_cb)(struct ieee1394_abuf *, int);
	void *ab_cbarg;
};

struct ieee1394_callbacks {
	void (*sc1394_reset)(struct ieee1394_softc *, void *);
	void *sc1394_resetarg;
	int (*sc1394_read)(struct ieee1394_abuf *);
	int (*sc1394_write)(struct ieee1394_abuf *);
	int (*sc1394_inreg)(struct ieee1394_abuf *, int);
	int (*sc1394_unreg)(struct ieee1394_abuf *, int);
};

struct ieee1394_attach_args {
	char name[7]; 
	u_int8_t uid[8];
	u_int16_t nodeid;
};    

struct ieee1394_softc {
	struct device sc1394_dev;
	struct device *sc1394_if; /* Move to fwohci level. */
	struct device *sc1394_iso; /* Move to fwohci level. */
	void *sc1394_isoarg;	/* XXX */
	
	struct ieee1394_callbacks sc1394_callback; /* Nuke probably. */
	u_int32_t *sc1394_configrom;
	u_int32_t sc1394_configrom_len;  /* quadlets. */
	u_int32_t sc1394_max_receive;
	u_int8_t sc1394_guid[8];
	u_int8_t sc1394_link_speed;	/* IEEE1394_SPD_* */
	u_int16_t sc1394_node_id;	/* my node id in network order */
	
	int (*sc1394_ifoutput)(struct device *, struct mbuf *,
	    void (*)(struct device *, struct mbuf *)); /* Nuke. */
	int (*sc1394_ifinreg)(struct device *, u_int32_t, u_int32_t,
	    void (*)(struct device *, struct mbuf *)); /* Nuke */
	int (*sc1394_ifsetiso)(struct device *, u_int32_t, u_int32_t, u_int32_t,
	    void (*)(struct device *, struct mbuf *)); /* Nuke */

	/* for isochronous receive */
	ieee1394_ir_tag_t (*sc1394_ir_open)(struct device *, int, int,
	    int, int, int);
	int (*sc1394_ir_close)(struct device *, ieee1394_ir_tag_t);
	int (*sc1394_ir_read)(struct device *, ieee1394_ir_tag_t,
	    struct uio *, int, int);
	int (*sc1394_ir_wait)(struct device *, ieee1394_ir_tag_t,
	    void *, char *);
	int (*sc1394_ir_select)(struct device *, ieee1394_ir_tag_t,
	    struct lwp *);

	/* for isochronous transmission */
	ieee1394_it_tag_t (*sc1394_it_open)(struct device *, int, int, int);
	int (*sc1394_it_writedata)(struct device *, int, int,
	    struct ieee1394_it_datalist *, int);
	int (*sc1394_it_close)(struct device *, ieee1394_it_tag_t);
	
	LIST_ENTRY(ieee1394_softc) sc1394_node;
};

struct ieee1394_node {
	struct device node_dev;
	
	struct ieee1394_softc *node_sc;	/* owning bus */
	u_int32_t *node_configrom;
	size_t node_configrom_len;
};

int ieee1394_init __P((struct ieee1394_softc *));


struct ieee1394_it_func_t {
	int (*ieee1394_it_writedata)(ieee1394_it_tag_t, int,
	    struct ieee1394_it_datalist *);
};


union ieee1394_it_data {
	u_int32_t id_data[2];
	u_int8_t *id_addr;
};

#define IEEE1394_IT_CMD_NUM	4

struct ieee1394_it_datalist {
	u_int16_t it_cmd[IEEE1394_IT_CMD_NUM];
	union ieee1394_it_data it_u[IEEE1394_IT_CMD_NUM];
};

#define IEEE1394_IT_CMD_MASK	0xf000
#define IEEE1394_IT_CMD_NOP	0x0000
#define IEEE1394_IT_CMD_IMMED	0x1000
#define IEEE1394_IT_CMD_PTR	0x2000
#define IEEE1394_IT_CMD_SIZE	0x0fff


#define	IEEE1394_ARGTYPE_PTR	0
#define	IEEE1394_ARGTYPE_MBUF	1

/*
 * some definitions for 2nd, 3rd and 4th arguments of
 * sc1394_devsetiso().
 */
#define IEEE1394_ISO_CHANNEL_ANY	64
#define IEEE1394_ISO_CHANNEL_MASK	0x3f
#define IEEE1394_ISO_TAG0		0x01
#define IEEE1394_ISO_TAG1		0x02
#define IEEE1394_ISO_TAG2		0x04
#define IEEE1394_ISO_TAG3		0x08
#define IEEE1394_ISO_DIR_READ		0
#define IEEE1394_ISO_DIR_WRITE		1

#define IEEE1394_IR_NEEDHEADER		0x01
#define IEEE1394_IR_TRIGGER_CIP_SYNC	0x02
#define IEEE1394_IR_SHORTDELAY		0x04

#endif	/* _DEV_IEEE1394_IEEE1394VAR_H_ */
