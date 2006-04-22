/*	$NetBSD: file_http.cpp,v 1.10.6.1 2006/04/22 11:37:28 simonb Exp $	*/

/*-
 * Copyright (c) 2001, 2002, 2004 The NetBSD Foundation, Inc.
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

#include <file.h>
#include <file_http.h>
#include <console.h>
#include <libsa_string.h>

#define	wcsicmp		_wcsicmp
static int WCE210_WSAStartup(WORD, LPWSADATA);
static int WCE210_WSACleanup(void);

HttpFile::HttpFile(Console *&cons)
	: File(cons),
	  _req_get("GET "),
	  _req_head("HEAD "),
	  _req_host(" HTTP/1.0\r\nHOST: "),
	  _req_ua("\r\nUser-Agent: HPCBOOT/ZERO(1st impact; Windows CE; "
#if defined MIPS
	      "MIPS"
#elif defined ARM
	      "ARM"
#elif defined SH3
	      "SH3"
#elif defined SH4
	      "SH4"
#else
	      "Unknown"
#endif
	      ")\r\n\r\n")
{

	_server_name[0] = '\0';
	_debug = 1;
	_memory_cache = TRUE;
	//    _memory_cache = FALSE; // not recomended.
	_buffer = 0;
	_reset_state();
	DPRINTF((TEXT("FileManager: HTTP\n")));

#if _WIN32_WCE <= 200
	_wsa_startup = WCE210_WSAStartup;
	_wsa_cleanup = WCE210_WSACleanup;
#else
	if (WinCEVersion.dwMajorVersion > 3 ||
	    (WinCEVersion.dwMajorVersion > 2) &&
	    (WinCEVersion.dwMinorVersion >= 11)) {
		_wsa_startup = WSAStartup;
		_wsa_cleanup = WSACleanup;
	} else {
		_wsa_startup = WCE210_WSAStartup;
		_wsa_cleanup = WCE210_WSACleanup;
	}
#endif
}

int
WCE210_WSAStartup(WORD ver, LPWSADATA data)
{

	data->wVersion		= ver;
	data->wHighVersion	= ver;
	data->szDescription[0]	= '\0';
	data->szSystemStatus[0]	= '\0';
	data->iMaxSockets	= 10;
	data->iMaxUdpDg		= 0;
	data->lpVendorInfo	= NULL;

	return (0);
}

int
WCE210_WSACleanup()
{

	return (0);
}

HttpFile::~HttpFile(void)
{
	if (_buffer)
		free(_buffer);
	_wsa_cleanup();
}

void
HttpFile::_reset_state(void)
{
	_ascii_filename[0] = '\0';
	_cached = FALSE;
	if (_buffer)
		free(_buffer);
	_buffer = 0;
	_header_size = 0;
	_cur_pos = 0;
}

BOOL
HttpFile::setRoot(TCHAR *server)
{
	SOCKET h;
	int ret, port;

	// parse server name and its port #
	TCHAR sep[] = TEXT(":/");

	TCHAR *token = wcstok(server, sep);
	for (int i = 0; i < 3 && token; i++, token = wcstok(0, sep)) {
		switch(i) {
		case 0:
			if (wcsicmp(token, TEXT("http"))) {
				return FALSE;
			}
			break;
		case 1:
			if (!_to_ascii(_server_name, token, MAX_PATH))
				return FALSE;
			port = 80;
			break;
		case 2:
			port = _wtoi(token);
			break;
		}
	}

	WORD version = MAKEWORD(1, 1);
	ret = _wsa_startup(version, &_winsock);
	if (ret != 0) {
		DPRINTF((TEXT("WinSock initialize failed.\n")));
		return FALSE;
	}
	if (LOBYTE(_winsock.wVersion) != 1 ||
	    HIBYTE(_winsock.wVersion) != 1) {
		DPRINTF((TEXT("can't use WinSock DLL.\n")));
		return FALSE;
	}

	h = socket(AF_INET, SOCK_STREAM, 0);
	if (h == INVALID_SOCKET) {
		DPRINTF((TEXT("can't open socket. cause=%d\n"),
		    WSAGetLastError()));
		return FALSE;
	}

	memset(&_sockaddr, 0, sizeof(sockaddr_in));
	_sockaddr.sin_family = AF_INET;
	_sockaddr.sin_port = htons(port);

	struct hostent *entry = gethostbyname(_server_name);
	if (entry == 0) {
		_sockaddr.sin_addr.S_un.S_addr = inet_addr(_server_name);
		if (_sockaddr.sin_addr.S_un.S_addr == INADDR_NONE) {
			DPRINTF((TEXT("can't get host by name.\n")));
			return FALSE;
		}
		uint8_t *b = &_sockaddr.sin_addr.S_un.S_un_b.s_b1;
		DPRINTF((TEXT("%d.%d.%d.%d "), b[0], b[1], b[2], b[3]));
		if (connect(h,(const struct sockaddr *)&_sockaddr,
		    sizeof(struct sockaddr_in)) == 0)
			goto connected;
	} else {
		for (uint8_t **addr_list =(uint8_t **)entry->h_addr_list;
		    *addr_list; addr_list++) {
			uint8_t *b = &_sockaddr.sin_addr.S_un.S_un_b.s_b1;
			for (int i = 0; i < 4; i++)
				b[i] = addr_list[0][i];

			DPRINTF((TEXT("%d.%d.%d.%d "), b[0], b[1], b[2],b[3]));
			if (connect(h,(const struct sockaddr *)&_sockaddr,
			    sizeof(struct sockaddr_in)) == 0)
				goto connected;
		}
	}
	DPRINTF((TEXT("can't connect server.\n")));
	return FALSE;

 connected:
	DPRINTF((TEXT("(%S) connected.\n"), _server_name));
	closesocket(h);

	return TRUE;
}

BOOL
HttpFile::open(const TCHAR *name, uint32_t flag)
{

	_reset_state();

	return _to_ascii(_ascii_filename, name, MAX_PATH);
}

size_t
HttpFile::_read_from_cache(void *buf, size_t bytes, off_t ofs)
{
	size_t transfer;

	if (ofs >= _buffer_size)
		return 0;

	transfer = ofs + bytes > _buffer_size ? _buffer_size - ofs : bytes;

	memcpy(buf, &_buffer[ofs], transfer);

	return transfer;
}

BOOL
HttpFile::seek(off_t offset)
{
	_cur_pos = offset;

	return TRUE;
}

size_t
HttpFile::read(void *buf, size_t bytes, off_t offset)
{
	char *b;
	off_t ofs;

	if (offset != -1) {
		ofs = offset;
	} else {
		ofs = _cur_pos;
		_cur_pos += bytes;
	}

	if (_memory_cache && _cached)
		return _read_from_cache(buf, bytes, ofs);

	// HEAD request(get header size).
	if (_header_size == 0)
		_buffer_size = _parse_header(_header_size);

	// reconnect
	Socket sock(_sockaddr);
	SOCKET h;
	if ((h = sock) == INVALID_SOCKET)
		return 0;

	// GET request
	strcpy(_request, _req_get);
	_set_request();
	send(h, _request, strlen(_request), 0);

	// skip header.
	b = static_cast <char *>(malloc(_header_size));
	_recv_buffer(h, b, _header_size);
	free(b);

	// read contents.
	size_t readed;
	if (_memory_cache) {
		_buffer = static_cast <char *>(malloc(_buffer_size));
		_recv_buffer(h, _buffer, _buffer_size);
		_cached = TRUE;
		return _read_from_cache(buf, bytes, ofs);
	} else {
		int i, n = ofs / bytes;
		b = static_cast <char *>(buf);

		for (readed = 0, i = 0; i < n; i++)
			readed += _recv_buffer(h, b, bytes);
		if ((n =(ofs % bytes)))
			readed += _recv_buffer(h, b, n);
		DPRINTF((TEXT("skip contents %d byte.\n"), readed));

		readed = _recv_buffer(h, b, bytes);
	}
	return readed;
}

size_t
HttpFile::_parse_header(size_t &header_size)
{
	int cnt, ret;
	char *buf;
	size_t sz = 0;

	// reconnect.
	Socket sock(_sockaddr);
	SOCKET h;
	if ((h = sock) == INVALID_SOCKET) {
		DPRINTF((TEXT("can't open socket.\n")));
		return 0;
	}

	// HEAD request
	strcpy(_request, _req_head);
	_set_request();
	send(h, _request, strlen(_request), 0);

	// Receive and search Content-Length: field.
	if ((buf = static_cast<char *>(malloc(TMP_BUFFER_SIZE))) == 0) {
		DPRINTF((TEXT("can't allocate receive buffer.\n")));
		return 0;
	}

	BOOL found = FALSE;
	for (cnt = 0; ret = _recv_buffer(h, buf, TMP_BUFFER_SIZE - 1);
	    cnt += ret) {
		buf[ret] = '\0';
		char sep[] = " :\r\n";
		char *token = libsa::strtok(buf, sep);
		while (token) {
			DPRINTFN(2, (TEXT("+token: %S\n"), token));
			if (libsa::stricmp(token, "content-length") == 0) {
				DPRINTFN(2, (TEXT("*token: %S\n"), token));
				token = libsa::strtok(0, sep);
				sz = atoi(token);
				found = TRUE;
				DPRINTFN(2, (TEXT("*content-length=%d\n"), sz));
			} else {
				token = libsa::strtok(0, sep);
			}
		}
	}
	header_size = cnt;

	if (!found) {
		DPRINTF((TEXT("No Content-Length.\n")));
	} else {
		DPRINTF((TEXT
		    ("open http://%S%S - header %d byte contents %d byte\n"),
		    _server_name, _ascii_filename, header_size, sz));
	}
	free(buf);

	return sz;
}

size_t
HttpFile::_recv_buffer(SOCKET h, char *buf, size_t size)
{
	size_t cnt, total = 0;

	do {
		cnt = recv(h, buf + total, size - total, 0);
		total += cnt;
		DPRINTFN(2,(TEXT("size %d readed %d byte(+%d)\n"),
		    size, total, cnt));
	} while (total < size && cnt > 0);

	DPRINTFN(1,(TEXT("total read %d byte\n"), total));
	return total;
}

void
HttpFile::_set_request(void)
{

	strcat(_request, _ascii_filename);
	strcat(_request, _req_host);
	strcat(_request, _server_name);
	strcat(_request, _req_ua);
}

Socket::Socket(struct sockaddr_in &sock)
	: _sockaddr(sock)
{

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket != INVALID_SOCKET)
		connect(_socket,
		    reinterpret_cast <const struct sockaddr *>(&_sockaddr),
		    sizeof(struct sockaddr_in));
}

Socket::~Socket(void)
{

	if (_socket != INVALID_SOCKET)
		closesocket(_socket);
}
