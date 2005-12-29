/*	$NetBSD: azalia_codec.c,v 1.4.2.2 2005/12/29 19:33:40 riz Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by TAMURA Kent
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: azalia_codec.c,v 1.4.2.2 2005/12/29 19:33:40 riz Exp $");

#include <dev/pci/azalia.h>

static int	azalia_codec_add_dacgroup(codec_t *, int, uint32_t);
static int	azalia_codec_find_pin(const codec_t *, int, int, uint32_t);
static int	azalia_codec_find_dac(const codec_t *, int, int);

int
azalia_codec_init_dacgroup(codec_t *this)
{
	int i, j, assoc, group;

	/*
	 * grouping DACs
	 *   [0] the lowest assoc DACs
	 *   [1] the lowest assoc digital outputs
	 *   [2] the 2nd assoc DACs
	 *      :
	 */
	this->ndacgroups = 0;
	for (assoc = 0; assoc < CORB_CD_ASSOCIATION_MAX; assoc++) {
		azalia_codec_add_dacgroup(this, assoc, 0);
		azalia_codec_add_dacgroup(this, assoc, COP_AWCAP_DIGITAL);
	}

	/* find DACs which do not connect with any pins by default */
	DPRINTF(("%s: find non-connected DACs\n", __func__));
	FOR_EACH_WIDGET(this, i) {
		boolean_t found;

		if (this->w[i].type != COP_AWTYPE_AUDIO_OUTPUT)
			continue;
		found = FALSE;
		for (group = 0; group < this->ndacgroups; group++) {
			for (j = 0; j < this->dacgroups[group].nconv; j++) {
				if (i == this->dacgroups[group].conv[j]) {
					found = TRUE;
					group = this->ndacgroups;
					break;
				}
			}
		}
		if (found)
			continue;
		if (this->ndacgroups >= 32)
			break;
		this->dacgroups[this->ndacgroups].nconv = 1;
		this->dacgroups[this->ndacgroups].conv[0] = i;
		this->ndacgroups++;
	}
#ifdef AZALIA_DEBUG
	for (group = 0; group < this->ndacgroups; group++) {
		DPRINTF(("%s: dacgroup[%d]:", __func__, group));
		for (j = 0; j < this->dacgroups[group].nconv; j++) {
			DPRINTF((" %2.2x", this->dacgroups[group].conv[j]));
		}
		DPRINTF(("\n"));
	}
#endif
	this->cur_dac = 0;

	/* enumerate ADCs */
	this->nadcs = 0;
	FOR_EACH_WIDGET(this, i) {
		if (this->w[i].type != COP_AWTYPE_AUDIO_INPUT)
			continue;
		this->adcs[this->nadcs++] = i;
		if (this->nadcs >= 32)
			break;
	}
	this->cur_adc = 0;
	return 0;
}

static int
azalia_codec_add_dacgroup(codec_t *this, int assoc, uint32_t digital)
{
	int i, j, n, dac, seq;

	n = 0;
	for (seq = 0 ; seq < CORB_CD_SEQUENCE_MAX; seq++) {
		i = azalia_codec_find_pin(this, assoc, seq, digital);
		if (i < 0)
			continue;
		dac = azalia_codec_find_dac(this, i, 0);
		if (dac < 0)
			continue;
		/* duplication check */
		for (j = 0; j < n; j++) {
			if (this->dacgroups[this->ndacgroups].conv[j] == dac)
				break;
		}
		if (j < n)	/* this group already has <dac> */
			continue;
		this->dacgroups[this->ndacgroups].conv[n++] = dac;
		DPRINTF(("%s: assoc=%d seq=%d ==> g=%d n=%d\n",
			 __func__, assoc, seq, this->ndacgroups, n-1));
	}
	if (n <= 0)		/* no such DACs */
		return 0;
	this->dacgroups[this->ndacgroups].nconv = n;

	/* check if the same combination is already registered */
	for (i = 0; i < this->ndacgroups; i++) {
		if (n != this->dacgroups[i].nconv)
			continue;
		for (j = 0; j < n; j++) {
			if (this->dacgroups[this->ndacgroups].conv[j] !=
			    this->dacgroups[i].conv[j])
				break;
		}
		if (j >= n) /* matched */
			return 0;
	}
	/* found no equivalent group */
	this->ndacgroups++;
	return 0;
}

static int
azalia_codec_find_pin(const codec_t *this, int assoc, int seq, uint32_t digital)
{
	int i;

	FOR_EACH_WIDGET(this, i) {
		if (this->w[i].type != COP_AWTYPE_PIN_COMPLEX)
			continue;
		if ((this->w[i].d.pin.cap & COP_PINCAP_OUTPUT) == 0)
			continue;
		if ((this->w[i].widgetcap & COP_AWCAP_DIGITAL) != digital)
			continue;
		if (this->w[i].d.pin.association != assoc)
			continue;
		if (this->w[i].d.pin.sequence == seq) {
			return i;
		}
	}
	return -1;
}

static int
azalia_codec_find_dac(const codec_t *this, int index, int depth)
{
	const widget_t *w;
	int i, j, ret;

	w = &this->w[index];
	if (w->type == COP_AWTYPE_AUDIO_OUTPUT) {
		DPRINTF(("%s: DAC: nid=0x%x index=%d\n",
		    __func__, w->nid, index));
		return index;
	}
	if (++depth > 50) {
		return -1;
	}
	if (w->selected >= 0) {
		j = w->connections[w->selected];
		ret = azalia_codec_find_dac(this, j, depth);
		if (ret >= 0) {
			DPRINTF(("%s: DAC path: nid=0x%x index=%d\n",
			    __func__, w->nid, index));
			return ret;
		}
	}
	for (i = 0; i < w->nconnections; i++) {
		j = w->connections[i];
		ret = azalia_codec_find_dac(this, j, depth);
		if (ret >= 0) {
			DPRINTF(("%s: DAC path: nid=0x%x index=%d\n",
			    __func__, w->nid, index));
			return ret;
		}
	}
	return -1;
}

