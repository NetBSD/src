/* $NetBSD: avdtp_signal.h,v 1.1 2017/01/28 16:55:54 nat Exp $ */

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

#include <sys/types.h>
#include <stdbool.h>
/* Our endpoint. */
#define INTSEP				8

/* AVDTP signals. */

#define	AVDTP_DISCOVER			0x01
#define	AVDTP_GET_CAPABILITIES		0x02
#define	AVDTP_SET_CONFIGURATION		0x03
#define	AVDTP_GET_CONFIGURATION		0x04
#define	AVDTP_RECONFIGURE		0x05
#define	AVDTP_OPEN			0x06
#define	AVDTP_START			0x07
#define	AVDTP_CLOSE			0x08
#define	AVDTP_SUSPEND			0x09
#define	AVDTP_ABORT			0x0a
#define	AVDTP_SECUURITY_CONTROL		0x0b

/* Signal Command & Response Header Masks. */

#define TRANSACTIONLABEL		0xf0
#define TRANSACTIONLABEL_S		4
#define SIGNALID_MASK			0x3f
#define PACKETTYPE			0x0c
#define PACKETTYPE_S			0x02
#define MESSAGETYPE			0x03
#define	SIGNALIDENTIFIER		0x3f
#define DISCOVER_SEP_IN_USE		0x02

/* Packet Types */
#define singlePacket			0x0
#define startPacket			0x1
#define continuePacket			0x2
#define endPacket			0x3

/* Message Types */
#define COMMAND				0x0
#define RESPONSEACCEPT			0x2
#define RESPONSEREJECT			0x3

/* Response general error/success lengths */
#define AVDTP_LEN_SUCCESS		2
#define AVDTP_LEN_ERROR			3

/* Error codes */
#define BAD_HEADER_FORMAT		0x01
#define BAD_LENGTH			0x11
#define BAD_ACP_SEID			0x12
#define SEP_IN_USE			0x13
#define SEP_NOT_IN_USE			0x14
#define BAD_SERV_CATAGORY		0x17
#define BAD_PAYLOAD_FORMAT		0x18
#define NOT_SUPPORTED_COMMAND		0x19
#define INVALID_CAPABILITIES		0x1a

#define BAD_RECOVERY_TYPE		0x22
#define BAD_MEDIA_TRANSPORT_FORMAT	0x23
#define BAD_RECOVERY_FORMAT		0x25
#define BAD_ROHC_FORMAT			0x26
#define BAD_CP_FORMAT			0x27
#define BAD_MULTIPLEXING_FORMAT		0x28
#define UNSUPPORTED_CONFIGURATION	0x29
#define BAD_STATE			0x31

/* Service Capabilities Field. */
#define mediaTransport			0x1
#define reporting			0x2
#define recovery			0x3
#define contentProtection		0x4
#define headerCompression		0x5
#define multiplexing			0x6
#define mediaCodec			0x7

/* Media Codec Capabilities */
#define mediaType			0xf0

struct avdtp_sepInfo {
	uint8_t sep;
	uint8_t media_Type;
};

int avdtpSendCommand(int fd, uint8_t command, uint8_t type,
    uint8_t *data, size_t datasize);
int avdtpCheckResponse(int recvfd, bool *isCommand, uint8_t *trans, uint8_t
    *signalId, uint8_t *pkt_type, uint8_t *data, size_t *datasize, uint8_t
    *sep);
int avdtpDiscover(uint8_t *buffer, size_t recvsize, struct avdtp_sepInfo
    *sepInfo, bool sink);
void avdtpGetCapabilities(int fd, int recvfd, uint8_t sep);
int avdtpAutoConfigSBC(int fd, int recvfd, uint8_t * capabilities, size_t
    caplen, uint8_t sep, u_int8_t *freq, u_int8_t *mode, u_int8_t *alloc_method,
    u_int8_t *bitpool, u_int8_t* bands, u_int8_t *blocks, u_int8_t srcsep);
int avdtpSetConfiguration(int fd, int recvfd, uint8_t sep, uint8_t *data,
    size_t datasize, int srcsep);
void avdtpOpen(int fd, int recvfd, uint8_t sep);
void avdtpStart(int fd, int recvfd, uint8_t sep);
void avdtpClose(int fd, int recvfd, uint8_t sep);
void avdtpSuspend(int fd, int recvfd, uint8_t sep);
void avdtpAbort(int fd, int recvfd, uint8_t sep);
int avdtpSendDiscResponseAudio(int, int, uint8_t, uint8_t, bool);
int avdtpSendCapabilitiesResponseSBC(int, int, int, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
int avdtpSendAccept(int, int, uint8_t, uint8_t);
int avdtpSendReject(int, int, uint8_t, uint8_t);

