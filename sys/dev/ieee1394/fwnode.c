/*	$NetBSD: fwnode.c,v 1.9 2002/02/11 12:32:43 augustss Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Chacon.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: fwnode.c,v 1.9 2002/02/11 12:32:43 augustss Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bus.h>

#include <dev/std/ieee1212reg.h>
#include <dev/std/sbp2reg.h>

#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/fwnodereg.h>

#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/fwnodevar.h>

static const char * const ieee1394_speeds[] = { IEEE1394_SPD_STRINGS };
static const char * const p1212_keytype_strings[] = P1212_KEYTYPE_STRINGS ;
static const char * const p1212_keyvalue_strings[] = P1212_KEYVALUE_STRINGS ;

int  fwnode_match(struct device *, struct cfdata *, void *);
void fwnode_attach(struct device *, struct device *, void *);
int  fwnode_detach(struct device *, int);
static void fwnode_configrom_input(struct ieee1394_abuf *, int);
static u_int16_t calc_crc(u_int32_t, u_int32_t *, int, int);
static int fwnode_parse_configrom (struct fwnode_softc *);
static int fwnode_parse_directory(struct fwnode_softc *, u_int32_t *,
    struct configrom_dir *);
static void fwnode_print_configrom_tree(struct fwnode_softc *,
    struct configrom_dir *, int);
static int fwnode_parse_leaf(u_int32_t *, struct configrom_data *);
static int fwnode_match_dev_type(struct fwnode_softc *,
    struct configrom_data *);
static int sbp2_print_data(struct configrom_data *);
static int sbp2_print_dir(u_int8_t);
static void sbp2_init(struct fwnode_softc *, struct fwnode_device_cap *);
static int  fwnode_print(void *, const char *);
static void fwnode_match_subdevs(struct fwnode_softc *);
static void sbp2_login(struct ieee1394_abuf *, int);
static void sbp2_login_resp(struct ieee1394_abuf *, int);

#ifdef FW_DEBUG
#define DPRINTF(x)      if (fwnodedebug) printf x
#define DPRINTFN(n,x)   if (fwnodedebug>(n)) printf x
int     fwnodedebug = 1;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct cfattach fwnode_ca = {
	sizeof(struct fwnode_softc), fwnode_match, fwnode_attach,
	fwnode_detach
};

int
fwnode_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct ieee1394_attach_args *fwa = aux;
	
	if (strcmp(fwa->name, "fwnode") == 0)
		return 1;
	return 0;
}

void
fwnode_attach(struct device *parent, struct device *self, void *aux)
{
	struct fwnode_softc *sc = (struct fwnode_softc *)self;
	struct ieee1394_softc *psc = (struct ieee1394_softc *)parent;
	struct ieee1394_attach_args *fwa = aux;
	struct ieee1394_abuf *ab;
	
	ab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK|M_ZERO);
	ab->ab_data = malloc(4, M_1394DATA, M_WAITOK);
	ab->ab_data[0] = 0;
	
	sc->sc_sc1394.sc1394_node_id = fwa->nodeid;
	memcpy(sc->sc_sc1394.sc1394_guid, fwa->uid, 8);
	sc->sc1394_read = fwa->read;
	sc->sc1394_write = fwa->write;
	sc->sc1394_inreg = fwa->inreg;
	
	TAILQ_INIT(&sc->sc_configrom_root);
	TAILQ_INIT(&sc->sc_dev_cap_head);
	
	/* XXX. Fix the fw code to use the generic routines. */
	sc->sc_sc1394.sc1394_ifinreg = psc->sc1394_ifinreg;
	sc->sc_sc1394.sc1394_ifoutput = psc->sc1394_ifoutput;
	
	printf(" Node %d: UID %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
	    sc->sc_sc1394.sc1394_node_id,
	    sc->sc_sc1394.sc1394_guid[0], sc->sc_sc1394.sc1394_guid[1],
	    sc->sc_sc1394.sc1394_guid[2], sc->sc_sc1394.sc1394_guid[3],
	    sc->sc_sc1394.sc1394_guid[4], sc->sc_sc1394.sc1394_guid[5],
	    sc->sc_sc1394.sc1394_guid[6], sc->sc_sc1394.sc1394_guid[7]);
	ab->ab_req = (struct ieee1394_softc *)sc;
	ab->ab_addr = CSR_BASE + CSR_CONFIG_ROM;
	ab->ab_length = 4;
	ab->ab_retlen = 0;
	ab->ab_cbarg = (void *)1;
	ab->ab_cb = fwnode_configrom_input;
	sc->sc1394_read(ab);
}

int
fwnode_detach(struct device *self, int flags)
{
        struct fwnode_softc *sc = (struct fwnode_softc *)self;
	struct configrom_dir *dir, *sdir;
	struct configrom_data *data;
	struct fwnode_device_cap *devcap;
	
	if (sc->sc_sc1394.sc1394_configrom &&
	    sc->sc_sc1394.sc1394_configrom_len)
		free(sc->sc_sc1394.sc1394_configrom, M_1394DATA);
	
	/* Avoid recursing. Find the bottom most node and work back. */
	dir = TAILQ_FIRST(&sc->sc_configrom_root);
	while (dir) {
		if (!TAILQ_EMPTY(&dir->subdir_root)) {
			sdir = TAILQ_FIRST(&dir->subdir_root);
			if (TAILQ_EMPTY(&sdir->subdir_root)) {
				TAILQ_REMOVE(&dir->subdir_root, sdir, dir);
				dir = sdir;
			}
			else {
				dir = sdir;
				continue;
			}
		} else {
			if (dir->parent)
				TAILQ_REMOVE(&dir->parent->subdir_root, dir,
				    dir);
			else
				TAILQ_REMOVE(&sc->sc_configrom_root, dir,
				    dir);
		}
		
		while ((data = TAILQ_FIRST(&dir->data_root))) {
			if (data->leafdata) {
				if (data->leafdata->text)
					free(data->leafdata->text, M_1394DATA);
				free(data->leafdata, M_1394DATA);
			}
			TAILQ_REMOVE(&dir->data_root, data, data);
			free(data, M_1394DATA);
		}
		sdir = dir;
		if (dir->parent) 
			dir = dir->parent;
		else
			dir = NULL;
		free(sdir, M_1394DATA);
	}
	while ((devcap = TAILQ_FIRST(&sc->sc_dev_cap_head))) {
		TAILQ_REMOVE(&sc->sc_dev_cap_head, devcap, dev_list);
		if (devcap->dev_data)
			free(devcap->dev_data, M_1394DATA);
		free(devcap, M_1394DATA);
	}
	return 0;
}

/*
 * This code is complex since it's getting called via callbacks from the
 * handlers in the 1394 chip specific code. In addition it's trying
 * to build a complete image of the ROM in memory plus a list of any immediate
 * values referenced in the ROM. This is done all here to keep the bus_read
 * logic/callback for the ROM in one place since reading the whole ROM may
 * require lots of small reads up front and building separate callback handlers
 * for each step would be even worse.
 */

static void
fwnode_configrom_input(struct ieee1394_abuf *ab, int rcode)
{
	struct fwnode_softc *sc = (struct fwnode_softc *)ab->ab_req;
	int i, len, infolen, crclen, newlen, offset, complete, val, *dirs;
	int numdirs, *tdirs;
	u_int32_t *t;
	u_int8_t type;
	
	dirs = NULL;

	if (rcode != IEEE1394_RCODE_COMPLETE) {
		DPRINTF(("Aborting configrom input, rcode: %d\n", rcode));
		goto fail;
	}
	
	/*
	 * 2 phases:
	 *
	 * Read the first quad and get size.
	 * Then read the rest of the rom.
	 */
	
	if (ab->ab_length != ab->ab_retlen) {
		DPRINTF(("%s: config rom short read. Expected :%d, received: "
		    "%d. Not attaching\n", sc->sc_sc1394.sc1394_dev.dv_xname,
		    ab->ab_length, ab->ab_retlen));
		goto fail;
	} 
	if (ab->ab_retlen % 4) {
		DPRINTF(("%s: configrom read of invalid length: %d\n",
		    sc->sc_sc1394.sc1394_dev.dv_xname, ab->ab_retlen));
		goto fail;
	}
	
	switch ((int)ab->ab_cbarg) {
	case 1:
		infolen = P1212_ROMFMT_GET_INFOLEN((ntohl(ab->ab_data[0])));
		if (infolen <= 1) {
			DPRINTF(("%s: ROM not initialized or minimal ROM: Info "
			    "length: %d\n", sc->sc_sc1394.sc1394_dev.dv_xname,
			    infolen));
			goto fail;
		}
		crclen = P1212_ROMFMT_GET_CRCLEN((ntohl(ab->ab_data[0])));
		if (crclen < infolen) {
			DPRINTF(("%s: CRC len less than info len. CRC len: %d, "
			    "Info len: %d\n", sc->sc_sc1394.sc1394_dev.dv_xname,
			    crclen, infolen));
			goto fail;
		}

		/*
		 * Make sure and at least read up to the root directory first
		 * quad. It's guarentee'd to be there per the 1212 spec.
		 */
		
		newlen = crclen;
		if (crclen == infolen)
			newlen++;
		
		/* Make sure to include the quad we're going to reread. */
		newlen++;
		newlen *= 4;
		free(ab->ab_data, M_1394DATA);
		ab->ab_data = malloc(newlen, M_1394DATA, M_WAITOK);
		
		/*
		 * Don't fill in configrom_len until a valid read is returned.
		 * Also, assign configrom to the data buffer to avoid a copy
		 * later.
		 */
		
		sc->sc_sc1394.sc1394_configrom = ab->ab_data;
		sc->sc_sc1394.sc1394_configrom_len = 0;
		memset(ab->ab_data, 0, crclen);
		ab->ab_addr = CSR_BASE + CSR_CONFIG_ROM;
		ab->ab_length = newlen;
		ab->ab_retlen = 0;
		ab->ab_cbarg = (void *)2;
		ab->ab_cb = fwnode_configrom_input;
		sc->sc1394_read(ab);
		return;
		
		/*
		 * This case can happen multiple times depending on what the ROM
		 * listed for items that need to be read.
		 */
		
	case 2:
		sc->sc_sc1394.sc1394_configrom_len = (ab->ab_retlen / 4);
		break;
	default:
		panic("Got a callback value that's impossible: %d",
		    (int)ab->ab_cbarg);
		break;
	}
	ab->ab_data = NULL;
	
	/*
	 * Now loop through it to check if all the offsets referenced are
	 * within the image stored so far. If not, get those as well.
	 */
	
	t = sc->sc_sc1394.sc1394_configrom;
	offset = P1212_ROMFMT_GET_INFOLEN((ntohl(t[0]))) + 1;
	complete = 0;
	numdirs = 0;
	newlen = 0;
	
	while (!complete) {
		
		/*
		 * Make sure the whole directory is in memory. If not, bail now
		 * and read it in.
		 */

		newlen = P1212_DIRENT_GET_LEN((ntohl(t[offset])));
		if ((offset + newlen + 1) > sc->sc_sc1394.sc1394_configrom_len) {
			newlen += offset + 1;
			newlen *= 4;
			break;
		}
		
		/*
		 * Starting with the first byte of the directory, read through
		 * and check the values found. On offsets and directories read
		 * them in if appropriate (always for offsets, if not in memory
		 * for leaf/directories).
		 */

		offset++;
		len = newlen;
		newlen = 0;
		for (i = 0; i < len; i++) {
			type = P1212_DIRENT_GET_KEYTYPE((ntohl(t[offset + i])));
			val = P1212_DIRENT_GET_VALUE((ntohl(t[offset + i])));
			switch (type) {
			case P1212_KEYTYPE_Immediate:
			case P1212_KEYTYPE_Offset:
				break;
			case P1212_KEYTYPE_Leaf:
				
				/*
				 * If a leaf is found, and it's beyond the
				 * current rom length and it's beyond the
				 * current newlen setting,
				 * then set newlen accordingly.
				 */
				
				if ((offset + i + val + 1) >
				    sc->sc_sc1394.sc1394_configrom_len) {
					if ((offset + i + val + 1) > newlen) {
						newlen = offset + i + val + 1;
					}
					break;
				}
				
				/*
				 * For leaf nodes just make sure the whole leaf
				 * length is in the buffer. There's no data
				 * inside of them that can refer to outside
				 * nodes.
				 */

				infolen =
				    P1212_DIRENT_GET_LEN((ntohl(t[offset + i +
					val])));
				if ((offset + i + val + infolen + 1) >
				    sc->sc_sc1394.sc1394_configrom_len) {
					if ((offset + i + val + infolen + 1) >
					    newlen) {
						newlen = offset + i + val +
						    infolen + 1;
					}
				}
				break;
				
			case P1212_KEYTYPE_Directory:
				
				/* Make sure the first quad is in memory. */
				
				if ((offset + i + val + 1) >
				    sc->sc_sc1394.sc1394_configrom_len) {
					if ((offset + i + val + 1) > newlen) {
						newlen = offset + i + val + 1;
					}
				}
				
				/*
				 * Can't just walk the ROM looking at type codes
				 * since these are only valid on directory
				 * entries. So save any directories
				 * we find into a queue and the bottom of the
				 * while loop will pop the last one off and walk
				 * that directory.
				 */
                
				tdirs = malloc(sizeof(int) * (numdirs + 1),
				    M_1394DATA, M_WAITOK);
				if (dirs) {
					memcpy(tdirs, dirs,
					    numdirs * sizeof(int));
					free(dirs, M_1394DATA);
				}
				dirs = tdirs;
				dirs[numdirs++] = offset + i + val;
				break;
			default:
				panic("Impossible type code: 0x%04hx",
				    (unsigned short)type);
				break;
			}
		}
		
		if (newlen) {
			newlen *= 4;
			if (dirs) 
				free(dirs, M_1394DATA);
			break;
		}
		if (dirs) {
			offset = dirs[--numdirs];
			if (!numdirs) {
				free(dirs, M_1394DATA);
				dirs = NULL;
			}
		} else
			complete = 1;
	}
	
	/* If not complete then newlen is set to a new length to read. */
	
	if (!complete) {
		free(t, M_1394DATA);
		ab->ab_data = malloc(newlen, M_1394DATA, M_WAITOK|M_ZERO);
		sc->sc_sc1394.sc1394_configrom = ab->ab_data;
		sc->sc_sc1394.sc1394_configrom_len = 0;
		ab->ab_addr = CSR_BASE + CSR_CONFIG_ROM;
		ab->ab_length = newlen;
		ab->ab_retlen = 0;
		ab->ab_cbarg = (void *)2;
		ab->ab_cb = fwnode_configrom_input;
		sc->sc1394_read(ab);
		return;
	}
	free(ab, M_1394DATA);
	fwnode_parse_configrom(sc);
	return;
	
 fail:
	/* This is only valid if len is > 0 */
	if ((sc->sc_sc1394.sc1394_configrom) &&
	    sc->sc_sc1394.sc1394_configrom_len)
		free(sc->sc_sc1394.sc1394_configrom, M_1394DATA);
	sc->sc_sc1394.sc1394_configrom = NULL;
	sc->sc_sc1394.sc1394_configrom_len = 0;
	if (dirs)
		free(dirs, M_1394DATA);
	if (ab->ab_data)
		free(ab->ab_data, M_1394DATA);
	if (ab)
		free(ab, M_1394DATA);
	return;
}

/*
 * A fairly well published reference implementation of the CRC routine had
 * a typo in it and some devices may be using it rather than the correct one
 * in calculating their ROM CRC's. To compensate an interface for generating
 * either is provided.
 *
 * len is in quadlets.
 */

static u_int16_t
calc_crc(u_int32_t crc, u_int32_t *data, int len, int broke)
{
	int shift;
	u_int32_t sum;
	int i;
	
	for (i = 0; i < len; i++) {
		for (shift = 28; shift > 0; shift -= 4) {
			sum = ((crc >> 12) ^ (ntohl(data[i]) >> shift)) &
			    0x0000000f;
			crc = (crc << 4) ^ (sum << 12) ^ (sum << 5) ^ sum;
		}
		
		
		/* The broken implementation doesn't do the last shift. */
		if (!broke) {
			sum = ((crc >> 12) ^ ntohl(data[i])) & 0x0000000f;
			crc = (crc << 4) ^ (sum << 12) ^ (sum << 5) ^ sum;
		}
	}
	return (u_int16_t)crc;
}

/*
 * Routines to parse the ROM into a tree that's usable. Also verify integrity
 * vs. the P1212 standard
 */

static int
fwnode_parse_configrom(struct fwnode_softc *sc)
{
	struct configrom_dir *root;
#ifdef FW_DEBUG
	int i;
#endif
	int next;
	u_int16_t crc, crc1, romcrc;
	u_int32_t *t;
	
	t = sc->sc_sc1394.sc1394_configrom;
	
#ifdef FW_DEBUG
	printf("%s: Config rom dump:\n", sc->sc_sc1394.sc1394_dev.dv_xname);
	for (i = 0; i < sc->sc_sc1394.sc1394_configrom_len; i++) {
		if ((i % 4) == 0) {
			if (i)
				printf("\n");
			printf("%s: 0x%02hx: ",
			    sc->sc_sc1394.sc1394_dev.dv_xname, (short)(4 * i));
		}
		printf("0x%08x ", ntohl(t[i]));
		
	}
	printf("\n");
#endif
	
	if (ntohl(t[1]) != IEEE1394_SIGNATURE) {
		DPRINTF(("%s: Invalid signature found in rom. Expected: "
		    "IEEE1394_SIGNATURE, received: 0x%08x\n",
		    sc->sc_sc1394.sc1394_dev.dv_xname, ntohl(t[1])));
		return 1;
	}
	
	/* Calculate both a good and known bad crc. */
	
	/* CRC's are calculated from everything except the first quad. */
	
	crc = calc_crc(0, &t[1], P1212_ROMFMT_GET_CRCLEN((ntohl(t[0]))), 0);
	crc1 = calc_crc(0, &t[1], P1212_ROMFMT_GET_CRCLEN((ntohl(t[0]))), 1);
	
	/* XXX: Need macro's to extract these. */
	sc->sc_sc1394.sc1394_max_receive = (0x00000001 <<
	    (((ntohl(t[2]) & 0x0000f000) >> 12) + 1));
	sc->sc_sc1394.sc1394_link_speed =
	    ntohl(t[2]) & 0x00000007;
	printf("%s: Link Speed: %s, max_rec: %d bytes\n",
	    sc->sc_sc1394.sc1394_dev.dv_xname, 
	    ieee1394_speeds[sc->sc_sc1394.sc1394_link_speed],
	    sc->sc_sc1394.sc1394_max_receive);
	romcrc = P1212_ROMFMT_GET_CRC((ntohl(t[0])));
	if (crc != romcrc)
		if (crc1 != romcrc) {
			DPRINTF(("%s: Invalid ROM: CRC: 0x%04hx, Calculated "
			    "CRC: 0x%04hx, CRC1: 0x%04hx\n",
			    sc->sc_sc1394.sc1394_dev.dv_xname,
			    (unsigned short)romcrc, (unsigned short)crc,
			    (unsigned short)crc1));
			return 1;
		}
	
	/* Now, walk the ROM. */
	
	/* Get the initlal offset for the root dir. */
	
	next = P1212_ROMFMT_GET_INFOLEN((ntohl(t[0]))) + 1;
	root = malloc(sizeof(struct configrom_dir), M_1394DATA, M_WAITOK);    
	TAILQ_INSERT_TAIL(&sc->sc_configrom_root, root, dir);
	root->dir_type = 0;
	root->offset = next;
	root->refs = 1;
	root->parent = NULL;
	if (!fwnode_parse_directory(sc, &t[next], root)) {
#ifdef FW_DEBUG
		fwnode_print_configrom_tree(sc, root, 0);
#endif
		fwnode_match_subdevs(sc);
	} else
		return 1;
	return 0;
}

static int
fwnode_parse_directory(struct fwnode_softc *sc, u_int32_t *addr,
    struct configrom_dir *head)
{
	struct configrom_dir *dir;
	struct configrom_data *data;
	u_int16_t len, crclen, crc, crc1, romcrc;
	u_int8_t type, val;
	int i;
	
	/* XXX: This whole piece needs reworking. Complex modern p1212
	   rom's can have multiple references to the same directory. Need
	   to add real ref counting, detection, post processing (to prune
	   duplicated refs), and a complete tree validation suite. The only
	   checking done at this level is the CRC checks, but there are hard
	   requirements for what can be in a directory at which level which
	   also needs to be verified. Plus all the recursive routines should be
	   unrolled since they're mostly tail recursive anyways.
	*/
	
	TAILQ_INIT(&head->data_root);
	TAILQ_INIT(&head->subdir_root);
	
	len = P1212_DIRENT_GET_LEN((ntohl(addr[0])));
	romcrc = P1212_DIRENT_GET_CRC((ntohl(addr[0])));
	
	crc = calc_crc(0, &addr[1], len, 0);
	crc1 = calc_crc(0, &addr[1], len, 1);
	if (crc != romcrc)
		if (crc1 != romcrc) {
			DPRINTF(("Invalid ROM: CRC: 0x%04hx, Calculated CRC: "
			    "0x%04hx, CRC1: 0x%04hx\n",
			    (unsigned short)romcrc, (unsigned short)crc,
			    (unsigned short)crc1));
			return 1;
		}
	addr++;
	
	for (i = 0; i < len; i++) {
		type = P1212_DIRENT_GET_KEYTYPE((ntohl(addr[i])));
		val = P1212_DIRENT_GET_KEYVALUE((ntohl(addr[i])));
		
		switch (type) {
		case P1212_KEYTYPE_Immediate:
		case P1212_KEYTYPE_Offset:
		case P1212_KEYTYPE_Leaf:
			data = malloc (sizeof(struct configrom_data),
			    M_1394DATA, M_WAITOK);
			data->key_type = type;
			data->key_value = val;
			data->key = P1212_DIRENT_GET_KEY((ntohl(addr[i])));
			data->val = P1212_DIRENT_GET_VALUE((ntohl(addr[i])));
			data->leafdata = NULL;
			TAILQ_INSERT_TAIL(&head->data_root, data, data);
			
			if (type != P1212_KEYTYPE_Leaf)
				if (fwnode_match_dev_type(sc, data))
					return 1;
			if (type == P1212_KEYTYPE_Leaf) {
				crclen = P1212_DIRENT_GET_LEN((ntohl(addr[i +
				    data->val])));
				romcrc = P1212_DIRENT_GET_CRC((ntohl(addr[i +
				    data->val])));
				crc = calc_crc(0, &addr[i + data->val + 1],
				    crclen, 0);
				crc1 = calc_crc(0, &addr[i + data->val + 1],
				    crclen, 1);
				if (crc != romcrc)
					if (crc1 != romcrc) {
						DPRINTF(("Invalid ROM: CRC: "
						    "0x%04hx, Calculated CRC: "
						    "0x%04hx, CRC1: 0x%04hx\n",
						    (unsigned short)romcrc,
						    (unsigned short)crc,
						    (unsigned short)crc1));
						return 1;
					}
				if (fwnode_parse_leaf(addr + i + data->val,
				    data))
					return 1;
			}
			break;
			
		case P1212_KEYTYPE_Directory:
			
			dir = malloc(sizeof(struct configrom_dir), M_1394DATA,
			    M_WAITOK);
			dir->parent = head;
			dir->dir_type = val;
			dir->offset = &addr[i] - sc->sc_sc1394.sc1394_configrom;
			TAILQ_INSERT_TAIL(&head->subdir_root, dir, dir);
			val = P1212_DIRENT_GET_VALUE((ntohl(addr[i])));
			if (fwnode_parse_directory(sc, &addr[i + val], dir))
				return 1;
			break;
		default:
			panic("Impossible type code: 0x%04hx",
			    (unsigned short)type);
			break;
		}
	}
	return 0;
}

int fwnode_parse_leaf(u_int32_t *addr, struct configrom_data *data)
{
	struct configrom_leafdata *l;
	int len, version;
	
	l = malloc(sizeof(struct configrom_leafdata), M_1394DATA,
	    M_WAITOK|M_ZERO);
	data->leafdata = l;
	l->desc_type = (ntohl(addr[1]) >> 24) & 0xff;
	l->spec_id = ntohl(addr[1]) & 0xffffff;
	len = P1212_DIRENT_GET_LEN((ntohl(addr[0])));
	
	switch (l->desc_type) {
	case 0:
		if (l->spec_id) {
			DPRINTF(("Invalid text spec id 0x%08x in descriptor "
			    "starting at: 0x%08x\n", l->spec_id,
			    ntohl(addr[0])));
			return 1;
		}
		l->char_width = (ntohl(addr[2]) >> 28) & 0xf;
		
		if (l->char_width == 8) {
			DPRINTF(("Invalid character width 0x%04hx in text "
			    "descriptor starting at: 0x%08x\n",
			    (unsigned short)l->char_width, ntohl(addr[0])));
			return 1;
		}
		l->char_set = (ntohl(addr[2]) >> 16) & 0xfff;
		l->char_lang = ntohl(addr[2]) & 0xffff;
		if ((!l->char_set && l->char_lang) ||
		    (l->char_set && (l->char_lang & 0x8000))) {
			DPRINTF(("Invalid character set 0x%04hx or character "
			    "language 0x%04hx in text descriptor starting at: "
			    "0x%08x\n", (unsigned short)l->char_set,
			    (unsigned short)l->char_lang, ntohl(addr[0])));
			return 1;
		}
		
		/* First 2 quads contain the data filled in above. Trim them. */
		len -= 2;
		
		/* No guarentee this is zero terminated so account for that. */
		l->datalen = (len * 4) + 1;
		l->text = malloc(l->datalen, M_1394DATA, M_WAITOK);
		
		/* Offset 3 of a text descriptor begins the text. */
		memcpy (l->text, &addr[3], l->datalen);
		l->text[l->datalen - 1] = 0x0;
		break;
	case 1:
		if (l->spec_id) {
			DPRINTF(("Invalid icon spec id 0x%08x in descriptor "
			    "starting at: 0x%08x\n", l->spec_id,
			    ntohl(addr[0])));
			return 1;
		}
		version = ntohl(addr[2]) & 0xffffff;
		if (version) {
			DPRINTF(("Invalid icon version 0x%08x in descriptor "
			    "starting at: 0x%08x\n", version, ntohl(addr[0])));
			return 1;
		}
		
		/* First 2 quads contain the data filled in above. Trim them. */
		len -= 2;
		
		l->datalen = len * 4;
		l->text = malloc(l->datalen, M_1394DATA, M_WAITOK);        
		/* Offset 3 of a icon descriptor begins the data. */
		memcpy(l->text, &addr[3], l->datalen);
		break;
	default:
		
		/* Trim the first quad off since it contains type and spec id. */
		len--;
		l->datalen = len * 4;
		l->text = malloc(l->datalen, M_1394DATA, M_WAITOK);
		/* Offset 2 of a descriptor begins the data. */
		memcpy(l->text, &addr[2], l->datalen);
		break;
	}
	return 0;
}

static int
fwnode_match_dev_type(struct fwnode_softc *sc, struct configrom_data *data)
{
	/* XXX: This whole structure needs to be gut'd and redone.
	   Only after reading in the whole tree should sections of the tree
	   be passed along to config_found. */
	
	struct fwnode_device_cap *devcap;
	int done, found_sbp2;
	
	done = found_sbp2 = 0;
	
	switch (data->key_value) {
	case P1212_KEYVALUE_Unit_Spec_Id:
		if (data->val == SBP2_UNIT_SPEC_ID)
			found_sbp2 = 1;
		break;
	case P1212_KEYVALUE_Unit_Sw_Version:
		if (data->val == SBP2_UNIT_SW_VERSION)
			found_sbp2 = 1;
		break;
	case P1212_KEYVALUE_Unit_Dependent_Info:
		if (data->key_type == P1212_KEYTYPE_Offset) 
			found_sbp2 = 1;
	default:
		break;
	}
	if (found_sbp2) {
		TAILQ_FOREACH(devcap, &sc->sc_dev_cap_head, dev_list) {
			
			if ((devcap->dev_type == DEVTYPE_SBP2) && found_sbp2) { 
				if (!devcap->dev_valid) {
					done = 1;
					devcap->dev_valid = 1;
				} else {
					DPRINTF(("found_sbp2: 0x%08x\n",
					    data->val));
					devcap->dev_data =
					    malloc(4, M_1394DATA, M_WAITOK);
					((u_int32_t *)devcap->dev_data)[0] =
					    data->val;
					done = 1;
				}
			}
		}
		
		/* If one wasn't found get a struct started. */
		if (!done) {
			devcap = malloc(sizeof(*devcap), M_1394DATA, M_WAITOK);
			devcap->dev_valid = 0;
			devcap->dev_spec = 0;
			devcap->dev_info = 0;
			devcap->dev_data = NULL;
			if (found_sbp2) {
				devcap->dev_type = DEVTYPE_SBP2;
				devcap->dev_print_data = sbp2_print_data;
				devcap->dev_print_dir = sbp2_print_dir;
				devcap->dev_init = sbp2_init;
			}
			TAILQ_INSERT_TAIL(&sc->sc_dev_cap_head, devcap,
			    dev_list);
		}
	}
	return 0;
}

static void
fwnode_print_configrom_tree(struct fwnode_softc *sc, struct configrom_dir *dir,
    int indent)
{
	int i, found;
	struct configrom_data *data;
	struct configrom_dir *subdir;
	struct configrom_leafdata *leaf;
	struct fwnode_device_cap *devcap;
	char *space;
	
	/* Set the indent string up. 4 spaces per level. */
	space = malloc((indent * 4) + 1, M_1394DATA, M_WAITOK);
	for (i = 0; i < (indent * 4); i++)
		space[i] = 0x20;
	space[indent * 4] = 0;
	
	if (dir->dir_type) {
		found = 0;
		TAILQ_FOREACH(devcap, &sc->sc_dev_cap_head, dev_list) {
			if (devcap->dev_print_dir(dir->dir_type)) {
				found = 1;
				break;
			}
		}
		if (!found) {
			printf("%sUnit ", space);
		}
	} else
		printf("Root ");
	printf("directory:\n\n");
	TAILQ_FOREACH(data, &dir->data_root, data) {
		found = 0;
		printf("%s", space);
		
		/*
		 * Try any capable devices first. 
		 * If none match just give up and print out the value.
		 *
		 */
		
		TAILQ_FOREACH(devcap, &sc->sc_dev_cap_head, dev_list) {
			if (devcap->dev_print_data(data)) {
				found = 1;
				break;
			}
		}
		if (found)
			continue;
		if (data->key_value >= (sizeof(p1212_keyvalue_strings) /
		    sizeof(char *))) 
			printf("Unknown type 0x%04hx: ",
			    (unsigned short)data->key_value);
		else
			printf("%s: ", p1212_keyvalue_strings[data->key_value]);
		if (data->key_value != P1212_KEYVALUE_Textual_Descriptor)
			printf("0x%08x\n", data->val);
		else {
			if ((data->key_type != P1212_KEYTYPE_Leaf) ||
			    (data->leafdata == NULL)) 
				panic("Invalid data node in configrom tree\n");
			leaf = data->leafdata;
			switch (leaf->desc_type) {
			case 0:
				if (leaf->char_width || leaf->char_set ||
				    leaf->char_lang)
					printf("Not ASCII printable: width: "
					    "0x%04hx, char set: "
					    "0x%04hx, lang: 0x%04hx\n",
					    leaf->char_width, leaf->char_set,
					    leaf->char_lang);
				else
					printf("%s\n", leaf->text);
				break;
			case 1:
				printf("Icon data (not printing)\n");
				break;
			default:
				printf("Unknown descriptor type: 0x%04hx\n",
				    data->leafdata->desc_type);
				break;
			}
		}
	}
	TAILQ_FOREACH(subdir, &dir->subdir_root, dir) {
		printf("\n");
		fwnode_print_configrom_tree(sc, subdir, (indent + 1));
	}
	free(space, M_1394DATA);
}

/* XXX: All the sbp2 routines are a complete hack simply to probe a device for
   the moment to make sure all the other code is good. This should all be gutted
   out to it's own file and properly abstracted.
*/

static int
sbp2_print_data(struct configrom_data *data)
{
	switch (data->key_value) {
	case SBP2_KEYVALUE_Command_Set:
		printf("SBP2 Command Set: ");
		if (data->val == 0x104d8)
			printf("SCSI 2\n");
		else
			printf("0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Unit_Characteristics:
		printf("SBP2 Unit Characteristics: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Command_Set_Revision:
		printf("SBP2 Command Set Revision: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Command_Set_Spec_Id:
		printf("SBP2 Command Set Spec Id: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Firmware_Revision:
		printf("SBP2 Firmware Revision: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Reconnect_Timeout:
		printf("SBP2 Reconnect Timeout: 0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Unit_Unique_Id:
		printf("SBP2 Unit Unique Id: 0x%08x\n", data->val);
		break;
	case P1212_KEYVALUE_Unit_Dependent_Info:
		if (data->key_type == P1212_KEYTYPE_Immediate) 
			printf("SBP2 Logical Unit Number: 0x%08x\n", data->val);
		else if (data->key_type == P1212_KEYTYPE_Offset) 
			printf("SBP2 Management Agent: 0x%08x\n", data->val);
		break;
	default:
		return 0;
	}
	return 1;
}

static int
sbp2_print_dir(u_int8_t dir_type)
{
	switch(dir_type) {
	case SBP2_KEYVALUE_Logical_Unit_Directory:
		printf("Logical Unit ");
		break;
	default:
		return 0;
	}
	return 1;
}

static void
sbp2_init(struct fwnode_softc *sc, struct fwnode_device_cap *devcap)
{
	struct ieee1394_abuf *ab, *ab2;
	u_int32_t loc = ((int32_t *)devcap->dev_data)[0];
	
	ab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK|M_ZERO);
	ab->ab_data = malloc(8, M_1394DATA, M_WAITOK|M_ZERO);
	ab2 = malloc(sizeof(struct ieee1394_abuf), M_1394DATA, M_WAITOK|M_ZERO);
	
	loc *= 4;
	ab->ab_length = 8;
	ab->ab_req = (struct ieee1394_softc *)sc;
	ab->ab_retlen = 0;
	ab->ab_cb = NULL;
	ab->ab_cbarg = NULL;
	ab->ab_addr = CSR_BASE + loc;
	ab->ab_data[0] = htonl(0x4000);
	ab->ab_data[1] = 0;
	ab->ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	
	ab2->ab_length = 32;
	ab2->ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
	ab2->ab_retlen = 0;
	ab2->ab_data = NULL;
	ab2->ab_addr = 0x0000400000000000;
	ab2->ab_cb = sbp2_login;
	ab2->ab_cbarg = devcap;
	ab2->ab_req = (struct ieee1394_softc *)sc;
	
	sc->sc1394_inreg(ab2, FALSE);
	sc->sc1394_write(ab);
	return;
}

static void
sbp2_login(struct ieee1394_abuf *ab, int rcode)
{
	struct fwnode_softc *sc = (struct fwnode_softc *)ab->ab_req;
	/*    struct fwnode_device_cap *devcap = ab->ab_cbarg;*/
	struct ieee1394_abuf *statab, *respab;
	
	/* Got a read so allocate the buffer and write out the response. */
	
	if (rcode) {
#ifdef FW_DEBUG
		printf ("sbp2_login: Bad return code: %d\n", rcode);
#endif
		if (ab->ab_data)
			free (ab->ab_data, M_1394DATA);
		free(ab, M_1394DATA);
	}
	
	ab->ab_data = malloc(32, M_1394DATA, M_WAITOK);
	respab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA,
	    M_WAITOK|M_ZERO);
	statab = malloc(sizeof(struct ieee1394_abuf), M_1394DATA,
	    M_WAITOK|M_ZERO);
	
	statab->ab_length = 32;
	statab->ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	statab->ab_retlen = 0;
	statab->ab_data = NULL;
	statab->ab_addr = 0x0000400000000030;
	statab->ab_cb = sbp2_login_resp;
	statab->ab_cbarg = ab->ab_cbarg;
	statab->ab_req = ab->ab_req;
	
	sc->sc1394_inreg(statab, TRUE);
	
	respab->ab_length = 16;
	respab->ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	respab->ab_retlen = 0;
	respab->ab_data = NULL;
	respab->ab_addr = 0x0000400000000020;
	respab->ab_cb = sbp2_login_resp;
	respab->ab_cbarg = ab->ab_cbarg;
	respab->ab_req = ab->ab_req;
	
	sc->sc1394_inreg(respab, TRUE);
	
	memset(ab->ab_data, 0, 32);
	ab->ab_data[2] = htonl(0x4000);
	ab->ab_data[3] = htonl(0x20);
	ab->ab_data[4] = htonl(0x90000000);
	ab->ab_data[5] = htonl(0x00000010);
	ab->ab_data[6] = htonl(0x4000);
	ab->ab_data[7] = htonl(0x30);
	ab->ab_retlen = 0;
	ab->ab_cb = NULL;
	ab->ab_cbarg = NULL;
	ab->ab_tcode = IEEE1394_TCODE_READ_RESP_BLOCK;
	ab->ab_length = 32;
	
	sc->sc1394_write(ab);
}

static void
sbp2_login_resp(struct ieee1394_abuf *ab, int rcode)
{
	/*    struct fwnode_device_cap *devcap = (struct fwnode_device_cap *)ab->ab_cbarg;*/
#ifdef FW_DEBUG
	int i;
#endif
	
	if (rcode) {
		DPRINTF(("Bad return code: %d\n", rcode));
		if (ab->ab_data)
			free(ab->ab_data, M_1394DATA);
		free(ab, M_1394DATA);
	}
	
	DPRINTF(("csr: 0x%016qx\n", ab->ab_addr));
#ifdef FW_DEBUG
	for (i = 0; i < (ab->ab_retlen / 4); i++) 
		printf("%d: 0x%08x\n", i, ntohl(ab->ab_data[i]));
#endif
	
	free(ab->ab_data, M_1394DATA);
	free(ab, M_1394DATA);
}

static void
fwnode_match_subdevs(struct fwnode_softc *sc)
{
	struct fwnode_device_cap *devcap;
	
	TAILQ_FOREACH(devcap, &sc->sc_dev_cap_head, dev_list) {
		switch(devcap->dev_type) {
		case DEVTYPE_UNKNOWN:
			DPRINTF(("%s: Unknown device %d\n",
			    sc->sc_sc1394.sc1394_dev.dv_xname,
			    devcap->dev_spec));
			break;
		case DEVTYPE_SBP2:
			devcap->dev_init(sc, devcap);
			break;
		default:
			panic("Unknown devcap type: %d", devcap->dev_type);
			fwnode_print(NULL, NULL);
			break;
		}
	}
}

static int
fwnode_print(void *aux, const char *pnp)
{
	char *name = aux;
	
	if (pnp)
		printf("%s at %s", name, pnp);
	
	return UNCONF;
}
