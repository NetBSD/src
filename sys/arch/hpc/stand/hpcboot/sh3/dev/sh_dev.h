/* -*-C++-*-	$NetBSD: sh_dev.h,v 1.1.2.2 2002/02/28 04:09:48 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#ifndef _SH3_DEV_SH_DEV_H_
#define _SH3_DEV_SH_DEV_H_
#include <sh3/cpu/sh.h>

class SHdev {
protected:
	HpcMenuInterface *_menu;
	Console *_cons;
	void print_stack_pointer(void);
	struct intr_priority {
		const char *name;
		paddr_t reg;
		int shift;
	};

	SHdev(void);
	virtual u_int32_t _scif_reg_read(vaddr_t) = 0;
	void scif_dump(int);
	void icu_dump_priority(struct intr_priority *);

public:
	virtual ~SHdev(void) { /* NO-OP */ }

	virtual void dump(u_int8_t);
#define DUMP_CPU	0x01
#define	DUMP_DEV	0x02
#define	DUMP_COMPANION	0x04
#define	DUMP_VIDEO	0x08
};

class SH3dev : public SHdev {
private:
	typedef SHdev super;

	void hd64461_dump(platid_t &);
	void bsc_dump(void);
	void pfc_dump(void);
	void tmu_dump(void);
	void tmu_channel_dump(int, paddr_t, paddr_t, paddr_t);
	void icu_dump(void);
	void icu_priority(void);
	void icu_control(void);
	static struct intr_priority _ipr_table[];

	virtual u_int32_t _scif_reg_read(vaddr_t va) {
		return (u_int32_t)_reg_read_1(va);
	}

public:
	SH3dev(void) { /* NO-OP */ }
	virtual ~SH3dev(void) { /* NO-OP */ }

	virtual void dump(u_int8_t);
};

class SH4dev : public SHdev {
private:
	typedef SHdev super;

	static struct intr_priority _ipr_table[];

	virtual u_int32_t _scif_reg_read(vaddr_t va) {
		return (u_int32_t)_reg_read_2(va);
	}

public:
	SH4dev(void) { /* NO-OP */ }
	virtual ~SH4dev(void) { /* NO-OP */ }

	virtual void dump(u_int8_t);
	void icu_dump(void);
	void hd64465_dump(void);
	void mq100_dump(void);
};
#endif // _SH3_DEV_SH_DEV_H_
