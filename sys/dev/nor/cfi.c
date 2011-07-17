/*	$NetBSD: cfi.c,v 1.2 2011/07/17 00:52:42 dyoung Exp $	*/

#include "opt_nor.h"
#include "opt_flash.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cfi.c,v 1.2 2011/07/17 00:52:42 dyoung Exp $"); 

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/endian.h>

#include <sys/bus.h>
        
#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>
#include <dev/nor/cfi_0002.h>


static bool cfi_chip_query(struct cfi * const);
static int  cfi_scan_media(device_t self, struct nor_chip *chip);
static void cfi_init(device_t);
static void cfi_select(device_t, bool);
static void cfi_read_1(device_t, flash_off_t, uint8_t *);
static void cfi_read_2(device_t, flash_off_t, uint16_t *);
static void cfi_read_4(device_t, flash_off_t, uint32_t *);
static void cfi_read_buf_1(device_t, flash_off_t, uint8_t *, size_t);
static void cfi_read_buf_2(device_t, flash_off_t, uint16_t *, size_t);
static void cfi_read_buf_4(device_t, flash_off_t, uint32_t *, size_t);
static void cfi_write_1(device_t, flash_off_t, uint8_t);
static void cfi_write_2(device_t, flash_off_t, uint16_t);
static void cfi_write_4(device_t, flash_off_t, uint32_t);
static void cfi_write_buf_1(device_t, flash_off_t, const uint8_t *, size_t);
static void cfi_write_buf_2(device_t, flash_off_t, const uint16_t *, size_t);
static void cfi_write_buf_4(device_t, flash_off_t, const uint32_t *, size_t);
static bool cfi_jedec_id(struct cfi * const);


/*
 * NOTE these opmode tables are informed by "Table 1. CFI Query Read"
 * in Intel "Common Flash Interface (CFI) and Command Sets"
 * Application Note 646, April 2000
 *
 * The byte ordering of the signature string here varies from that table
 * because of discrepancy in observed behavior, for the case:
 *	- x16 device operating in 16-bit mode
 * Similar discrepancy is expected (but not verified) for the case:
 *	- x32 device operating in 32-bit mode
 * so the ordering is changed here for that case also.
 *
 * XXX down-sized, interleaved & multi-chip opmodes not yet supported
 */

/* 1-byte access */
static const struct cfi_opmodes cfi_opmodes_1[] = {
	{ 0, 0, 0, 0x10,  3, "QRY", "x8 device operating in 8-bit mode" },
};

/* 2-byte access */
static const struct cfi_opmodes cfi_opmodes_2[] = {
	{ 1, 1, 0, 0x20,  6, "\0Q\0R\0Y",
		"x16 device operating in 16-bit mode" },
};

/* 4-byte access */
static const struct cfi_opmodes cfi_opmodes_4[] = {
	{ 2, 2, 0, 0x40, 12, "\0\0\0Q\0\0\0R\0\0\0Y",
		"x32 device operating in 32-bit mode" },
};


const struct nor_interface nor_interface_cfi = {
	.scan_media = cfi_scan_media,
	.init = cfi_init,
	.select = cfi_select,
	.read_1 = cfi_read_1,
	.read_2 = cfi_read_2,
	.read_4 = cfi_read_4,
	.read_buf_1 = cfi_read_buf_1,
	.read_buf_2 = cfi_read_buf_2,
	.read_buf_4 = cfi_read_buf_4,
	.write_1 = cfi_write_1,
	.write_2 = cfi_write_2,
	.write_4 = cfi_write_4,
	.write_buf_1 = cfi_write_buf_1,
	.write_buf_2 = cfi_write_buf_2,
	.write_buf_4 = cfi_write_buf_4,
	.read_page = NULL,			/* cmdset */
	.program_page = NULL,			/* cmdset */
	.busy = NULL,
	.private = NULL,
	.access_width = -1,
	.part_info = NULL,
	.part_num = -1,
};


/* only data[7..0] are used regardless of chip width */
#define cfi_unpack_1(n)			((n) & 0xff)

/* construct (arbitrarily big endian) uint16_t */
#define cfi_unpack_2(b0, b1)						\
	((cfi_unpack_1(b1) << 8) | cfi_unpack_1(b0))

/* construct (arbitrarily) big endian uint32_t */
#define cfi_unpack_4(b0, b1, b2, b3)					\
	((cfi_unpack_1(b3) << 24) |					\
	 (cfi_unpack_1(b2) << 16) |					\
	 (cfi_unpack_1(b1) <<  8) |					\
	 (cfi_unpack_1(b0)))

#define cfi_unpack_qry(qryp, data)					\
    do {								\
	(qryp)->qry[0] = cfi_unpack_1(data[0x10]);			\
	(qryp)->qry[1] = cfi_unpack_1(data[0x11]);			\
	(qryp)->qry[2] = cfi_unpack_1(data[0x12]);			\
	(qryp)->id_pri = be16toh(cfi_unpack_2(data[0x13], data[0x14]));	\
	(qryp)->addr_pri =						\
		be16toh(cfi_unpack_2(data[0x15], data[0x16]));		\
	(qryp)->id_alt = be16toh(cfi_unpack_2(data[0x17], data[0x18]));	\
	(qryp)->addr_alt =						\
		be16toh(cfi_unpack_2(data[0x19], data[0x1a]));		\
	(qryp)->vcc_min = cfi_unpack_1(data[0x1b]);			\
	(qryp)->vcc_max = cfi_unpack_1(data[0x1c]);			\
	(qryp)->vpp_min = cfi_unpack_1(data[0x1d]);			\
	(qryp)->vpp_max = cfi_unpack_1(data[0x1e]);			\
	(qryp)->write_word_time_typ = cfi_unpack_1(data[0x1f]);		\
	(qryp)->write_nbyte_time_typ = cfi_unpack_1(data[0x20]);	\
	(qryp)->erase_blk_time_typ = cfi_unpack_1(data[0x21]);		\
	(qryp)->erase_chiptime_typ = cfi_unpack_1(data[0x22]);		\
	(qryp)->write_word_time_max = cfi_unpack_1(data[0x23]);		\
	(qryp)->write_nbyte_time_max = cfi_unpack_1(data[0x24]);	\
	(qryp)->erase_blk_time_max = cfi_unpack_1(data[0x25]);		\
	(qryp)->erase_chiptime_max = cfi_unpack_1(data[0x26]);		\
	(qryp)->device_size = cfi_unpack_1(data[0x27]);			\
	(qryp)->interface_code_desc =					\
		be16toh(cfi_unpack_2(data[0x28], data[0x29]));		\
	(qryp)->write_nbyte_size_max = 					\
		be16toh(cfi_unpack_2(data[0x2a], data[0x2b]));		\
	(qryp)->erase_blk_regions = cfi_unpack_1(data[0x2c]);		\
	u_int _i = 0x2d;						\
	const u_int _n = (qryp)->erase_blk_regions;			\
	KASSERT(_n <= 4);						\
	for (u_int _r = 0; _r < _n; _r++, _i+=4) {			\
		(qryp)->erase_blk_info[_r].y =				\
			be32toh(cfi_unpack_2(data[_i+0], data[_i+1]));	\
		(qryp)->erase_blk_info[_r].z =				\
			be32toh(cfi_unpack_2(data[_i+2], data[_i+3]));	\
	}								\
    } while (0)

#define cfi_unpack_pri_0002(qryp, data)					\
    do {								\
	(qryp)->pri.cmd_0002.pri[0] = cfi_unpack_1(data[0x00]);		\
	(qryp)->pri.cmd_0002.pri[1] = cfi_unpack_1(data[0x01]);		\
	(qryp)->pri.cmd_0002.pri[2] = cfi_unpack_1(data[0x02]);		\
	(qryp)->pri.cmd_0002.version_maj = cfi_unpack_1(data[0x03]);	\
	(qryp)->pri.cmd_0002.version_min = cfi_unpack_1(data[0x04]);	\
	(qryp)->pri.cmd_0002.asupt = cfi_unpack_1(data[0x05]);		\
	(qryp)->pri.cmd_0002.erase_susp = cfi_unpack_1(data[0x06]);	\
	(qryp)->pri.cmd_0002.sector_prot = cfi_unpack_1(data[0x07]);	\
	(qryp)->pri.cmd_0002.tmp_sector_unprot =			\
		cfi_unpack_1(data[0x08]);				\
	(qryp)->pri.cmd_0002.sector_prot_scheme =			\
		cfi_unpack_1(data[0x09]);				\
	(qryp)->pri.cmd_0002.simul_op = cfi_unpack_1(data[0x0a]);	\
	(qryp)->pri.cmd_0002.burst_mode_type = cfi_unpack_1(data[0x0b]);\
	(qryp)->pri.cmd_0002.page_mode_type = cfi_unpack_1(data[0x0c]);	\
	(qryp)->pri.cmd_0002.acc_min = cfi_unpack_1(data[0x0d]);	\
	(qryp)->pri.cmd_0002.acc_max = cfi_unpack_1(data[0x0e]);	\
	(qryp)->pri.cmd_0002.wp_prot = cfi_unpack_1(data[0x0f]);	\
	/* XXX 1.3 stops here */					\
	(qryp)->pri.cmd_0002.prog_susp = cfi_unpack_1(data[0x10]);	\
	(qryp)->pri.cmd_0002.unlock_bypass = cfi_unpack_1(data[0x11]);	\
	(qryp)->pri.cmd_0002.sss_size = cfi_unpack_1(data[0x12]);	\
	(qryp)->pri.cmd_0002.soft_feat = cfi_unpack_1(data[0x13]);	\
	(qryp)->pri.cmd_0002.page_size = cfi_unpack_1(data[0x14]);	\
	(qryp)->pri.cmd_0002.erase_susp_time_max =			\
		cfi_unpack_1(data[0x15]);				\
	(qryp)->pri.cmd_0002.prog_susp_time_max =			\
		cfi_unpack_1(data[0x16]);				\
	(qryp)->pri.cmd_0002.embhwrst_time_max =			\
		cfi_unpack_1(data[0x38]);				\
	(qryp)->pri.cmd_0002.hwrst_time_max =				\
		cfi_unpack_1(data[0x39]);				\
    } while (0)

#define CFI_QRY_UNPACK_COMMON(cfi, data, type, found)			\
    do {								\
	struct cfi_query_data * const qryp = &cfi->cfi_qry_data;	\
									\
	memset(qryp, 0, sizeof(*qryp));					\
	cfi_unpack_qry(qryp, data);					\
									\
	switch (qryp->id_pri) {						\
	case 0x0002:							\
		if ((cfi_unpack_1(data[qryp->addr_pri + 0]) == 'P') &&	\
		    (cfi_unpack_1(data[qryp->addr_pri + 1]) == 'R') &&	\
		    (cfi_unpack_1(data[qryp->addr_pri + 2]) == 'I')) {	\
			type *pri_data = &data[qryp->addr_pri];		\
			cfi_unpack_pri_0002(qryp, pri_data);		\
			found = true;					\
			break;						\
		}							\
	default:							\
		printf("%s: unsupported id_pri=%#x\n",			\
			__func__, qryp->id_pri);			\
		break;	/* unknown command set */			\
	}								\
    } while (0)

/*
 * cfi_chip_query_opmode - determine operational mode based on QRY signature
 */
static bool
cfi_chip_query_opmode(struct cfi *cfi, uint8_t *data,
    const struct cfi_opmodes *tab, u_int nentries)
{
	for (u_int i=0; i < nentries; i++) {
		if (memcmp(&data[tab[i].qsa], tab[i].sig, tab[i].len) == 0) {
			cfi->cfi_opmode = &tab[i];
			return true;
		}
	}
	return false;
}

static bool
cfi_chip_query_1(struct cfi * const cfi)
{
	uint8_t data[0x80];

	bus_space_read_region_1(cfi->cfi_bst, cfi->cfi_bsh, 0, data,
		__arraycount(data));

	bool found = cfi_chip_query_opmode(cfi, data, cfi_opmodes_1,
		__arraycount(cfi_opmodes_1));

	if (found) {
		CFI_QRY_UNPACK_COMMON(cfi, data, uint8_t, found);
	}

	return found;
}

static bool
cfi_chip_query_2(struct cfi * const cfi)
{
	uint16_t data[0x80];

	bus_space_read_region_2(cfi->cfi_bst, cfi->cfi_bsh, 0, data,
		__arraycount(data));

	bool found = cfi_chip_query_opmode(cfi, (uint8_t *)data,
		cfi_opmodes_2, __arraycount(cfi_opmodes_2));

	if (found) {
		CFI_QRY_UNPACK_COMMON(cfi, data, uint16_t, found);
	}

	return found;
}

static bool
cfi_chip_query_4(struct cfi * const cfi)
{
	uint32_t data[0x80];

	bus_space_read_region_4(cfi->cfi_bst, cfi->cfi_bsh, 0, data,
		__arraycount(data));

	bool found = cfi_chip_query_opmode(cfi, (uint8_t *)data,
		cfi_opmodes_4, __arraycount(cfi_opmodes_4));

	if (found) {
		CFI_QRY_UNPACK_COMMON(cfi, data, uint32_t, found);
	}

	return found;
}

static bool
cfi_chip_query_8(struct cfi * const cfi)
{
#ifdef NOTYET
	uint64_t data[0x80];

	bus_space_read_region_8(cfi->cfi_bst, cfi->cfi_bsh, 0, data,
		__arraycount(data));

	bool found = cfi_chip_query_opmode(cfi, (uint8_t *)data,
		cfi_opmodes_8, __arraycount(cfi_opmodes_8));

	if (found) {
		CFI_QRY_UNPACK_COMMON(cfi, data, uint64_t, found);
	}

	return found;
#else
	return false;
#endif
}

/*
 * cfi_chip_query - detect a CFI chip
 *
 * fill in the struct cfi as we discover what's there
 */
static bool
cfi_chip_query(struct cfi * const cfi)
{
	bool found = false;
	const bus_size_t cfi_query_offset[] = {
		CFI_QUERY_MODE_ADDRESS,
		CFI_QUERY_MODE_ALT_ADDRESS
	};

	KASSERT(cfi != NULL);
	KASSERT(cfi->cfi_bst != NULL);

	for (int j=0; !found && j < __arraycount(cfi_query_offset); j++) {

		cfi_reset_default(cfi);
		cfi_cmd(cfi, cfi_query_offset[j], CFI_QUERY_DATA);

		switch(cfi->cfi_portwidth) {
		case 0:
			found = cfi_chip_query_1(cfi);
			break;
		case 1:
			found = cfi_chip_query_2(cfi);
			break;
		case 2:
			found = cfi_chip_query_4(cfi);
			break;
		case 3:
			found = cfi_chip_query_8(cfi);
			break;
		default:
			panic("%s: bad portwidth %d\n",
				__func__, cfi->cfi_portwidth);
		}
	}

	return found;
}

/*
 * cfi_probe - search for a CFI NOR trying various port & chip widths
 *
 * NOTE:
 *   striped NOR chips design not supported yet,
 *   so force portwidth=chipwidth for now
 *   eventually permute portwidth seperately
 */
bool
cfi_probe(struct cfi * const cfi)
{
	bool found;

	KASSERT(cfi != NULL);

	for (u_int cw = 0; cw < 3; cw++) {
		cfi->cfi_portwidth = 		/* XXX */
		cfi->cfi_chipwidth = cw;
		found = cfi_chip_query(cfi);
		if (found)
			goto out;
	}
 out:
	cfi_reset_default(cfi);		/* exit QRY mode */
	return found;
}

bool
cfi_identify(struct cfi * const cfi)
{
	const bus_space_tag_t bst = cfi->cfi_bst;
	const bus_space_handle_t bsh = cfi->cfi_bsh;
	bool found = true;

	KASSERT(cfi != NULL);
	KASSERT(bst != NULL);

	memset(cfi, 0, sizeof(struct cfi));	/* XXX clean slate */
	cfi->cfi_bst = bst;		/* restore bus space */
	cfi->cfi_bsh = bsh;		/*  "       "   "    */

	/* gather CFI PRQ and PRI data */
	if (! cfi_probe(cfi)) {
		aprint_debug("%s: cfi_probe failed\n", __func__);
		found = false;
		goto out;
	}

	/* gather ID data if possible */
	if (! cfi_jedec_id(cfi)) {
		aprint_debug("%s: cfi_jedec_id failed\n", __func__);
		goto out;
	}

 out:
	cfi_reset_default(cfi);	/* exit QRY mode */

	return found;
}

static int
cfi_scan_media(device_t self, struct nor_chip *chip)
{
	struct nor_softc *sc = device_private(self);
	KASSERT(sc != NULL);
	KASSERT(sc->sc_nor_if != NULL);
	struct cfi * const cfi = (struct cfi * const)sc->sc_nor_if->private;
	KASSERT(cfi != NULL);

	sc->sc_nor_if->access_width = cfi->cfi_portwidth;

	chip->nc_manf_id = cfi->cfi_id_data.id_mid;
	chip->nc_dev_id = cfi->cfi_id_data.id_did[0]; /* XXX 3 words */
	chip->nc_size = 1 << cfi->cfi_qry_data.device_size;

	/* size of line for Read Buf command */
	chip->nc_line_size = 1 << cfi->cfi_qry_data.pri.cmd_0002.page_size;

	/*
	 * size of erase block
	 * XXX depends on erase region
	 */
	chip->nc_num_luns = 1;
	chip->nc_lun_blocks = cfi->cfi_qry_data.erase_blk_info[0].y + 1;
	chip->nc_block_size = cfi->cfi_qry_data.erase_blk_info[0].z * 256;

	switch (cfi->cfi_qry_data.id_pri) {
	case 0x0002:
		cfi_0002_init(sc, cfi, chip);
		break;
	default:
		return -1;
	}

	return 0;
}

void
cfi_init(device_t self)
{
	/* nothing */
}

static void
cfi_select(device_t self, bool select)
{
	/* nothing */
}

static void
cfi_read_1(device_t self, flash_off_t offset, uint8_t *datap)
{
}

static void
cfi_read_2(device_t self, flash_off_t offset, uint16_t *datap)
{
}

static void
cfi_read_4(device_t self, flash_off_t offset, uint32_t *datap)
{
}

static void
cfi_read_buf_1(device_t self, flash_off_t offset, uint8_t *datap, size_t size)
{
}

static void
cfi_read_buf_2(device_t self, flash_off_t offset, uint16_t *datap, size_t size)
{
}

static void
cfi_read_buf_4(device_t self, flash_off_t offset, uint32_t *datap, size_t size)
{
}

static void
cfi_write_1(device_t self, flash_off_t offset, uint8_t data)
{
}

static void
cfi_write_2(device_t self, flash_off_t offset, uint16_t data)
{
}

static void
cfi_write_4(device_t self, flash_off_t offset, uint32_t data)
{
}

static void
cfi_write_buf_1(device_t self, flash_off_t offset, const uint8_t *datap,
    size_t size)
{
}

static void
cfi_write_buf_2(device_t self, flash_off_t offset, const uint16_t *datap,
    size_t size)
{
}

static void
cfi_write_buf_4(device_t self, flash_off_t offset, const uint32_t *datap,
    size_t size)
{
}

void
cfi_cmd(struct cfi * const cfi, bus_size_t off, uint32_t val)
{
	const bus_space_tag_t bst = cfi->cfi_bst;
	bus_space_handle_t bsh = cfi->cfi_bsh;

	off <<= cfi->cfi_portwidth;

	DPRINTF(("%s: %p %x %x %x\n", __func__, bst, bsh, off, val));

	switch(cfi->cfi_portwidth) {
	case 0:
		bus_space_write_1(bst, bsh, off, (uint8_t)val);
		break;
	case 1:
		bus_space_write_2(bst, bsh, off, val);
		break;
	case 2:
		bus_space_write_4(bst, bsh, off, (uint32_t)val);
		break;
#ifdef NOTYET
	case 3:
		bus_space_write_4(bst, bsh, off, (uint64_t)val);
		break;
#endif
	default:
		panic("%s: bad portwidth %d bytes\n",
			__func__, 1 << cfi->cfi_portwidth);
	}
}

/*
 * cfi_reset_default - when we don't know which command will work, use both
 */
void
cfi_reset_default(struct cfi * const cfi)
{
	cfi_cmd(cfi, CFI_ADDRESS_ANY, CFI_RESET_DATA);
	cfi_cmd(cfi, CFI_ADDRESS_ANY, CFI_ALT_RESET_DATA);
}

/*
 * cfi_reset_std - use standard reset command
 */
void
cfi_reset_std(struct cfi * const cfi)
{
	cfi_cmd(cfi, CFI_ADDRESS_ANY, CFI_RESET_DATA);
}

/*
 * cfi_reset_alt - use "alternate" reset command
 */
void
cfi_reset_alt(struct cfi * const cfi)
{
	cfi_cmd(cfi, CFI_ADDRESS_ANY, CFI_ALT_RESET_DATA);
}

static void
cfi_jedec_id_2(struct cfi * const cfi)
{
	struct cfi_jedec_id_data *idp = &cfi->cfi_id_data;
	uint16_t data[0x10];

	bus_space_read_region_2(cfi->cfi_bst, cfi->cfi_bsh, 0, data,
		__arraycount(data));

	idp->id_mid = data[0];
	idp->id_did[0] = data[1];
	idp->id_did[1] = data[0xe];
	idp->id_did[2] = data[0xf];
	idp->id_prot_state = data[2];
	idp->id_indicators = data[3];

	/* software bits, upper and lower
	 * - undefined on S29GL-P
	 * - defined   on S29GL-S
	 */
	idp->id_swb_lo = data[0xc];
	idp->id_swb_hi = data[0xd];
}

/*
 * cfi_jedec_id - get JEDEC ID info
 *
 * this should be ignored altogether for CFI chips?
 * JEDEC ID is superceded by CFI info except CFI is not
 * a true superset of the JEDEC, so some info provided
 * by JEDEC is not available via CFI QRY.
 * But the JEDEC info is unreliable:
 * - different chips not distinguishaable by IDs
 * - some fields undefined (read as 0xff) on some chips
 */
static bool
cfi_jedec_id(struct cfi * const cfi)
{
	DPRINTF(("%s\n", __func__));

	cfi_cmd(cfi, 0x555, 0xaa);
	cfi_cmd(cfi, 0x2aa, 0x55);
	cfi_cmd(cfi, 0x555, 0x90);

	switch(cfi->cfi_portwidth) {
	case 1:
		cfi_jedec_id_2(cfi);
		break;
#ifdef NOTYET
	case 0:
		cfi_jedec_id_1(cfi);
		break;
	case 2:
		cfi_jedec_id_4(cfi);
		break;
	case 3:
		cfi_jedec_id_8(cfi);
		break;
#endif
	default:
		panic("%s: bad portwidth %d bytes\n",
			__func__, 1 << cfi->cfi_portwidth);
	}

	return true;
}

void
cfi_print(device_t self, struct cfi * const cfi)
{
	char pbuf[sizeof("XXXX MB")];
	struct cfi_query_data * const qryp = &cfi->cfi_qry_data;

	format_bytes(pbuf, sizeof(pbuf), 1 << qryp->device_size);
	aprint_normal_dev(self, "CFI NOR flash %s %s\n", pbuf,
		cfi_interface_desc_str(qryp->interface_code_desc));
#ifdef NOR_VERBOSE
	aprint_normal_dev(self, "manufacturer id %#x, device id %#x %#x %#x\n",
		cfi->cfi_id_data.id_mid,
		cfi->cfi_id_data.id_did[0],
		cfi->cfi_id_data.id_did[1],
		cfi->cfi_id_data.id_did[2]);
	aprint_normal_dev(self, "%s\n", cfi->cfi_opmode->str);
	aprint_normal_dev(self, "sw bits lo=%#x hi=%#x\n",
		cfi->cfi_id_data.id_swb_lo,
		cfi->cfi_id_data.id_swb_hi);
	aprint_normal_dev(self, "max multibyte write size %d\n",
		1 << qryp->write_nbyte_size_max);
	aprint_normal_dev(self, "%d Erase Block Region(s)\n",
		qryp->erase_blk_regions);
	for (u_int r=0; r < qryp->erase_blk_regions; r++) {
		size_t sz = qryp->erase_blk_info[r].z * 256;
		format_bytes(pbuf, sizeof(pbuf), sz);
		aprint_normal("    %d: %d blocks, size %s\n", r,
			qryp->erase_blk_info[r].y + 1, pbuf);
	}
#endif

	switch (cfi->cfi_qry_data.id_pri) {
	case 0x0002:
		cfi_0002_print(self, cfi);
		break;
	}
}
