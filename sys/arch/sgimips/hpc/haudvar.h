/* $NetBSD: haudvar.h,v 1.1 2026/06/11 01:03:58 rumble Exp $ */

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

#ifndef _ARCH_SGIMIPS_HPC_HAUDVAR_H_
#define _ARCH_SGIMIPS_HPC_HAUDVAR_H_

/*
 * Hollywood Audio is based around a Motorola 56001 DSP and supporting hardware
 * such as DACs, SRAMs, the HPC, and either the AUD1 ASIC or an equivalent FPGA.
 *
 * This driver leverages IRIX's hdsp.lod 56K code, which takes care of the audio
 * hardware behind the DSP and interfaces with the kernel primarily via circular
 * buffers in host physical memory.
 *
 * The DSP has a 32768 24-bit word memory provided by three 32Kbyte SRAMs. This
 * is used for the DSP's firmware code and data. The HPC maps the SRAMs into
 * the CPU's physical address space so that the kernel can load the DSP's
 * firmware and initialise circular command buffers. The SRAM is mapped via
 * 32768 contiguous 32-bit words at offset HPC1_DSP_SRAM. The HPC also provides
 * DMA for the DSP so that it can read from or write to the circular buffers in
 * host memory.
 *
 * IRIX's hdsp.lod firmware has two command buffers, one for sending command
 * requests to the DSP and one for receiving responses back. Command examples
 * include setting up new circular buffers for playback or recording ('sample
 * buffers'), tearing down unused circular buffers, and changing various audio
 * parameters such as output volumes, input attentuation, etc.
 *
 * The command request and response buffers are bootstrapped by writing to
 * special locations low in the SRAM space (after writing the firmware, but
 * before taking the DSP chip out of reset). After bootstrap, all subsequent
 * interaction happens via host memory (buffers), registers in the HPC, and
 * interrupts. The command request and response buffers are contiguous in host
 * physical memory, though due to their small size they can easily fit within a
 * page.
 *
 * All playback and recording uses 16-bit big endian PCM samples. Each sample
 * occupies a full 32-bit word in the circular buffer and is shifted 8 bits left
 * (i.e., it occupies the middle two bytes).
 *
 * Sample buffers for playback and recording do not need to be physically
 * contiguous in memory. Constituent pages are registered with the DSP as part
 * of setting up a new buffer (via the command buffer).
 *
 * Both command and sample buffers are controlled by a 16-byte header. The
 * command buffers' headers are followed immediately by their circular buffer.
 * Sample buffers' headers and circular buffers can be split apart, but this
 * driver maintains the same layout for both types.
 *
 * The header's format is shared with the DSP and consists of four 32-bit fields
 * (see haud_dsp_buffer_header_t below for details). Each circular buffer is
 * either used to send audio data to the DSP for playback (TX) or receive audio
 * data from the DSP for recording (RX).
 *
 * Each sample buffer has two unique identifiers: one assigned by the kernel and
 * registered with the DSP (kern_id), and one assigned by the DSP in response to
 * the registration (dsp_id). The DSP-returned ID is presumably dense and refers
 * to a fixed table of buffers under management. The DSP writes dsp_id to the TX
 * handshake register to indicate which buffer the current interrupt applies to.
 * It's not really clear what use kern_id has on the DSP side.
 *
 * The special command request and response queues apparently go by IDs 0 and 1,
 * respectively, and the first sample buffer registered gets ID 2.
 *
 * The DSP appears to only support one output stream and one input stream, as
 * sample buffers aren't registered against a particular port and sample rates
 * are configured globally for all outputs and all inputs. That said, the DSP
 * will supposedly mix multiple active output buffers automatically (per IRIX's
 * audio(1) manpage).
 *
 * Audio is played to all output ports simultaneously. On systems with an
 * internal speaker (Indigo), plugging into the headphone jack cuts off the
 * speaker output.
 *
 * A single input source can be selected at time. TODO: What happens if multiple
 * input sample buffers are allocated? Presumably the DSP would return an error
 * for such a request.
 */

/*
 * Number of 4KB pages we will allocate for each sample buffer.
 */
#define HAUD_SAMPLE_BUFFER_PAGES 64

/*
 * Circular buffer in the format understood by the DSP. Used for both command
 * request/response buffers and audio sample buffers.
 */
typedef struct haud_dsp_buffer_header {
	u_int32_t head;  	// Start of data (index in words units)
	u_int32_t tail;  	// End of data (index in word units)
	u_int32_t intr;  	// Set to 1 to trigger interrupt by DSP.
	u_int32_t watr;  	// A low watermark for interrupt triggering?
} haud_dsp_buffer_header_t;
CTASSERT(sizeof(haud_dsp_buffer_header_t) == 4 * 4);

/*
 * Represents a circular buffer registered with the DSP along with kernel state.
 *
 * Buffers are logically contiguous, but cobbled together from individual pages.
 * The first 16 bytes of the first page contains the header. We follow that
 * immediately with up to 1020 words of data. Pages after the first contain only
 * data.
 *
 * We could split the header and data, but there's not much point.
 */
typedef struct haud_buffer {
	u_int32_t kern_id;	// ID assigned by kernel, registered with DSP.
	u_int32_t dsp_id;	// ID assigned by DSP.
	bool is_write_buffer;	// True: CPU->DSP, false: DSP->CPU (ex. record).
	kcondvar_t cv;		// For blocking on DSP I/O (with sc_intr_lock).

	// One or more pages for the header and circular buffer. Command request
	// and response buffers are one page, sample buffers are multiple. 
	struct {
		bus_dmamap_t dma_map;
		bus_dma_segment_t dma_seg;
		uint32_t *kaddr;
	} pages[HAUD_SAMPLE_BUFFER_PAGES];
	int npages;
} haud_buffer_t;

/*
 * DSP Commands: Requests sent to the DSP via the command request buffer to
 * configure new sample buffers and change input/output parameters. Each request
 * is responded to via the response queue.
 *
 * The first word of each request and response is the full length of the message
 * in 32-bit words. The second word is the command opcode. Responses use the
 * same opcode as the request.
 */

#define HAUD_DSP_CMD_REGISTER_BUFFER_OPCODE 4
struct haud_dsp_cmd_register_buffer_req {
	u_int32_t len;		// 8.
	u_int32_t op;		// 4.
	u_int32_t kern_id;	// Kernel-assigned identifier.
	u_int32_t cap;		// Number of 32-bit words in the buffer.
	u_int32_t out;		// 1: Output (CPU->DSP), 0: Input (DSP->CPU).
	u_int32_t hdr_hi;	// haud_dsp_buffer phys. address (high 16 bits).
	u_int32_t hdr_lo;	// haud_dsp_buffer phys. address (low 16 bits).
	u_int32_t buf_off;	// Buffer start offset in first page (bytes).

	/*
	 * The following contains the physical page number (i.e., phys_addr /
	 * 4096) of each page in the buffer. The first page needs to be included
	 * even though it's already referenced above.
	 *
	 * This is a variable-length message, but we always allocate sample
	 * buffers of the same static size.
	 *
	 * TODO: What is the DSP's upper bound for pages per buffer?
	 */
	u_int32_t page_nums[HAUD_SAMPLE_BUFFER_PAGES];
};
CTASSERT(sizeof(struct haud_dsp_cmd_register_buffer_req) ==
    (8 + HAUD_SAMPLE_BUFFER_PAGES) * 4);

struct haud_dsp_cmd_register_buffer_resp {
	u_int32_t len;		// 5.
	u_int32_t op;		// 4.
	u_int32_t kern_id;	// Echo of the ID given in the request.
	u_int32_t error;	// 0 iff request succeeded.
	u_int32_t dsp_id;	// ID allocated by the DSP.
};
CTASSERT(sizeof(struct haud_dsp_cmd_register_buffer_resp) == 5 * 4);

#define HAUD_DSP_CMD_DEREGISTER_BUFFER_OPCODE 5
struct haud_dsp_cmd_deregister_buffer_req {
	u_int32_t len;		// 4.
	u_int32_t op;		// 5.
	u_int32_t unknown;	// Always 0?
	u_int32_t dsp_id;	// ID of the buffer to dereg (XXX DSP ID?).
};
CTASSERT(sizeof(struct haud_dsp_cmd_deregister_buffer_req) == 4 * 4);

struct haud_dsp_cmd_deregister_buffer_resp {
	u_int32_t len;		// 4.
	u_int32_t op;		// 5.
	u_int32_t unknown;	// Always 0?
	u_int32_t error;	// 0 iff request succeeded.
};
CTASSERT(sizeof(struct haud_dsp_cmd_deregister_buffer_resp) == 4 * 4);

#define HAUD_DSP_CMD_SET_AUDIO_PARAMS 7
struct haud_dsp_cmd_set_audio_params_req {
	u_int32_t len;		// Variable: 3 + 2 * num_params.
	u_int32_t op;		// 7.
	u_int32_t unknown;	// Always 0?

	/*
	 * This is a variable-length message, but we always send the same full
	 * set of parameters.
	 */
	struct {
		u_int32_t param;
		u_int32_t value;
	} params[7];		// One or more <param, value> pairs.
};
CTASSERT(sizeof(struct haud_dsp_cmd_set_audio_params_req) == (3 + 7 * 2) * 4);

struct haud_dsp_cmd_set_audio_params_resp {
	u_int32_t len;		// 4.
	u_int32_t op;		// 7.
	u_int32_t unknown;	// Always 0?
	u_int32_t unknown2;	// error?
};
CTASSERT(sizeof(struct haud_dsp_cmd_set_audio_params_resp) == 4 * 4);

#define HAUD_AUDIO_PARAMS_INPUT_SRC		0	// 0:line 1:mic 2: digtl
#define HAUD_AUDIO_PARAMS_INPUT_ATTN_L		1	// Left attenuation
#define HAUD_AUDIO_PARAMS_INPUT_ATTN_R		2	// Right attenuation
#define HAUD_AUDIO_PARAMS_INPUT_RATE		3	// See values below
#define HAUD_AUDIO_PARAMS_OUTPUT_RATE		4	// See values below
#define HAUD_AUDIO_PARAMS_SPKR_GAIN_L		5	// Left gain
#define HAUD_AUDIO_PARAMS_SPKR_GAIN_R		6	// Right gain

/*
 * Minimum and maximum values for input/output gain/attenuation parameters.
 */
#define HAUD_MIN_GAIN	0
#define HAUD_MAX_GAIN	255
#define HAUD_MIN_ATTN	0
#define HAUD_MAX_ATTN	255

/*
 * Values for HAUD_AUDIO_PARAMS_INPUT_RATE and _OUTPUT_RATE parameters.
 * Many others are supported, too, but aren't terribly important.
 */
#define HAUD_RATE_48000	0x00
#define HAUD_RATE_44100	0x04
#define HAUD_RATE_22050	0x14
#define HAUD_RATE_11025	0x34

struct haud_softc {
	device_t sc_dev;
	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;

	bus_space_tag_t sc_st;
	bus_space_handle_t sc_regs_sh;
	bus_space_handle_t sc_sram_sh;
	bus_dma_tag_t sc_dma_tag;

	haud_buffer_t *sc_cmd_req;
	haud_buffer_t *sc_cmd_resp;
	haud_buffer_t *sc_output;

	// Protected by sc_intr_lock.
	bool sc_dsp_booted;

	// Provided by the audio(9) layer in its call to haud_start_output.
	void (*sc_output_intr)(void *);
	void *sc_output_intr_arg;

	// Used to trigger additional output after haud_start_output.
	void *sc_output_softint_cookie;

	int sc_speaker_l_gain;
	int sc_speaker_r_gain;
};

#endif  /* _ARCH_SGIMIPS_HPC_HAUDVAR_H_ */
