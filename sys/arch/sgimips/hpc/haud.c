/* $NetBSD: haud.c,v 1.1 2026/06/11 01:03:58 rumble Exp $ */

/*
 * Copyright (c) 2025 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: haud.c,v 1.1 2026/06/11 01:03:58 rumble Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/intr.h>
#include <machine/sysconf.h>

#include <dev/audio/audio_if.h>
#include <dev/firmload.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>

#include <sgimips/hpc/haudreg.h>
#include <sgimips/hpc/haudvar.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)      printf x
#else
#define DPRINTF(x)
#endif

static int haud_open(void *, int);
static int haud_query_format(void *, audio_format_query_t *);
static int haud_set_format(void *, int,
	const audio_params_t *, const audio_params_t *,
	audio_filter_reg_t *, audio_filter_reg_t *);
static int haud_round_blocksize(void *, int,
	int, const audio_params_t *);
static int haud_start_output(void *, void *, int, void (*)(void *),
	void *);
static int haud_halt_output(void *);
static int haud_getdev(void *, struct audio_device *);
static int haud_set_port(void *, mixer_ctrl_t *);
static int haud_get_port(void *, mixer_ctrl_t *);
static int haud_query_devinfo(void *, mixer_devinfo_t *);
static int haud_get_props(void *);
static void haud_get_locks(void *, kmutex_t **, kmutex_t **);

static const struct audio_hw_if haud_hw_if = {
	.open			= haud_open,
	.query_format		= haud_query_format,
	.set_format		= haud_set_format,
	.start_output		= haud_start_output,
	.halt_output		= haud_halt_output,
	.getdev			= haud_getdev,
	.set_port		= haud_set_port,
	.get_port		= haud_get_port,
	.query_devinfo		= haud_query_devinfo,
	.get_props		= haud_get_props,
	.get_locks		= haud_get_locks,
	.round_blocksize	= haud_round_blocksize,
};

static const struct audio_device haud_device = {
	"HAUD",
	"",
	"haud"
};

static const struct audio_format haud_formats = {
	.mode		= AUMODE_PLAY,
	.encoding	= AUDIO_ENCODING_SLINEAR_BE,
	.validbits	= 16,
	.precision	= 16,
	.channels	= 2,
	.channel_mask	= AUFMT_STEREO,
	.frequency_type	= 1,
	.frequency	= { 44100 },
};
#define HAUD_NFORMATS __arraycount(haud_formats)

#define HAUD_MASTER_VOL		0
#define HAUD_OUTPUT_CLASS	1

static int  haud_match(device_t, cfdata_t, void *);
static void haud_attach(device_t, device_t, void *);
static void haud_softintr(void *);
static int  haud_intr(void *);

CFATTACH_DECL_NEW(haud, sizeof(struct haud_softc),
    haud_match, haud_attach, NULL, NULL);

#define haud_write_sram_word(sc,idx,val) \
    bus_space_write_4(sc->sc_st, sc->sc_sram_sh, idx*4, val)

#define haud_write_reg(sc,off,val) \
    bus_space_write_4(sc->sc_st, sc->sc_regs_sh, off, val)

#define haud_read_reg(sc,off) \
    bus_space_read_4(sc->sc_st, sc->sc_regs_sh, off)

/*
 * XXX We only allocate one sample buffer right now, which the DSP assigns
 * this ID to. It happens to be the same as the kernel ID we send in the
 * registration request.
 *
 * If we dynamically allocate buffers in the future, we will need to track the
 * DSP IDs returned after registering.
 */
#define HAUD_SINGLETON_OUTPUT_BUFFER_ID 2

// Hardware assumes 4K pages.
CTASSERT(PAGE_SIZE == 4096);

#define WORDS_PER_PAGE (PAGE_SIZE / sizeof(u_int32_t))
#define HEADER_WORDS (sizeof(haud_dsp_buffer_header_t) / sizeof(u_int32_t))

static haud_dsp_buffer_header_t *
haud_buffer_header(haud_buffer_t *buf) {
	KASSERT(MIPS_KSEG1_P(buf->pages[0].kaddr));
	return (haud_dsp_buffer_header_t *)buf->pages[0].kaddr;
}

static int
haud_buffer_word_capacity(haud_buffer_t *buf)
{
	return (buf->npages * PAGE_SIZE - sizeof(haud_dsp_buffer_header_t)) /
	    sizeof(u_int32_t);
}

static int
haud_buffer_page_number(int buf_idx)
{
	return (buf_idx + HEADER_WORDS) / WORDS_PER_PAGE;
}

static int
haud_buffer_page_offset(int buf_idx)
{
	const int first_page_words = WORDS_PER_PAGE - HEADER_WORDS;
	if (buf_idx < first_page_words) {
		return buf_idx + HEADER_WORDS;
	} else {
		return (buf_idx - first_page_words) % WORDS_PER_PAGE;
	}
}

static bus_addr_t
haud_buffer_page_dma_addr(haud_buffer_t *buf, int page)
{
	KASSERT(buf->pages[page].dma_map->dm_nsegs == 1);
	return buf->pages[page].dma_map->dm_segs[0].ds_addr;
}

static int
haud_buffer_occupied_words(haud_buffer_t *buf)
{
	haud_dsp_buffer_header_t *hdr = haud_buffer_header(buf);
	int head = hdr->head;
	int tail = hdr->tail;
	int capacity = haud_buffer_word_capacity(buf);
	return tail >= head ? tail - head : capacity - (head - tail);
}

static int
haud_buffer_free_words(haud_buffer_t *buf)
{
	return haud_buffer_word_capacity(buf) - haud_buffer_occupied_words(buf);
}

static bool
haud_alloc_buffer_page(struct haud_softc *sc,
		       haud_buffer_t *buf,
		       int page,
		       bool no_wait)
{
	// HPC can only address 28 bits for SCSI and Ethernet. Does this
	// device have the same limitation? Only a potential issue on IP20.
	const bus_size_t boundary = 1 << 28;

	const int flags = no_wait ? BUS_DMA_NOWAIT : 0;

	int rsegs;
	if (bus_dmamem_alloc(sc->sc_dma_tag, PAGE_SIZE, PAGE_SIZE, boundary,
	    &buf->pages[page].dma_seg, 1, &rsegs, flags)) {
		goto  fail_dmamem_alloc;
	}

	// We rely on BUS_DMA_COHERENT mapping accesses to KSEG1 (uncached) for
	// the first page. This avoids potential clobbering of the header, and
	// buffer contents following it, that could be in the same cache line.
	//
	// The problem is that CPU and DSP accesses to the circular buffer are
	// apparently only loosely coordinated. If, for example, the CPU is
	// reading from an input buffer, it must update the head index after
	// consuming the DSP's data. However, the DSP may write new data to the
	// buffer and update the tail index at any point (so long as the buffer
	// isn't full). This means that we cannot keep the CPU cache coherent
	// with the buffer. If the CPU were to update the head index with a
	// cached write, we would risk writing back stale words in the same
	// cacheline.
	//
	// This isn't a concern on IP12 since the R3000's D-cache is 4 bytes
	// wide, but IP20's L1 and L2 caches are 32B and 128B, respectively.
	//
	// We could separate the header and buffer to limit uncached accesses
	// to just the header. Cached reads/writes to audio data in the first
	// page would be roughly 10x faster, but the benefit of speeding up
	// access to 1/n'th of the buffer isn't really worth it.
	const int coherent = page == 0 ? BUS_DMA_COHERENT : 0;
	if (bus_dmamem_map(sc->sc_dma_tag, &buf->pages[page].dma_seg, 1,
	    PAGE_SIZE, (void **)&buf->pages[page].kaddr, flags | coherent)) {
		goto fail_dmamem_map;
	}
	KASSERT((page != 0) ^ MIPS_KSEG1_P(buf->pages[page].kaddr));

	if (bus_dmamap_create(sc->sc_dma_tag, PAGE_SIZE, 1, PAGE_SIZE, boundary,
	    flags, &buf->pages[page].dma_map)) {
		goto fail_dmamap_create;
	}

	if (bus_dmamap_load(sc->sc_dma_tag, buf->pages[page].dma_map,
	    buf->pages[page].kaddr, PAGE_SIZE, NULL, flags)) {
		goto fail_dmamap_load;
	}

	memset(buf->pages[page].kaddr, 0, PAGE_SIZE);

	return buf;

fail_dmamap_load:
	bus_dmamap_destroy(sc->sc_dma_tag, buf->pages[page].dma_map);
fail_dmamap_create:
fail_dmamem_map:
	bus_dmamem_free(sc->sc_dma_tag, &buf->pages[page].dma_seg, 1);
fail_dmamem_alloc:
	return NULL;
}

static void
haud_free_buffer_page(struct haud_softc *sc,
		      haud_buffer_t *buf,
		      int page)
{
	bus_dmamap_destroy(sc->sc_dma_tag, buf->pages[page].dma_map);
	bus_dmamem_free(sc->sc_dma_tag, &buf->pages[page].dma_seg, 1);
}

static haud_buffer_t *
haud_create_buffer(struct haud_softc *sc, bool is_command_buffer)
{
	int flags = (is_command_buffer ? M_NOWAIT : 0) | M_ZERO;
	haud_buffer_t *buf = malloc(sizeof(haud_buffer_t), M_DEVBUF, flags);
	if (buf == NULL) {
		return NULL;
	}

	/*
	 * XXX Consider splitting the header and buffer and allocating the
	 * latter in virtually contiguous memory. That would simplify the
	 * buffer read/write routines for sample buffers. Though the command
	 * buffers can't be split and the asymmetry would add some complexity
	 * back.
	 */
	buf->npages = is_command_buffer ? 1 : __arraycount(buf->pages);
	for (int i = 0; i < buf->npages; i++) {
		const bool no_wait = is_command_buffer;
		if (!haud_alloc_buffer_page(sc, buf, i, no_wait)) {
			for (int j = 0; j < i; j++) {
				haud_free_buffer_page(sc, buf, j);
			}
			return NULL;	
		}
	}

	cv_init(&buf->cv, "haudintr");

	return buf;
}

static void
haud_copy_audio_to_buffer(u_int32_t *dst,
		          const u_int16_t *src,
		          int copy_words)
{
	// A simple copy loop is 7 instrs/word. Naive unrolling approaches 4,
	// but GCC leaves load hazard slots unused (nop-filled). Manual
	// pipelining fills those slots and approaches optimal 3 instrs/word.
	// Too bad the samples aren't half-word aligned...
	const u_int16_t *end = src + copy_words;
	while (src + 16 <= end) {
		u_int16_t a, b;
#define _pipelined_copy(_x, _y)		\
    a = src[_x];			\
    b = src[_y];			\
    dst[_x] = ((u_int32_t)a) << 8;	\
    dst[_y] = ((u_int32_t)b) << 8
		_pipelined_copy(0, 1);
		_pipelined_copy(2, 3);
		_pipelined_copy(4, 5);
		_pipelined_copy(6, 7);
		_pipelined_copy(8, 9);
		_pipelined_copy(10, 11);
		_pipelined_copy(12, 13);
		_pipelined_copy(14, 15);
#undef _pipelined_copy
		src += 16, dst += 16;
	}
	while (src < end) {
		*dst++ = ((u_int32_t)*src++) << 8;
	}
}

static void
haud_copy_audio_from_buffer(u_int16_t *dst, const u_int32_t *src, int copy_words) {
	const u_int32_t *end = src + copy_words;
	while (src + 16 <= end) {
		u_int32_t a, b;
#define _pipelined_copy(_x, _y)		\
    a = src[_x];			\
    b = src[_y];			\
    dst[_x] = (a >> 8) & 0xffff;	\
    dst[_y] = (b >> 8) & 0xffff
		_pipelined_copy(0, 1);
		_pipelined_copy(2, 3);
		_pipelined_copy(4, 5);
		_pipelined_copy(6, 7);
		_pipelined_copy(8, 9);
		_pipelined_copy(10, 11);
		_pipelined_copy(12, 13);
		_pipelined_copy(14, 15);
#undef _pipelined_copy
		src += 16, dst += 16;
	}
	while (src < end) {
		*dst++ = (*src++ >> 8) & 0xffff;
	}
}

/*
 * Adapter for callers that write / read commands (32-bit) or audio (16-bit)
 * to / from the 32-bit circular DSP buffer using haud_{write,read}_buffer.
 */
typedef struct haud_buffer_io {
	enum haud_buffer_io_type {
		IO_TYPE_COMMAND,
		IO_TYPE_AUDIO,
	} type;
	union {
		u_int32_t *command;
		u_int16_t *audio;
	} data;
	int length;
} haud_buffer_io_t;

static bool
haud_wait_for_write(struct haud_softc *sc, haud_buffer_t *buf, int words)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	for (int i = 0; haud_buffer_free_words(buf) < words; i++) {
		if (i == 10) {
			printf("%s: wait_for_write stuck; bailing\n",
			    device_xname(sc->sc_dev));
			return false;
		}	
		haud_dsp_buffer_header_t *hdr = haud_buffer_header(buf);
		hdr->watr = haud_buffer_word_capacity(buf) - words;
		hdr->intr = 1;
		cv_timedwait(&buf->cv, &sc->sc_intr_lock, mstohz(100));
	}
	return true;
}

static bool
haud_write_buffer(struct haud_softc *sc,
		  haud_buffer_t *buf,
		  const haud_buffer_io_t *input)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	haud_dsp_buffer_header_t *hdr = haud_buffer_header(buf);

	KASSERT(buf->is_write_buffer);
	const int cap = haud_buffer_word_capacity(buf);
	KASSERT(input->length <= cap);

	if (!haud_wait_for_write(sc, buf, input->length)) {
		printf("%s: write_buffer timed out waiting for free space; "
		    "dropping samples", device_xname(sc->sc_dev));
		return false;
	}

	int tail = hdr->tail;
	int words_left = input->length;
	while (words_left > 0) {
		const int page = haud_buffer_page_number(tail);
		const int page_offset = haud_buffer_page_offset(tail);
		const int src_start = input->length - words_left;
		const int words = MIN(words_left, WORDS_PER_PAGE - page_offset);
		u_int32_t *dst = buf->pages[page].kaddr + page_offset;

		if (input->type == IO_TYPE_AUDIO) {
			const u_int16_t *src = input->data.audio + src_start;
			haud_copy_audio_to_buffer(dst, src, words);
		} else {
			const u_int32_t *src = input->data.command + src_start;
			for (int i = 0; i < words; i++) {
				dst[i] = *src++;
			}
		}

		tail += words;
		if (tail == haud_buffer_word_capacity(buf)) {
			tail = 0;
		}
		words_left -= words;

		bus_dmamap_sync(sc->sc_dma_tag, buf->pages[page].dma_map,
		    page_offset * sizeof(u_int32_t), words * sizeof(u_int32_t),
		    BUS_DMASYNC_PREWRITE);
	}
	hdr->tail = tail;

	return true;
}

static bool
haud_wait_for_read(struct haud_softc *sc, haud_buffer_t *buf, int words)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));
	for (int i = 0; haud_buffer_occupied_words(buf) < words; i++) {
		if (i == 10) {
			printf("%s: wait_for_read stuck; bailing\n",
			    device_xname(sc->sc_dev));
			return false;
		}	
		haud_dsp_buffer_header_t *hdr = haud_buffer_header(buf);
		hdr->watr = haud_buffer_word_capacity(buf) - words;
		hdr->intr = 1;
		cv_timedwait(&buf->cv, &sc->sc_intr_lock, mstohz(100));
	}
	return true;
}

static bool
haud_read_buffer(struct haud_softc *sc,
		 haud_buffer_t *buf,
		 const haud_buffer_io_t *output)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	haud_dsp_buffer_header_t *hdr = haud_buffer_header(buf);

	KASSERT(!buf->is_write_buffer);
	const int cap = haud_buffer_word_capacity(buf);
	KASSERT(output->length <= cap);

	// XXX- Handle the case of audio with stopped sampling, resulting in
	// a short read.
	if (!haud_wait_for_read(sc, buf, output->length)) {
		printf("%s: read_buffer timed out waiting for data; "
		    "dropping samples", device_xname(sc->sc_dev));
		return false;
	}

	int head = hdr->head;
	int words_left = output->length;
	while (words_left > 0) {
		const int page = haud_buffer_page_number(head);
		const int page_offset = haud_buffer_page_offset(head);
		const int dst_start = output->length - words_left;
		const int words = MIN(words_left, WORDS_PER_PAGE - page_offset);
		u_int32_t *src = buf->pages[page].kaddr + page_offset;

		bus_dmamap_sync(sc->sc_dma_tag, buf->pages[page].dma_map,
		    page_offset * sizeof(u_int32_t), words * sizeof(u_int32_t),
		    BUS_DMASYNC_PREREAD);

		if (output->type == IO_TYPE_AUDIO) {
			u_int16_t *dst = output->data.audio + dst_start;
			haud_copy_audio_from_buffer(dst, src, words);
		} else {
			u_int32_t *dst = output->data.command + dst_start;
			for (int i = 0; i < words; i++) {
				*dst++ = src[i];
			}
		}

		head += words;
		if (head == haud_buffer_word_capacity(buf)) {
			head = 0;
		}
		words_left -= words;
	}
	hdr->head = head;

	return true;
}

static void
haud_dsp_request(struct haud_softc *sc,
		 u_int32_t *request,
		 u_int32_t request_byte_length,
		 u_int32_t *response,
		 u_int32_t response_byte_length)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	// Requests/responses are always in word lengths (byte parameters
	// are for caller convenience of using sizeof).
	KASSERT(request_byte_length % 4 == 0);
	KASSERT(response_byte_length % 4 == 0);
	const int request_words = request_byte_length / 4;
	const int response_words = response_byte_length / 4;

	// Set the len field.
	request[0] = request_words;

	DPRINTF(("haud: Sending request to DSP:\n"));
	for (int i = 0; i < request_words; i++) {
		DPRINTF(("  0x%x\n", request[i]));
	}

	haud_buffer_io_t input = {
	    .type = IO_TYPE_COMMAND,
	    .data.command = request,
	    .length = request_words
	};
	haud_write_buffer(sc, sc->sc_cmd_req, &input);

	// The DSP responds almost immediately to some commands, but changing
	// audio parameters can take hundreds of milliseconds.
	if (!haud_wait_for_read(sc, sc->sc_cmd_resp, response_words)) {
		printf("%s: DSP did not respond to request id %d\n",
		    device_xname(sc->sc_dev), request[1]);
		return;
	}

	haud_buffer_io_t output = {
	    .type = IO_TYPE_COMMAND,
	    .data.command = response,
	    .length = response_words
	};
	haud_read_buffer(sc, sc->sc_cmd_resp, &output);

	DPRINTF(("haud: Received response from DSP:\n"));
	for (int i = 0; i < response_words; i++) {
		DPRINTF(("  0x%x\n", response[i]));
	}
}

static haud_buffer_t *
haud_alloc_sample_buffer(struct haud_softc *sc,
			 u_int32_t kern_id,
			 bool is_write_buffer)
{
	haud_buffer_t *buf = haud_create_buffer(sc, false);
	if (buf == NULL) {
		return buf;
	}

	buf->kern_id = kern_id;
	buf->is_write_buffer = is_write_buffer;

	// Register the buffer with the DSP.
	struct haud_dsp_cmd_register_buffer_req req;
	req.op = HAUD_DSP_CMD_REGISTER_BUFFER_OPCODE;
	req.kern_id = kern_id;
	req.cap = haud_buffer_word_capacity(buf);
	req.out = is_write_buffer ? 1 : 0;
	req.hdr_hi = haud_buffer_page_dma_addr(buf, 0) >> 16;
	req.hdr_lo = haud_buffer_page_dma_addr(buf, 0) & 0xffff;
	req.buf_off = sizeof(*haud_buffer_header(buf));
	for (int i = 0; i < __arraycount(req.page_nums); i++) {
		req.page_nums[i] = haud_buffer_page_dma_addr(buf, i) >> 12;
	}
	CTASSERT(__arraycount(buf->pages) == __arraycount(req.page_nums));

	struct haud_dsp_cmd_register_buffer_resp resp;
	haud_dsp_request(sc, (u_int32_t *)&req, sizeof(req),
	    (u_int32_t *)&resp, sizeof(resp));

	buf->dsp_id = resp.dsp_id;

	return buf;
}

static void
haud_set_audio_params(struct haud_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	struct haud_dsp_cmd_set_audio_params_req req;
	req.op = HAUD_DSP_CMD_SET_AUDIO_PARAMS;
	req.unknown = 0;
#define _setparam(_i, _p, _v)	\
    req.params[_i].param = _p;	\
    req.params[_i].value = _v
	_setparam(0, HAUD_AUDIO_PARAMS_INPUT_SRC, 0);
	_setparam(1, HAUD_AUDIO_PARAMS_INPUT_ATTN_L, 0);
	_setparam(2, HAUD_AUDIO_PARAMS_INPUT_ATTN_R, 0);
	_setparam(3, HAUD_AUDIO_PARAMS_INPUT_RATE, HAUD_RATE_44100);
	_setparam(4, HAUD_AUDIO_PARAMS_OUTPUT_RATE, HAUD_RATE_44100);
	_setparam(5, HAUD_AUDIO_PARAMS_SPKR_GAIN_L, sc->sc_speaker_l_gain);
	_setparam(6, HAUD_AUDIO_PARAMS_SPKR_GAIN_R, sc->sc_speaker_r_gain);
#undef _setparam

	struct haud_dsp_cmd_set_audio_params_resp resp;
	haud_dsp_request(sc, (u_int32_t *)&req, sizeof(req),
	    (u_int32_t *)&resp, sizeof(resp));

}

static bool
haud_load_firmware(struct haud_softc *sc)
{
	const int firmware_size = 128 * 1024;
	firmware_handle_t fhp;
	uint32_t *fw = NULL;
	int error;

	if ((error = firmware_open("haud", "hdsp.bin", &fhp))) {
		printf("%s: error %d opening firmware file, see haud(9)\n",
		    device_xname(sc->sc_dev), error);
		return false;
	}

	if (firmware_get_size(fhp) != firmware_size) {
		printf("%s: invalid firmware file size (must be %dKiB)\n",
		    device_xname(sc->sc_dev), firmware_size / 1024);
		firmware_close(fhp);
		return false;
	}

	fw = malloc(firmware_size, M_DEVBUF, M_NOWAIT | M_ZERO);
	if (fw == NULL) {
		firmware_close(fhp);
		return false;
	}

	if ((error = firmware_read(fhp, 0, fw, firmware_size))) {
		printf("%s: firmware file read failedu: %d\n",
		    device_xname(sc->sc_dev), error);
		firmware_close(fhp);
		free(fw, M_DEVBUF);
		return false;
	}

	for (int i = 0; i < firmware_size / 4; i++) {
		haud_write_sram_word(sc, i, fw[i]);
	}

	firmware_close(fhp);
	free(fw, M_DEVBUF);

	return true;
}

static bool
haud_boot_dsp(struct haud_softc *sc)
{
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_dsp_booted) {
		return true;
	}

	haud_write_reg(sc, HAUD_MISC_CSR,
	    HAUD_MISC_CSR_RESET | HAUD_MISC_CSR_32K_SRAM);
	delay(100);

	mutex_spin_exit(&sc->sc_intr_lock);
	bool loaded = haud_load_firmware(sc);
	mutex_spin_enter(&sc->sc_intr_lock);

	if (!loaded) {
		return false;
	}

	// Set up command request buffer and point the DSP at it.
	haud_buffer_header(sc->sc_cmd_req)->head = 0;
	haud_buffer_header(sc->sc_cmd_req)->tail = 0;
	haud_buffer_header(sc->sc_cmd_req)->intr = 0;
	haud_buffer_header(sc->sc_cmd_req)->watr = 0;
	haud_write_sram_word(sc, 0,
	    haud_buffer_page_dma_addr(sc->sc_cmd_req, 0) & 0xffff);
	haud_write_sram_word(sc, 1,
	    haud_buffer_page_dma_addr(sc->sc_cmd_req, 0) >> 16);
	haud_write_sram_word(sc, 2, haud_buffer_word_capacity(sc->sc_cmd_req));

	// Set up command response buffer and point the DSP at it.
	haud_buffer_header(sc->sc_cmd_resp)->head = 0;
	haud_buffer_header(sc->sc_cmd_resp)->tail = 0;
	haud_buffer_header(sc->sc_cmd_resp)->intr = 0;
	haud_buffer_header(sc->sc_cmd_resp)->watr = 0;
	haud_write_sram_word(sc, 3,
	    haud_buffer_page_dma_addr(sc->sc_cmd_resp, 0) & 0xffff);
	haud_write_sram_word(sc, 4,
	    haud_buffer_page_dma_addr(sc->sc_cmd_resp, 0) >> 16);
	haud_write_sram_word(sc, 5, haud_buffer_word_capacity(sc->sc_cmd_resp));

	// Enable TX and RX handshake interrupts. Don't interrupt on DMA, as
	// that happens far too frequently.
	haud_write_reg(sc, HAUD_CPU_INTR_STAT, 0);
	haud_write_reg(sc, HAUD_CPU_INTR_MASK,
	    HAUD_CPU_INTR_MASK_TX_ENBL | HAUD_CPU_INTR_MASK_RX_ENBL);

	// Fire up the DSP.
	haud_write_reg(sc, HAUD_MISC_CSR, HAUD_MISC_CSR_32K_SRAM);

	// Wait for the firmware to interrupt. This should happen within tens
	// of microseconds.
	cv_timedwait(&sc->sc_cmd_req->cv, &sc->sc_intr_lock, mstohz(10));

	if (!sc->sc_dsp_booted) {
		printf("%s: DSP failed to boot within 1000 usec\n",
		    device_xname(sc->sc_dev));
		return false;
	}

	printf("%s: DSP firmware booted\n", device_xname(sc->sc_dev));

	// Set up initial audio parameters.
	sc->sc_speaker_l_gain = sc->sc_speaker_r_gain = 16;
	haud_set_audio_params(sc);

	// Allocate and register our single output buffer.
	sc->sc_output = haud_alloc_sample_buffer(
	    sc, HAUD_SINGLETON_OUTPUT_BUFFER_ID, true);
	if (sc->sc_output == NULL) {
		// Bummer. Well, just reset the chip and we can try to reinit
		// again later.
		haud_write_reg(sc, HAUD_MISC_CSR,
		    HAUD_MISC_CSR_RESET | HAUD_MISC_CSR_32K_SRAM);
		sc->sc_dsp_booted = false;
		return false;
	}

	return true;
}

/*
 * Hollywood Audio should be present on most, if not all, IP12 Indigos and IP20
 * Indigos, though perhaps rare "Hollywood Light" or VME-based Indigos lack it.
 *
 * On IP12 Personal Irises the same Hollywood Audio hardware was implemented
 * as an option card called "Magnum Audio" (partially, anyway -- the DSP and
 * some other components are always on the mainboard).
 */
static int
haud_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hpc_attach_args *haa = aux;

	if (strcmp(haa->ha_name, cf->cf_name)) {
		return 0;
	}

	// See if we can read the CSR register.
	if (platform.badaddr((void *)(vaddr_t)(haa->ha_sh + haa->ha_devoff +
	    HAUD_MISC_CSR), sizeof(uint32_t))) {
		aprint_normal(": not installed (CSR unreadable)");
		return 0;
	}

	// See if we can read the first word in the DSP's SRAM.
	if (platform.badaddr((void *)(vaddr_t)(haa->ha_sh + haa->ha_dmaoff),
	    sizeof(uint32_t))) {
		aprint_normal(": not installed (SRAM unreadable)");
		return 0;
	}

	// Try resetting the DSP, writing to DSP SRAM, and reading back.
	*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(haa->ha_sh + haa->ha_devoff +
	    HAUD_MISC_CSR) = HAUD_MISC_CSR_RESET | HAUD_MISC_CSR_32K_SRAM;
	delay(100);
	const uint32_t random_24b = 0x00448de3;
	*(volatile uint32_t *)
	    MIPS_PHYS_TO_KSEG1(haa->ha_sh + haa->ha_dmaoff) = random_24b;
	if (*(volatile uint32_t *)
	    MIPS_PHYS_TO_KSEG1(haa->ha_sh + haa->ha_dmaoff) != random_24b) {
		aprint_normal(": not installed (SRAM unwritable)");
		return 0;
	}

	return 1;
}

static void
haud_attach(device_t parent, device_t self, void *aux)
{
	struct haud_softc *sc = device_private(self);
	struct hpc_attach_args *haa = aux;

	sc->sc_dev = self;
	sc->sc_st = haa->ha_st;
	sc->sc_dma_tag = haa->ha_dmat;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_devoff,
	    HPC1_DSP_DEVREGS_SIZE, &sc->sc_regs_sh)) {
		aprint_error(": unable to map HPC registers\n");
		return;
	}

	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_dmaoff,
	    HPC1_DSP_SRAM_SIZE, &sc->sc_sram_sh)) {
		aprint_error(": unable to map SRAM\n");
		return;
	}

	sc->sc_output_softint_cookie = softint_establish(SOFTINT_SERIAL,
	    haud_softintr, sc);
	if (sc->sc_output_softint_cookie == NULL) {
		aprint_error(": unable to establish soft interrupt\n");
		return;
	}

	if (cpu_intr_establish(haa->ha_irq, IPL_AUDIO, haud_intr, sc) == NULL) {
		aprint_error(": unable to establish hw interrupt\n");
		softint_disestablish(sc->sc_output_softint_cookie);
		return;
	}

	sc->sc_cmd_req = haud_create_buffer(sc, true);
	KASSERT(sc->sc_cmd_req != NULL);
	sc->sc_cmd_req->kern_id = 0;
	sc->sc_cmd_req->dsp_id = 0;
	sc->sc_cmd_req->is_write_buffer = true;

	sc->sc_cmd_resp = haud_create_buffer(sc, true);
	KASSERT(sc->sc_cmd_resp != NULL);
	sc->sc_cmd_req->kern_id = 1;
	sc->sc_cmd_req->dsp_id = 1;
	sc->sc_cmd_resp->is_write_buffer = false;

	aprint_normal(": Hollywood Audio (awaiting firmware, see haud(4))\n");

	sc->sc_dsp_booted = false;

	audio_attach_mi(&haud_hw_if, sc, self);
}

static void
haud_softintr(void *v)
{
	struct haud_softc *sc = v;
	mutex_spin_enter(&sc->sc_intr_lock);
	if (sc->sc_output_intr) {
		sc->sc_output_intr(sc->sc_output_intr_arg);
	}
	mutex_spin_exit(&sc->sc_intr_lock);
}

static int
haud_intr(void *v)
{
	struct haud_softc *sc = v;
	bool handled = false;

	mutex_spin_enter(&sc->sc_intr_lock);

	if (!sc->sc_dsp_booted) {
		sc->sc_dsp_booted = true;
	}

	const u_int32_t stat = haud_read_reg(sc, HAUD_CPU_INTR_STAT);
	haud_write_reg(sc, HAUD_CPU_INTR_STAT, 0);

	if (stat & HAUD_CPU_INTR_STAT_DMA) {
		// Nothing to do (this should be masked out anyway).
	}

	if (stat & HAUD_CPU_INTR_STAT_TX) {
		haud_buffer_t *buf = NULL;
		const u_int32_t buf_id = haud_read_reg(sc, HAUD_TX_HANDSHAKE);
		switch (buf_id) {
		case 0:
			buf = sc->sc_cmd_req;
			break;
		case 1:
			// Why TX interrupts for the cmd response queue?
			buf = sc->sc_cmd_resp;
			break;
		case HAUD_SINGLETON_OUTPUT_BUFFER_ID:
			buf = sc->sc_output;
			break;
		case 0xffff:
			// Usually this ID is read when the DSP first interrupts
			// after booting.
			break;
		default:
			printf("%s: unexpected TX intr for buf id 0x%x\n",
			    device_xname(sc->sc_dev), buf_id);
			break;
		}

		if (buf != NULL) {
			// If a thread is waiting in haud_wait_for_write, we
			// will wake it up below.
			haud_buffer_header(buf)->intr = 0;
			cv_signal(&buf->cv);
		}

		handled = true;
	}

	if (stat & HAUD_CPU_INTR_STAT_RX) {
		// Nothing to do until we support recording.
	}

	mutex_spin_exit(&sc->sc_intr_lock);

	return handled; 
}

static int
haud_open(void *v, int flags)
{
	struct haud_softc *sc = v;

	if (!haud_boot_dsp(sc)) {
		return ENXIO;
	}

	return 0;
}

static int
haud_query_format(void *v, audio_format_query_t *afp)
{
	return audio_query_format(&haud_formats, 1, afp);
}

static int
haud_set_format(void *v, int setmode,
		const audio_params_t *play, const audio_params_t *rec,
		audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	/* Nothing to do. We only support one format right now. */
	return 0;
}

static int
haud_round_blocksize(void *v, int blocksize,
		     int mode, const audio_params_t *param)
{
	KASSERT(blocksize <= PAGE_SIZE * 4);
	return PAGE_SIZE * 4;
}

static int
haud_halt_output(void *v)
{
	/* Nothing special to do. DSP will stop when it hits the tail. */
	struct haud_softc *sc = v;
	sc->sc_output_intr = NULL;
	return 0;
}

static int
haud_getdev(void *v, struct audio_device *dev)
{
	*dev = haud_device;
	return 0;
}

static int
haud_set_port(void *v, mixer_ctrl_t *mc)
{
	struct haud_softc *sc = v;

	KASSERT(!mutex_owned(&sc->sc_intr_lock));

	if (mc->type != AUDIO_MIXER_VALUE ||
	    mc->dev != HAUD_MASTER_VOL ||
	    mc->un.value.num_channels != 2) {
		return EINVAL;
	}

	const int l = mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
	const int r = mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
	if (l < HAUD_MIN_GAIN || r < HAUD_MIN_GAIN ||
	    l > HAUD_MAX_GAIN || r > HAUD_MAX_GAIN) {
		return EINVAL;
	}
	if (l != sc->sc_speaker_l_gain || r != sc->sc_speaker_r_gain) {
		mutex_spin_enter(&sc->sc_intr_lock);
		sc->sc_speaker_l_gain = l;
		sc->sc_speaker_r_gain = r;
		if (sc->sc_dsp_booted) {
			haud_set_audio_params(sc);
		}
		mutex_spin_exit(&sc->sc_intr_lock);
	}

	return 0;
}

static int
haud_get_port(void *v, mixer_ctrl_t *mc)
{
	struct haud_softc *sc = v;

	KASSERT(!mutex_owned(&sc->sc_intr_lock));

	if (mc->type != AUDIO_MIXER_VALUE ||
	    mc->dev != HAUD_MASTER_VOL ||
	    mc->un.value.num_channels != 2) {
		return EINVAL;
	}

	mutex_spin_enter(&sc->sc_intr_lock);
	mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = sc->sc_speaker_r_gain;
	mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = sc->sc_speaker_l_gain;
	mutex_spin_exit(&sc->sc_intr_lock);

	return 0;
}

static int
haud_query_devinfo(void *v, mixer_devinfo_t *dev)
{
	switch (dev->index) {
	case HAUD_MASTER_VOL:
		dev->type = AUDIO_MIXER_VALUE;
		dev->mixer_class = HAUD_OUTPUT_CLASS;
		dev->prev = dev->next = AUDIO_MIXER_LAST;
		strcpy(dev->label.name, AudioNmaster);
		dev->un.v.num_channels = 2;
		dev->un.v.delta = 16;
		strcpy(dev->un.v.units.name, AudioNvolume);
		break;

	case HAUD_OUTPUT_CLASS:
		dev->type = AUDIO_MIXER_CLASS;
		dev->mixer_class = HAUD_OUTPUT_CLASS;
		dev->next = dev->prev = AUDIO_MIXER_LAST;
		strcpy(dev->label.name, AudioCoutputs);
		break;

	default:
		return EINVAL;
	}

	return 0;
}

static int
haud_get_props(void *v)
{
	return AUDIO_PROP_PLAYBACK;
}

static int
haud_start_output(void *v, void *block, int blksize,
		  void (*intr)(void *), void *intrarg)
{
	struct haud_softc *sc = v;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	sc->sc_output_intr = intr;
	sc->sc_output_intr_arg = intrarg;

	haud_buffer_io_t input = {
	    .type = IO_TYPE_AUDIO,
	    .data.audio = block,
	    .length = blksize / 2
	};
	if (!haud_write_buffer(sc, sc->sc_output, &input)) {
		return EBUSY;
	}

	// Trigger the next block of input.
	softint_schedule(sc->sc_output_softint_cookie);

	return 0;
}

static void
haud_get_locks(void *v, kmutex_t **intr, kmutex_t **thread)
{
	struct haud_softc *sc = v;
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
