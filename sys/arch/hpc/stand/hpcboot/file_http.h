/* -*-C++-*-	$NetBSD: file_http.h,v 1.1.10.1 2002/04/01 07:40:13 nathanw Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#ifndef _HPCBOOT_FILE_HTTP_H_
#define _HPCBOOT_FILE_HTTP_H_

#include <file.h>
#include <winsock.h>

class Socket {
private:
	struct sockaddr_in &_sockaddr;
	SOCKET _socket;
public:
	explicit Socket(struct sockaddr_in &);
	operator SOCKET() const { return _socket; }
	virtual ~Socket(void);
};

class HttpFile : public File {
public:
	enum ops {
		FILE_CACHE	= 0x00000001,
		USE_PROXY	= 0x00000002		// XXX not yet.
	};

private:
	// Wrapper for absorb Windows CE API difference.
	int (*_wsa_startup)(WORD, LPWSADATA);
	int (*_wsa_cleanup)(void);

private:
	// IP Socket
	struct WSAData _winsock;
	struct sockaddr_in _sockaddr;

	enum { TMP_BUFFER_SIZE = 256 };

	char _server_name[MAX_PATH];

	// HTTP request
	char _request[MAX_PATH];
	const char *_req_get;
	const char *_req_head;
	const char *_req_host;
	const char *_req_ua;

	void _set_request(void);

	// HTTP header.
	size_t _header_size;
	size_t _parse_header(size_t &);

	// File buffer.
	BOOL _memory_cache;
	BOOL _cached;
	char *_buffer;
	size_t _buffer_size;
	size_t _read_from_cache(void *, size_t, off_t);
	size_t _recv_buffer(SOCKET, char *, size_t);
	void _reset_state(void);
	off_t _cur_pos;

public:
	HttpFile::HttpFile(Console *&);
	virtual ~HttpFile(void);

	BOOL setRoot(TCHAR *);
	BOOL open(const TCHAR *, u_int32_t);
	size_t size(void) { size_t hsz; return _parse_header(hsz); }
	BOOL close(void) { return TRUE; }
	size_t read(void *, size_t, off_t = -1);
	size_t write(const void *, size_t, off_t = -1) { return 0; }
	BOOL seek(off_t);

};

#endif //_HPCBOOT_FILE_HTTP_H_
