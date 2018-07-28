/* $NetBSD: avdtp.c,v 1.1.14.1 2018/07/28 04:38:14 pgoyette Exp $ */

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

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "avdtp_signal.h"
#include "sbc_encode.h"

static uint8_t transLabel = 1;
int
avdtpSendCommand(int fd, uint8_t command, uint8_t type, uint8_t *data,
    size_t datasize)
{
#define SINGLE_PACKET 0
#define START_PACKET 1
#define CONTINUE_PACKET 2
#define END_PACKET 3
#define signalID 3

	uint8_t header[64];
	size_t extra_size = 0;
	const uint8_t packetType = (SINGLE_PACKET & 3) << 2;
	const uint8_t messageType = (type & 3);

	transLabel &= 0xf;

	header[0] = (uint8_t)((transLabel << 4) | packetType | messageType);
	if (command != 0)
		header[1] = command & 0x3f;
	else
		header[1] = signalID & 0x3f; /* Bits 7/6 Reserved */

	transLabel++;
	if (data != NULL) {
		extra_size = datasize;
		memcpy(header + 2, data, extra_size);
	}
	write(fd, &header, extra_size + 2);

	return transLabel - 1;
}

int
avdtpCheckResponse(int recvfd, bool *isCommand, uint8_t *trans, uint8_t
    *signalId, uint8_t *pkt_type, uint8_t *data, size_t *datasize, uint8_t *sep)
{
	uint8_t buffer[1024];
	size_t len;

	*isCommand = false;
	len = (size_t)read(recvfd, buffer, sizeof(buffer));

	if (datasize)
		*datasize = 0;

	if (len < AVDTP_LEN_SUCCESS)
		return ENOMEM;

	*trans = (uint8_t)((buffer[0] & TRANSACTIONLABEL) >> TRANSACTIONLABEL_S);
	*signalId = buffer[1] & SIGNALID_MASK;
	if ((buffer[0] & MESSAGETYPE) == COMMAND) {
		if (datasize)
			*datasize = 0;
		if (sep && len > 2)
			*sep = buffer[2] >> 2;
		*isCommand = true;
	}

	if (len == AVDTP_LEN_ERROR)
		return buffer[2];
	else if ((len % AVDTP_LEN_SUCCESS) == 0 &&
	    buffer[0] & RESPONSEACCEPT) {
		if (len == AVDTP_LEN_SUCCESS)
			return 0;
	}
	if (datasize && data && len > AVDTP_LEN_SUCCESS &&
	    buffer[0] & RESPONSEACCEPT) {
		memcpy(data, buffer + 2, len - 2);
		*datasize = len - 2;

		return 0;
	}

	if (isCommand)
		return 0;

	return EINVAL;
}
	
int
avdtpSendCapabilitiesResponseSBC(int fd, int recvfd, int trans, uint8_t mySep,
    uint8_t bitpool, uint8_t freq, uint8_t mode, uint8_t bands, uint8_t blocks,
    uint8_t alloc_method)
{
	uint8_t data[12], freqmode, blk_len_sb_alloc, freq_dat, mode_dat;
	uint8_t bands_dat, blocks_dat, alloc_dat;

	
	freq_dat = (uint8_t)(freq << 4);
	mode_dat = mode;
	freqmode = freq_dat | mode_dat;

	blocks_dat = (uint8_t)(blocks << 4);
	bands_dat = (uint8_t)(bands << 2);
	alloc_dat = alloc_method;
	blk_len_sb_alloc = blocks_dat| bands_dat | alloc_dat;

	data[0] = (uint8_t)(trans << 4 | RESPONSEACCEPT);
	data[1] = AVDTP_GET_CAPABILITIES;
	data[2] = mediaTransport;
	data[3] = 0;
	data[4] = mediaCodec;
	data[5] = 0x6;
	data[6] = mediaTypeAudio;
	data[7] = SBC_CODEC_ID;
	data[8] = freqmode;
	data[9] = blk_len_sb_alloc;
	data[10] = MIN_BITPOOL;
	if (bitpool > MIN_BITPOOL)
		data[11] = bitpool;
	else
		data[11] = DEFAULT_MAXBPOOL;

	write(fd, data, sizeof(data));

	return 0;
}

int
avdtpSendAccept(int fd, int recvfd, uint8_t trans, uint8_t myCommand)
{
	uint8_t data[2];

	data[0] = (uint8_t)(trans << 4 | RESPONSEACCEPT);
	data[1] = myCommand;;

	write(fd, data, sizeof(data));

	return 0;
}

int
avdtpSendReject(int fd, int recvfd, uint8_t trans, uint8_t myCommand)
{
	uint8_t data[4];

	data[0] = (uint8_t)(trans << 4 | RESPONSEREJECT);
	data[1] = myCommand;
	data[2] = 0;

	write(fd, data, sizeof(data));

	return 0;
}

int
avdtpSendDiscResponseAudio(int fd, int recvfd, uint8_t trans, uint8_t mySep,
    bool sink)
{
	uint8_t data[4];

	data[0] = (uint8_t)(trans << 4 | RESPONSEACCEPT);
	data[1] = AVDTP_DISCOVER;
	data[2] = (uint8_t)(mySep << 2);
	data[3] = (uint8_t)((sink ? 1 : 0) << 3);

	write(fd, data, sizeof(data));

	return 0;
}

int
avdtpDiscover(uint8_t *buffer, size_t recvsize,  struct avdtp_sepInfo *sepInfo,
    bool sink)
{
	size_t offset;
	bool isSink;

	if (recvsize >= 2) {
		for (offset = 0;offset < recvsize;offset += 2) {
			sepInfo->sep = buffer[offset] >> 2;
			sepInfo->media_Type = buffer[offset+1] >> 4;
			isSink = (buffer[offset+1] >> 3) & 1;
			if (buffer[offset] & DISCOVER_SEP_IN_USE ||
			     isSink != sink)
				continue;
			else
				break;
		}
		if (offset > recvsize)
			return EINVAL;

		return 0;
	}

	return EINVAL;
}

void
avdtpGetCapabilities(int fd, int recvfd, uint8_t sep)
{
	uint8_t address = (uint8_t)(sep << 2);

	avdtpSendCommand(fd, AVDTP_GET_CAPABILITIES, 0, &address, 1);
}

int
avdtpSetConfiguration(int fd, int recvfd, uint8_t sep, uint8_t *data,
    size_t datasize, int srcsep)
{
	uint8_t configAddresses[2];
	uint8_t *configData;

	if (data == NULL || datasize == 0)
		return EINVAL;

	configData = malloc(datasize + 2);
	if (configData == NULL)
		return ENOMEM;
	configAddresses[0] = (uint8_t)(sep << 2);
	configAddresses[1] = (uint8_t)(srcsep << 2);

	memcpy(configData, configAddresses, 2);
	memcpy(configData + 2, data, datasize);

	avdtpSendCommand(fd, AVDTP_SET_CONFIGURATION, 0,
	    configData, datasize + 2); 
	free(configData);

	return 0;

}

void
avdtpOpen(int fd, int recvfd, uint8_t sep)
{
	uint8_t address = (uint8_t)(sep << 2);

	avdtpSendCommand(fd, AVDTP_OPEN, 0, &address, 1);
}

void
avdtpStart(int fd, int recvfd, uint8_t sep)
{
	uint8_t address = (uint8_t)(sep << 2);

	avdtpSendCommand(fd, AVDTP_START, 0, &address, 1);
}

void
avdtpClose(int fd, int recvfd, uint8_t sep)
{
	uint8_t address = (uint8_t)(sep << 2);

	avdtpSendCommand(fd, AVDTP_CLOSE, 0, &address, 1);
}

void
avdtpSuspend(int fd, int recvfd, uint8_t sep)
{
	uint8_t address = (uint8_t)(sep << 2);

	avdtpSendCommand(fd, AVDTP_SUSPEND, 0, &address, 1);
}

void
avdtpAbort(int fd, int recvfd, uint8_t sep)
{
	uint8_t address = (uint8_t)(sep << 2);

	avdtpSendCommand(fd, AVDTP_ABORT, 0, &address, 1);
}

int
avdtpAutoConfigSBC(int fd, int recvfd, uint8_t *capabilities, size_t cap_len,
    uint8_t sep, uint8_t *freq, uint8_t *mode, uint8_t *alloc_method, uint8_t
    *bitpool, uint8_t* bands, uint8_t *blocks, uint8_t srcsep)
{
	uint8_t freqmode, blk_len_sb_alloc, availFreqMode, availConfig; 
	uint8_t supBitpoolMin, supBitpoolMax, tmp_mask;
	size_t i;

	for (i = 0; i < cap_len; i++) {
		if (capabilities[i] == mediaTransport &&
		    capabilities[i + 1] == 0 &&
		    capabilities[i + 2] == mediaCodec &&
		    capabilities[i + 4] == mediaTypeAudio &&
		    capabilities[i + 5] == SBC_CODEC_ID)
			break;
	}
	if (i >= cap_len)
		goto auto_config_failed;

	availFreqMode = capabilities[i + 6];
	availConfig = capabilities[i + 7];
	supBitpoolMin = capabilities[i + 8];
	supBitpoolMax = capabilities[i + 9];

	freqmode = (uint8_t)(*freq << 4 | *mode);
	tmp_mask = availFreqMode & freqmode;
	*mode = (uint8_t)(1 << FLS(tmp_mask & 0xf));
	*freq = (uint8_t)(1 << FLS(tmp_mask >> 4));
	 
	freqmode = (uint8_t)(*freq << 4 | *mode);
	if ((availFreqMode & freqmode) != freqmode)
		goto auto_config_failed;

	blk_len_sb_alloc = (uint8_t)(*blocks << 4 | *bands << 2 |
	    *alloc_method);

	tmp_mask = availConfig & blk_len_sb_alloc;
	*blocks = (uint8_t)(1 << FLS(tmp_mask >> 4));
	*bands = (uint8_t)(1 << FLS((tmp_mask >> 2) & 3));
	*alloc_method = (uint8_t)(1 << FLS(tmp_mask & 3));

	blk_len_sb_alloc = (uint8_t)(*blocks << 4 | *bands << 2 |
	    *alloc_method);

	if ((availConfig & blk_len_sb_alloc) != blk_len_sb_alloc)
		goto auto_config_failed;
	
	if (*alloc_method == ALLOC_SNR)
		supBitpoolMax &= (uint8_t)~1;

	if (*mode == MODE_DUAL || *mode == MODE_MONO)
		supBitpoolMax /= 2;

	if (*bands == BANDS_4)
		supBitpoolMax /= 2;

	if (supBitpoolMax > *bitpool)
		supBitpoolMax = *bitpool;
	else
		*bitpool = supBitpoolMax;

	uint8_t config[] = {mediaTransport, 0x0, mediaCodec, 0x6,
	    mediaTypeAudio, SBC_CODEC_ID, freqmode, blk_len_sb_alloc,
	    supBitpoolMin, supBitpoolMax};

	if (avdtpSetConfiguration(fd, fd, sep, config, sizeof(config),
	    srcsep) == 0)
		return 0;

auto_config_failed:
	return EINVAL;
}
