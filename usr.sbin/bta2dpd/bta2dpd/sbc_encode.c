/* $NetBSD: sbc_encode.c,v 1.13 2023/07/21 02:11:18 nat Exp $ */

/*-
 * Copyright (c) 2015 - 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
 *		This software is dedicated to the memory of -
 *	   Baron James Anlezark (Barry) - 1 Jan 1949 - 13 May 2012.
 *
 *		Barry was a man who loved his music.
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

#include <sys/cdefs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <sbc_coeffs.h>
#include <sbc_crc.h>
#include "sbc_encode.h"

static uint8_t make_crc(uint8_t);
uint8_t Crc8(uint8_t, uint8_t *, size_t, ssize_t);
static ssize_t make_frame(uint8_t *, int16_t *);
static ssize_t parseFrame(uint8_t *, int16_t *);
static void calc_scalefactors(int32_t samples[16][2][8]);
static uint8_t calc_scalefactors_joint(int32_t sb_sample[16][2][8]);
static size_t sbc_encode(int16_t *, int32_t *);
static void calc_bitneed(void);
static ssize_t get_bits(uint8_t *, int, uint32_t *);
static ssize_t move_bits(uint8_t *, int, uint32_t);
static ssize_t move_bits_crc(uint8_t *, int, uint32_t);
static size_t sbc_decode(int32_t *, int16_t *);

uint32_t scalefactor[2][8];
int bits[2][8];
int global_chan = 2;
int global_bands = 8;
int global_blocks = 16;
int global_volume = 0;
uint8_t global_bitpool = 32;
uint8_t global_mode = MODE_STEREO;
uint8_t global_alloc = ALLOC_LOUDNESS;
uint8_t global_freq = FREQ_44_1K;
uint8_t global_bands_config = BANDS_8;
uint8_t global_block_config = BLOCKS_16;
uint8_t join = 0;

#define SYNCWORD	0x9c

struct a2dp_frame_header {
	uint8_t syncword;
	uint8_t config;
	uint8_t bitpool;
	uint8_t crc;
};

struct a2dp_frame_header_joint {
	uint8_t syncword;
	uint8_t config;
	uint8_t bitpool;
	uint8_t crc;
	uint8_t joint;
};


struct a2dp_frame_mono {
	struct a2dp_frame_header header;
	uint8_t scale[4];
	uint8_t samples[256];
};

struct a2dp_frame_joint {
	struct a2dp_frame_header_joint header;
	uint8_t scale[8];
	uint8_t samples[256];
};

struct a2dp_frame {
	struct a2dp_frame_header header;
	uint8_t scale[8];
	uint8_t samples[256];
};

struct rtpHeader {
	uint8_t id;		/* Just random number. */
	uint8_t id2;
	uint8_t seqnumMSB;	/* Packet seq. number most significant byte. */
	uint8_t seqnumLSB;
	uint8_t ts3;		/* Timestamp most significant byte. */
	uint8_t ts2;
	uint8_t ts1;
	uint8_t ts0;		/* Timestamp least significant byte. */
	uint8_t reserved3;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;	/* Reseverd least significant byte set to 1. */
	uint8_t numFrames;	/* Number of sbc frames in this packet. */
};
	    
/* Loudness offset allocations. */
int loudnessoffset8[4][8] = {
    { -2, 0, 0, 0, 0, 0, 0, 1 },
    { -3, 0, 0, 0, 0, 0, 1, 2 },
    { -4, 0, 0, 0, 0, 0, 1, 2 },
    { -4, 0, 0, 0, 0, 0, 1, 2 },
};

int loudnessoffset4[4][4] = {
    { -1, 0, 0, 0 },
    { -2, 0, 0, 1 },
    { -2, 0, 0, 1 },
    { -2, 0, 0, 1 }
};

u_int
FLS(uint8_t x)
{
	u_int numset = 0;
	while (x) {
		if (x & 1)
			break;
		x >>= 1;
		numset++;
	}
	return numset;
}

uint8_t
calc_scalefactors_joint(int32_t sb_sample[16][2][8])
{
	int64_t sb_j[16][2];
	uint32_t lz, x, y;
	int32_t ax;
	int block, sb;
	unsigned int joint;

	joint = 0;
	for (sb = 0; sb < global_bands - 1; sb++) {
		for (block = 0; block < global_blocks; block++) {
			sb_j[block][0] = (sb_sample[block][0][sb]) +
			    (sb_sample[block][1][sb]);
			sb_j[block][1] = (sb_sample[block][0][sb]) -
			    (sb_sample[block][1][sb]);
		}

		x = 1 << 15;
		y = 1 << 15;
		for (block = 0; block < global_blocks; block++) {
			ax = abs((int32_t)(sb_j[block][0] / 2));
			if (ax)
				x |= (uint32_t)ax;
			ax = abs((int32_t)(sb_j[block][1] / 2));
			if (ax)
				y |= (uint32_t)ax;
		}

		lz = 1;
		while (!(x & __BIT(30))) {
			lz++;
			x <<= 1;
		}
		x = 16 - lz;

		lz = 1;
		while (!(y & __BIT(30))) {
			lz++;
			y <<= 1;
		}
		y = 16 - lz;

		if ((scalefactor[0][sb] + scalefactor[1][sb]) > x + y) {
			joint |= (unsigned int)(1 << (global_bands - sb - 1));
			scalefactor[0][sb] = x;
			scalefactor[1][sb] = y;
			for (block = 0; block < global_blocks; block++) {
				sb_sample[block][0][sb] = (int32_t)
				    (sb_j[block][0] / 2);
				sb_sample[block][1][sb] = (int32_t)
				    (sb_j[block][1] / 2);
			}
		}
	}

	return (uint8_t)joint;
}

void
calc_scalefactors(int32_t samples[16][2][8])
{
	uint32_t lz, x;
	int32_t ax;
	int ch, sb, block;

	for (ch = 0; ch < global_chan; ch++) {
		for (sb = 0; sb < global_bands; sb++) {
			x = 1 << 16;
			for (block = 0; block < global_blocks; block++) {
				ax = abs((int32_t)samples[block][ch][sb]);
				if (ax)
					x |= (uint32_t)ax;
			}

			lz = 1;
			while (!(x & __BIT(30))) {
				lz++;
				x <<= 1;
			}
			scalefactor[ch][sb] =  16 - lz;
		}
	}
}

void
calc_bitneed(void)
{
	int32_t bitneed[2][8];
	int32_t max_bitneed, bitcount;
	int32_t slicecount, bitslice;
	int32_t loudness;
	int ch, sb,start_chan = 0;

	if (global_mode == MODE_DUAL)
		global_chan = 1;
next_chan:
	max_bitneed=0;
	bitcount=0;
	slicecount=0;

	if (global_alloc == ALLOC_SNR) {
		for (ch = start_chan; ch < global_chan; ch++) {
			for (sb = 0; sb < global_bands; sb++) {
				bitneed[ch][sb] = (int32_t)scalefactor[ch][sb];

				if (bitneed[ch][sb] > max_bitneed)
					max_bitneed = bitneed[ch][sb];
			}
		}
	} else {
		for (ch = start_chan; ch < global_chan; ch++) {
			for (sb = 0; sb < global_bands; sb++) {
				if (scalefactor[ch][sb] == 0)
					bitneed[ch][sb] = -5;
				else {
					if (global_bands == 8) {
						loudness = (int32_t)
						    ((int)scalefactor[ch][sb]
						    - loudnessoffset8
						    [3 - FLS(global_freq)][sb]);
					} else {
						loudness = (int32_t)
						    ((int)scalefactor[ch][sb]
						    - loudnessoffset4
						    [3 - FLS(global_freq)][sb]);
					}
					if (loudness > 0)
						bitneed[ch][sb] = loudness / 2;
					else
						bitneed[ch][sb] = loudness;
				}
				if (bitneed[ch][sb] > max_bitneed)
					max_bitneed = bitneed[ch][sb];
			}
		}
	}

	slicecount = bitcount = 0;
	bitslice = max_bitneed+1;
	do {
		bitslice--;
		bitcount += slicecount;
		slicecount = 0;
		for (ch = start_chan; ch < global_chan; ch++) {
			for (sb = 0; sb < global_bands; sb++) {
				if((bitneed[ch][sb] > bitslice + 1)&&
				    (bitneed[ch][sb] < bitslice + 16))
					slicecount++;
				else if(bitneed[ch][sb] == bitslice + 1)
					slicecount += 2;
			}
		}
	} while (bitcount + slicecount < global_bitpool);
	if (bitcount + slicecount == global_bitpool) {
		bitcount += slicecount;
		bitslice--;
	}

	for (ch = start_chan; ch < global_chan; ch++) {
		for (sb = 0; sb < global_bands; sb++) {
			if (bitneed[ch][sb] < bitslice + 2)
				bits[ch][sb] = 0;
			else {
				bits[ch][sb] = bitneed[ch][sb] - bitslice;
				if (bits[ch][sb] > 16)
					bits[ch][sb] = 16;
			}
		}
	}

	if (global_mode == MODE_DUAL)
		ch = start_chan;
	else
		ch = 0;
	sb = 0;
	while (bitcount < global_bitpool && sb < global_bands) {
		if ((bits[ch][sb] >= 2) && (bits[ch][sb] < 16)) {
			bits[ch][sb]++;
			bitcount++;
		} else if ((bitneed[ch][sb] == bitslice + 1) &&
		    (global_bitpool > bitcount + 1)) {
			bits[ch][sb] = 2;
			bitcount += 2;
		}
		if (global_chan == 1 || start_chan == 1)
			sb++;
		else if (ch == 1) {
			ch = 0;
			sb++;
		} else
			ch = 1;
	}

	if (global_mode == MODE_DUAL)
		ch = start_chan;
	else
		ch = 0;
	sb = 0;
	while (bitcount < global_bitpool && sb < global_bands) {
		if (bits[ch][sb] < 16) {
			bits[ch][sb]++;
			bitcount++;
		}
		if (global_chan == 1 || start_chan == 1)
			sb++;
		else if (ch == 1) {
			ch = 0;
			sb++;
		} else
			ch = 1;
	}

	if (global_mode == MODE_DUAL && start_chan == 0) {
		start_chan = 1;
		global_chan = 2;
		goto next_chan;
	}
}

ssize_t
get_bits(uint8_t *data, int numbits, uint32_t *sample)
{
	static uint64_t cache = 0;
	static int cache_pos = 0;
	uint64_t tmp_cache;
	ssize_t written = 0;

	while (cache_pos < numbits) {
		cache <<= 8;
		cache |= *data & 0xff;
		data++;
		written++;
		cache_pos += 8;
	}

	if (numbits == 0) {
		if (cache_pos >= 8) { 
			cache_pos -= 8;
			tmp_cache = cache >> cache_pos;
			*sample = (uint32_t)tmp_cache;
			written--;
		}
		if (cache_pos) {
			*sample = (uint32_t)cache;
			written--;
		}
		cache = 0;
		cache_pos = 0;
	} else {
		cache_pos -= numbits;
		tmp_cache = cache & __BITS((uintmax_t)(numbits + cache_pos),
		    (uintmax_t)cache_pos); 
		cache &= ~tmp_cache;
		tmp_cache >>= cache_pos;
		*sample = (uint32_t)tmp_cache;
	}
	return written;
}

ssize_t
move_bits(uint8_t *data, int numbits, uint32_t sample)
{
	static uint64_t cache = 0;
	static int cache_pos = 0;
	uint8_t tmp_cache;
	ssize_t written = 0;

	if (numbits == 0) {
		while (cache_pos >= 8) { 
			cache_pos -= 8;
			tmp_cache = (uint8_t)(cache >> cache_pos);
			*data++ = tmp_cache;
			written++;
		}
		if (cache_pos)
			*data = (uint8_t)cache;
		cache = 0;
		cache_pos = 0;
	} else {
		cache_pos += numbits;
		cache <<= numbits;
		cache |= sample & __BITS((uintmax_t)numbits, 0); 
		while (cache_pos >= 8) {
			cache_pos -= 8;
			tmp_cache = (uint8_t)(cache >> cache_pos);
			*data++ = tmp_cache;
			written++;
		}
	}
	return written;
}

ssize_t
move_bits_crc(uint8_t *data, int numbits, uint32_t sample)
{
	static uint64_t cache = 0;
	static int cache_pos = 0;
	uint8_t tmp_cache;
	ssize_t written = 0;

	if (numbits > 8 || numbits < 0)
		return 0;

	if (numbits == 0) {
		while  (cache_pos >= 8) { 
			cache_pos -= 8;
			tmp_cache = (uint8_t)(cache >> cache_pos);
			*data++ = tmp_cache;
			written++;
		}
		if (cache_pos)
			*data = (uint8_t)cache;
		cache = 0;
		cache_pos = 0;
	} else {
		cache_pos += numbits;
		cache <<= numbits;
		cache |= sample & __BITS((uintmax_t)numbits, 0); 
		if (cache_pos >= 8) {
			cache_pos -= 8;
			tmp_cache = (uint8_t)(cache >> cache_pos);
			*data = tmp_cache;
			written++;
		}
	}
	return written;
}

size_t
sbc_encode(int16_t *input, int32_t *samples)
{
	int64_t delta[2][8], levels[2][8], S[80];
	static int32_t L[80], R[80];
	int32_t *X, Z[80], Y[80];
	int32_t output[16][2][8];
	int32_t audioout;
	int16_t left[8], right[8], *data;
	size_t numsamples;
	int i, k, block, chan, sb;

	for (block = 0;block < global_blocks; block++) {

		k = 0;
		for (i = 0;i < global_bands;i++) {
			left[i] = input[k++];
			if (global_chan == 2)
				right[i] = input[k++];
		}
		input += k;

		for (chan = 0; chan < global_chan; chan++) {
			if (chan == 0) {
				X = L;
				data = left;
			} else {
				X = R;
				data = right;
			}

			for (i = (global_bands * 10) - 1;i > global_bands -1;
			    i--)
				X[i] = X[i - global_bands];
			k = 0;
			for (i = global_bands - 1; i >= 0; i--)
				X[i] = (int16_t)le16toh(data[k++]);
			for (i = 0; i < global_bands * 10; i++) {
				if (global_bands == 8) {
					Z[i] = (sbc_coeffs8[i] * (X[i] <<
					    global_volume)); 
				} else {
					Z[i] = (sbc_coeffs4[i] * (X[i] <<
					    global_volume)); 
				}
			}
			for (i = 0; i < global_bands * 2; i++) {
				Y[i] = 0;
				for (k = 0;k < 5;k++)
					Y[i] += Z[i + k * global_bands * 2];
			}
			for (i = 0; i < global_bands; i++) {
				S[i] = 0;
				for (k = 0; k < global_bands * 2; k++) {
					if (global_bands == 8) {
						S[i] += (int64_t)cosdata8[i][k]
						    * (int64_t)Y[k];
					} else {
						S[i] += (int64_t)cosdata4[i][k]
						    * (int64_t)Y[k];
					}
				}
				output[block][chan][i] = (int32_t)(S[i] /
				    SIMULTI);
			}
		}
	}

	calc_scalefactors(output);
	if (global_mode == MODE_JOINT)
		join = calc_scalefactors_joint(output);

	calc_bitneed();

	for(chan = 0; chan < global_chan; chan++) {
		for (sb = 0; sb < global_bands; sb++) {
			levels[chan][sb] = ((1 << bits[chan][sb]) - 1) <<
				(15 - scalefactor[chan][sb]);
			delta[chan][sb] = 1 << (scalefactor[chan][sb] + 16);
		}
	}

	numsamples = 0;
	for (block = 0; block < global_blocks; block++) {
		for (chan = 0; chan < global_chan; chan++) {
			for (sb = 0; sb < global_bands; sb++) {
				if (bits[chan][sb] == 0)
					continue;

				audioout = (int32_t)((levels[chan][sb] *
				    (delta[chan][sb] + (int32_t)output[block]
				    [chan][sb])) >> 32);

				samples[numsamples++] = audioout;
			}
		}
	}
	return numsamples;
}

size_t
sbc_decode(int32_t *samples, int16_t *pcm)
{
	static int64_t levels[2][8], delta[2][8];
	static int64_t  S[8], V[2][160], L[160], R[160];
	int64_t *X;
	static int64_t U[2][160], W[2][160];
	int64_t audioout;
	int chan, block, sb, position, i, k;
	size_t numsamples;

	for(chan = 0; chan < global_chan; chan++) {
		for (sb = 0; sb < global_bands; sb++) {
			levels[chan][sb] = (1 << bits[chan][sb]) - 1;
			delta[chan][sb] = 1 << (scalefactor[chan][sb] + 1);
		}
	}

	numsamples = 0;
	for (block = 0; block < global_blocks; block++) {
		for (chan = 0; chan < global_chan; chan++) {
			for (sb = 0; sb < global_bands; sb++) {
				if (bits[chan][sb] == 0)
					audioout = 0;
				else {
					audioout = ((((samples[numsamples]
					    * 2) + 1) * delta[chan][sb]) /
					    levels[chan][sb]) -
					    delta[chan][sb];
				}
				samples[numsamples] = (int32_t)audioout;
				numsamples++;
			}
		}
	}

	if (global_mode == MODE_JOINT) {
		k = 0;
		while (k < (global_blocks * global_bands * global_chan)) {
			for (sb = 0; sb < global_bands; sb++) {
				if (join & 1 << (global_bands - sb - 1)) {
					audioout = samples[k];
					samples[k] = (2 * samples[k]) + (2 *
					    samples[k + global_bands]);
					samples[k + global_bands] = 
					    (int32_t)(2 * audioout) - (2 *
					    samples[k + global_bands]);
					samples[k] /= 2;
					samples[k + global_bands] /= 2;
				}
				k++;
			}
			k += global_bands;
		}
	}


	position = 0;
	for (block = 0; block < global_blocks; block++) {
		for (chan = 0; chan < global_chan; chan++) {
			if (chan == 0)
				X = L;
			else
				X = R;

			for (i = 0; i < global_bands; i++)
				S[i] = samples[position++];
			for (i = ((global_bands * 20) - 1); i >= (global_bands
			    * 2); i--)
				V[chan][i] = V[chan][i - (global_bands * 2)];

			for (k = 0; k < global_bands * 2; k++) {
				V[chan][k] = 0;
				for (i = 0; i < global_bands; i++) {
					if (global_bands == 8) {
						V[chan][k] += cosdecdata8[i][k]
						    * S[i];
					} else {
						V[chan][k] += cosdecdata4[i][k]
						    * S[i];
					}
				}
				V[chan][k] /= SIMULTI;		
			}

			for (i = 0; i <= 4; i++) {
				for (k = 0; k < global_bands; k++) {
					U[chan][(i * global_bands * 2) + k] =
					    V[chan][(i * global_bands * 4) + k];
					U[chan][(i * global_bands
					    * 2) + global_bands + k] =
			    		V[chan][(i * global_bands * 4) +
					    (global_bands * 3) + k];
				}
			}
			
			for (i = 0; i < global_bands * 10; i++) {
				if (global_bands == 4) {
					W[chan][i] = U[chan][i] *
					    (sbc_coeffs4[i] *  -4);
				} else if (global_bands == 8) {
					W[chan][i] = U[chan][i] *
					    (sbc_coeffs8[i] *  -8);
				}
			}

			for (k = 0; k < global_bands; k++) {
				int offset = k + (block * global_bands);
				X[offset] = 0;
				for (i = 0; i < 10; i++) {
					X[offset] += W[chan][k + (i *
					    global_bands)];	
				}
				X[offset] /= COEFFSMULTI;
			}
		}
	}

	k = 0;
	i = 0;
	while (k < (global_blocks * global_bands)) {
		pcm[i++] = (int16_t)L[k];
		if (global_chan == 2)
			pcm[i++] = (int16_t)R[k];
		k++;
	}

	return numsamples;
}

uint8_t
Crc8(uint8_t inCrc, uint8_t *inData, size_t numbits, ssize_t inBytes)
{
	uint8_t data;
        int i;

	for (i = 0; i < (int)inBytes; i++) {
		data = inCrc ^ inData[i];
		
		if (numbits == 8)
			data = sbc_crc8[data];
		else if (numbits == 4)
			data = sbc_crc4[data];

		inCrc = data;
	}
	return inCrc;
}

uint8_t
make_crc(uint8_t config)
{
	uint8_t crc, data[11];
	int i, j;
	uint8_t *dataStart = data;
	uint8_t *crcData = data;


	crcData += move_bits_crc(crcData, 8, config);
	crcData += move_bits_crc(crcData, 8, global_bitpool);
	if (global_mode == MODE_JOINT) {
		if (global_bands == 8)
			crcData += move_bits_crc(crcData, 8, join);
		else
			crcData += move_bits_crc(crcData, 4, join);
	}

	for(i = 0; i < global_chan; i++) {
		for (j = 0; j < global_bands; j++)
			crcData += move_bits_crc(crcData, 4, scalefactor[i][j]);
	}

	crc = Crc8(0xf, data, 8, (crcData - dataStart));

	if (global_mode == MODE_JOINT && global_bands == 4) {
		move_bits_crc(crcData, 0, 0);
		crc = Crc8(crc, crcData, 4, 1);
	}

	return crc;
}

ssize_t
make_frame(uint8_t *frame, int16_t *input)
{
	static int32_t samples[256 * 2];
	uint8_t config, crc;
	int block, chan, sb, j, i;

	uint8_t *frameStart = frame;

	config = (uint8_t)(((3 - FLS(global_freq)) << 6) |
	    ((3 - FLS(global_block_config)) << 4) | ((3 - FLS(global_mode))
	    << 2) | ((FLS(global_alloc)) << 1) |
	    (1 - FLS(global_bands_config)));

	sbc_encode(input,samples);

	crc = make_crc(config);

	frame += move_bits(frame, 8, SYNCWORD);
	frame += move_bits(frame, 8, config);
	frame += move_bits(frame, 8, global_bitpool);
	frame += move_bits(frame, 8, crc);

	if (global_mode == MODE_JOINT && global_bands == 8)
		frame += move_bits(frame, 8, join);
	else if (global_mode == MODE_JOINT && global_bands == 4)
		frame += move_bits(frame, 4, join);

	for(i = 0; i < global_chan; i++) {
		for (j = 0; j < global_bands; j++)
			frame += move_bits(frame, 4, scalefactor[i][j]);
	}

	i = 0;
	for (block = 0; block < global_blocks; block++) {
		for (chan = 0; chan < global_chan; chan++) {
			for (sb = 0; sb < global_bands; sb++) {
				if (bits[chan][sb] == 0)
					continue;

				frame += move_bits(frame, bits[chan][sb],
				    (uint32_t)samples[i++]);
			}
		}
	}
	frame += move_bits(frame, 0, 0);

	return frame - frameStart;
}

static ssize_t
readloop(int fd, void *buf, size_t nbytes)
{
	size_t count;
	ssize_t ret;

	count = 0;
	while (nbytes > 0) {
		ret = read(fd, ((char *)buf) + count, nbytes);
		if (ret < 0) {
			if (count == 0)
				return ret;
			break;
		}
		if (ret == 0)
			break;
		count += (size_t)ret;
		nbytes -= (size_t)ret;
	}

	return (ssize_t) count;
}

ssize_t
stream(int in, int outfd, uint8_t mode, uint8_t freq, uint8_t bands, uint8_t
    blocks, uint8_t alloc_method, uint8_t bitpool, size_t mtu, int volume)
{
	struct rtpHeader myHeader;
	struct timeval myTime;
	uint8_t *whole, *frameData;
	int16_t music[2048];
	ssize_t len, mySize[16], offset, next_pkt;
	ssize_t pkt_len;
	size_t readsize, totalSize;
	size_t frequency;
	static size_t ts = 0;
	static uint16_t seqnumber = 0;
	static time_t prevTime, readTime, sleepTime, timeNow;
	int numpkts, tries;

	global_mode = mode;
	global_bitpool = bitpool;
	global_alloc = alloc_method;
	global_freq = freq;
	global_volume = volume;

	global_bands_config = bands;
	if (bands == BANDS_8)
		global_bands = 8;
	else
		global_bands = 4;

	if (blocks == BLOCKS_4)
		global_blocks = 4;
	else if (blocks == BLOCKS_8)
		global_blocks = 8;
	else if (blocks == BLOCKS_12)
		global_blocks = 12;
	else {
		blocks = BLOCKS_16;
		global_blocks = 16;
	}

	global_block_config = blocks;

	global_chan = 2;
	if (global_mode == MODE_MONO)
		global_chan = 1;

	if (global_freq == FREQ_16K)
		frequency = 16000;
	else if (global_freq == FREQ_32K)
		frequency = 32000;
	else if (global_freq == FREQ_48K)
		frequency = 48000;
	else
		frequency = 44100;

	memset(&myHeader, 0, sizeof(myHeader));
	myHeader.id = 0x80;	/* RTP v2 */
	myHeader.id2 = 0x60; 	/* payload type 96. */
	myHeader.seqnumMSB = (uint8_t)(seqnumber >> 8);
	myHeader.seqnumLSB = (uint8_t)seqnumber;
	myHeader.ts3 = (uint8_t)(ts >> 24);
	myHeader.ts2 = (uint8_t)(ts >> 16);
	myHeader.ts1 = (uint8_t)(ts >> 8);
	myHeader.ts0 = (uint8_t)ts;
	myHeader.reserved0 = 0x01;

	totalSize = sizeof(myHeader);

	frameData = malloc(mtu);
	if (frameData == NULL)
		return -1;

	readsize = (size_t)((global_blocks * global_bands * global_chan) * 2);

	numpkts = 0;
	next_pkt = 0;
	len = 0;
	pkt_len = 80;
	readTime = 0;
	while (totalSize + ((size_t)pkt_len * 2) <= mtu) {

		len = readloop(in, music, readsize);
		readTime += (time_t)readsize;
		if (len < (int)readsize)
			break;

		pkt_len = make_frame(frameData + next_pkt, music);

		mySize[numpkts] = pkt_len;
		next_pkt += pkt_len;
		totalSize += (size_t)mySize[numpkts];
		numpkts++;

		if (numpkts > 12)
			break;
	}

	if (len < (int)readsize) {
		free(frameData);
		return -1;
	}

	readTime = readTime * 1000000 / 2 / global_chan / (time_t)frequency;

	myHeader.numFrames = (uint8_t)numpkts;
	whole = malloc(totalSize);
	if (whole == NULL)
		return -1;

	memcpy(whole, &myHeader, sizeof(myHeader));
	offset = sizeof(myHeader);

	memcpy(whole + offset, frameData, (size_t)next_pkt);
	free(frameData);

	/* Wait if necessary to avoid rapid playback. */
	gettimeofday(&myTime, NULL);
	timeNow = myTime.tv_sec * 1000000 + myTime.tv_usec;
	if (prevTime == 0)
		prevTime = timeNow;
	else
		sleepTime += readTime - (timeNow - prevTime);
	if (sleepTime >= 1000) {
		usleep(500);
		sleepTime -= 1000;
	}
	prevTime = timeNow;

	tries = 1;
send_again:
	len = write(outfd, whole, totalSize);

	if (len == -1 && errno == EAGAIN) {
		tries --;
		if (tries >= 0) {
			usleep(1);
			goto send_again;
		} else
			len = (ssize_t)totalSize;
	} else if (len == -1 && (errno == EINPROGRESS ||
	    errno == EWOULDBLOCK)) {
		usleep(1);
			len = (ssize_t)totalSize;
	}

	seqnumber++;
	ts += (1000000 * (size_t)(global_blocks * global_bands)
	    / frequency) * (size_t)numpkts;

	free(whole);

	return len;
}

ssize_t
recvstream(int in, int outfd)
{
	struct rtpHeader myHeader;
	static uint8_t frameData[8192];
	uint8_t *myFrame;
	static int16_t music[4096];
	size_t decsize, totalSize, offset;
	int numpkts, cur_pkt;
	ssize_t len, res, pkt_len, next_pkt;
	static ssize_t readlen = 0;
	totalSize = 0;

	len = read(in, frameData + readlen, (size_t)(1000 - readlen));
	readlen += len;
	if (readlen <= 0) {
		readlen = 0;
		return -1;
	}

	if (readlen < (int)sizeof(myHeader)) {
		readlen = 0;
		return -1;
	}

	memcpy(&myHeader, frameData, sizeof(myHeader));
	if (myHeader.id != 0x80) {
		return -1;
	}

	numpkts = myHeader.numFrames;
	if (numpkts < 1 || numpkts > 13) {
		return -1;
	}

	myFrame = frameData + sizeof(myHeader);
	next_pkt = 0;
	pkt_len = 0;
	cur_pkt = 0;
	offset = 0;
	while (cur_pkt < numpkts) {
		pkt_len = parseFrame(myFrame + next_pkt, &music[offset]);
		decsize = (size_t)(global_blocks * global_bands * global_chan);

		next_pkt += pkt_len;
		totalSize += 2 * decsize;
		offset += decsize;
		cur_pkt++;
	}

	res = (ssize_t)(sizeof(myHeader)) + next_pkt;
		readlen -= res;
	if (readlen > 0)
		memcpy(frameData, frameData + res, (size_t)readlen);

send_again:
	len = write(outfd, music, totalSize);

	if (len == -1 && errno == EAGAIN)
		goto send_again;

	return len;
}

ssize_t
parseFrame(uint8_t *myFrame, int16_t *pcm)
{
	uint8_t mode, bitpool, alloc_method, freq, bands, config, myCrc, blkCrc;
	int sb, chan, block, blocks, i, j;
	int32_t samples[768];

	uint8_t *myFrame_start = myFrame;
	if (*myFrame++ != SYNCWORD)
		return -1;

	config = *myFrame++;
	bitpool = *myFrame++;
	myCrc = *myFrame++;

	freq = (uint8_t)(1 << (3 - ((config & 0xc0) >> 6)));
	blocks = 1 << (3 - ((config & 0x30) >> 4));
	mode = (uint8_t)(1 << (3 - ((config & 0x0c) >> 2)));
	alloc_method = (uint8_t)(1 << (((config & 0x02) >> 1)));
	bands = (uint8_t)(1 << (1 - (config & 0x01)));

	global_mode = mode;
	global_bitpool = bitpool;
	global_alloc = alloc_method;
	global_freq = freq;

	global_bands_config = bands;
	if (bands == BANDS_8)
		global_bands = 8;
	else
		global_bands = 4;

	if (blocks == BLOCKS_4)
		global_blocks = 4;
	else if (blocks == BLOCKS_8)
		global_blocks = 8;
	else if (blocks == BLOCKS_12)
		global_blocks = 12;
	else {
		blocks = BLOCKS_16;
		global_blocks = 16;
	}

	global_block_config = (uint8_t)blocks;

	global_chan = 2;
	if (global_mode == MODE_MONO)
		global_chan = 1;

	if (global_mode == MODE_JOINT && global_bands == 8)
		myFrame += get_bits(myFrame, 8, (uint32_t *)&join);
	else if (global_mode == MODE_JOINT && global_bands == 4)
		myFrame += get_bits(myFrame, 4, (uint32_t *)&join);
	else
		join = 0;

	for(i = 0; i < global_chan; i++) {
		for (j = 0; j < global_bands; j++) {
			myFrame += get_bits(myFrame, 4,
			    (uint32_t *)&scalefactor[i][j]);
		}
	}

	blkCrc = make_crc(config);
	if (blkCrc != myCrc)
		return 0;

	calc_bitneed();
	i = 0;
	for (block = 0; block < global_blocks; block++) {
		for (chan = 0; chan < global_chan; chan++) {
			for (sb = 0; sb < global_bands; sb++) {
				samples[i] = 0;
				if (bits[chan][sb] == 0) {
					i++;
					continue;
				}

				myFrame += get_bits(myFrame, bits[chan][sb],
				    (uint32_t *)&samples[i++]);
			}
		}
	}
	myFrame += get_bits(myFrame, 0, (uint32_t *)&samples[i]);
	sbc_decode(samples, pcm);

	return myFrame - myFrame_start;
}
