/*	$NetBSD: proxy2.c,v 1.2 2025/01/26 16:25:38 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <isc/proxy2.h>

enum isc_proxy2_states {
	ISC_PROXY2_STATE_WAITING_SIGNATURE,
	ISC_PROXY2_STATE_WAITING_HEADER,
	ISC_PROXY2_STATE_WAITING_PAYLOAD, /* Addresses and TLVs */
	ISC_PROXY2_STATE_END
};

static inline void
isc__proxy2_handler_init_direct(isc_proxy2_handler_t *restrict handler,
				const uint16_t max_size,
				const isc_region_t *restrict data,
				isc_proxy2_handler_cb_t cb, void *cbarg) {
	*handler = (isc_proxy2_handler_t){ .result = ISC_R_UNSET,
					   .max_size = max_size };
	isc_proxy2_handler_setcb(handler, cb, cbarg);

	if (data == NULL) {
		isc_buffer_init(&handler->hdrbuf, handler->buf,
				sizeof(handler->buf));
	} else {
		isc_buffer_init(&handler->hdrbuf, data->base, data->length);
		isc_buffer_add(&handler->hdrbuf, data->length);
	}
}

void
isc_proxy2_handler_init(isc_proxy2_handler_t *restrict handler, isc_mem_t *mctx,
			const uint16_t max_size, isc_proxy2_handler_cb_t cb,
			void *cbarg) {
	REQUIRE(handler != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(max_size == 0 || max_size >= ISC_PROXY2_HEADER_SIZE);
	REQUIRE(cb != NULL);

	isc__proxy2_handler_init_direct(handler, max_size, NULL, cb, cbarg);

	isc_mem_attach(mctx, &handler->mctx);
	isc_buffer_setmctx(&handler->hdrbuf, handler->mctx);
}

void
isc_proxy2_handler_uninit(isc_proxy2_handler_t *restrict handler) {
	REQUIRE(handler != NULL);

	/*
	 * Uninitialising the object from withing the callback does not
	 * make any sense.
	 */
	INSIST(handler->calling_cb == false);
	if (handler->mctx != NULL) {
		isc_buffer_clearmctx(&handler->hdrbuf);
		isc_mem_detach(&handler->mctx);
	}
	isc_buffer_invalidate(&handler->hdrbuf);
}

void
isc_proxy2_handler_clear(isc_proxy2_handler_t *restrict handler) {
	REQUIRE(handler != NULL);

	*handler = (isc_proxy2_handler_t){ .result = ISC_R_UNSET,
					   .mctx = handler->mctx,
					   .cb = handler->cb,
					   .cbarg = handler->cbarg,
					   .hdrbuf = handler->hdrbuf,
					   .max_size = handler->max_size };

	isc_buffer_clear(&handler->hdrbuf);
	isc_buffer_trycompact(&handler->hdrbuf);
}

isc_proxy2_handler_t *
isc_proxy2_handler_new(isc_mem_t *mctx, const uint16_t max_size,
		       isc_proxy2_handler_cb_t cb, void *cbarg) {
	isc_proxy2_handler_t *newhandler;

	REQUIRE(mctx != NULL);
	REQUIRE(cb != NULL);

	newhandler = isc_mem_get(mctx, sizeof(*newhandler));
	isc_proxy2_handler_init(newhandler, mctx, max_size, cb, cbarg);

	return newhandler;
}

void
isc_proxy2_handler_free(isc_proxy2_handler_t **restrict phandler) {
	isc_proxy2_handler_t *restrict handler = NULL;
	isc_mem_t *mctx = NULL;
	REQUIRE(phandler != NULL && *phandler != NULL);

	handler = *phandler;

	isc_mem_attach(handler->mctx, &mctx);
	isc_proxy2_handler_uninit(handler);
	isc_mem_putanddetach(&mctx, handler, sizeof(*handler));

	*phandler = NULL;
}

void
isc_proxy2_handler_setcb(isc_proxy2_handler_t *restrict handler,
			 isc_proxy2_handler_cb_t cb, void *cbarg) {
	REQUIRE(handler != NULL);
	REQUIRE(cb != NULL);
	handler->cb = cb;
	handler->cbarg = cbarg;
}

static inline int
proxy2_socktype_to_socktype(const isc_proxy2_socktype_t proxy_socktype) {
	int socktype = 0;

	switch (proxy_socktype) {
	case ISC_PROXY2_SOCK_UNSPEC:
		socktype = 0;
		break;
	case ISC_PROXY2_SOCK_STREAM:
		socktype = SOCK_STREAM;
		break;
	case ISC_PROXY2_SOCK_DGRAM:
		socktype = SOCK_DGRAM;
		break;
	default:
		ISC_UNREACHABLE();
	};

	return socktype;
}

static inline void
isc__proxy2_handler_callcb(isc_proxy2_handler_t *restrict handler,
			   const isc_result_t result,
			   const isc_proxy2_command_t cmd,
			   const isc_proxy2_socktype_t proxy_socktype,
			   const isc_sockaddr_t *src_addr,
			   const isc_sockaddr_t *dst_addr,
			   const isc_region_t *restrict tlv_data,
			   const isc_region_t *restrict extra_data) {
	int socktype = 0;

	handler->result = result;
	handler->calling_cb = true;

	if (result != ISC_R_SUCCESS) {
		handler->cb(result, cmd, -1, NULL, NULL, NULL, NULL,
			    handler->cbarg);
	} else {
		socktype = proxy2_socktype_to_socktype(proxy_socktype);
		handler->cb(result, cmd, socktype,
			    proxy_socktype == ISC_PROXY2_SOCK_UNSPEC ? NULL
								     : src_addr,
			    proxy_socktype == ISC_PROXY2_SOCK_UNSPEC ? NULL
								     : dst_addr,
			    tlv_data->length == 0 ? NULL : tlv_data,
			    extra_data->length == 0 ? NULL : extra_data,
			    handler->cbarg);
	}

	handler->calling_cb = false;
}

static inline void
isc__proxy2_handler_error(isc_proxy2_handler_t *restrict handler,
			  const isc_result_t result) {
	INSIST(result != ISC_R_SUCCESS);
	isc__proxy2_handler_callcb(handler, result, ISC_PROXY2_CMD_ILLEGAL,
				   ISC_PROXY2_SOCK_ILLEGAL, NULL, NULL, NULL,
				   NULL);
	if (result != ISC_R_NOMORE) {
		handler->state = ISC_PROXY2_STATE_END;
	}
}

static inline bool
isc__proxy2_handler_handle_signature(isc_proxy2_handler_t *restrict handler) {
	isc_region_t remaining = { 0, 0 };
	size_t len;

	isc_buffer_remainingregion(&handler->hdrbuf, &remaining);
	len = ISC_MIN(remaining.length, ISC_PROXY2_HEADER_SIGNATURE_SIZE);

	if (memcmp(ISC_PROXY2_HEADER_SIGNATURE, remaining.base, len) != 0) {
		isc__proxy2_handler_error(handler, ISC_R_UNEXPECTED);
		return false;
	} else if (len == ISC_PROXY2_HEADER_SIGNATURE_SIZE) {
		isc_buffer_forward(&handler->hdrbuf,
				   ISC_PROXY2_HEADER_SIGNATURE_SIZE);
		handler->expect_data = ISC_PROXY2_HEADER_SIZE -
				       ISC_PROXY2_HEADER_SIGNATURE_SIZE;
		handler->state++;
	} else {
		INSIST(len < ISC_PROXY2_HEADER_SIGNATURE_SIZE);
		isc__proxy2_handler_error(handler, ISC_R_NOMORE);
		return false;
	}
	return true;
}

static inline bool
isc__proxy2_handler_handle_header(isc_proxy2_handler_t *restrict handler) {
	/*
	 * The PROXYv2 header can be described as (signature 'sig' has been
	 * processed and verified already as a separate step):
	 *
	 *  struct proxy_hdr_v2 {
	 *     uint8_t sig[12];  // hex 0D 0A 0D 0A 00 0D 0A 51 55 49 54 0A
	 *     uint8_t ver_cmd;  // protocol version and command
	 *     uint8_t fam;      // protocol family and address
	 *     uint16_t len;     // number of following bytes part of the header
	 *  };
	 */
	uint8_t ver_cmd = 0;
	uint8_t cmd = 0;
	uint8_t fam = 0;
	uint16_t len = 0;
	int addrfamily = 0;
	int socktype = 0;
	size_t min_addr_payload_size = 0;

	ver_cmd = isc_buffer_getuint8(&handler->hdrbuf);

	/* extract version and check it */
	if ((ver_cmd & 0xF0U) >> 4 != 2) {
		/* only support for version 2 is implemented */
		isc__proxy2_handler_error(handler, ISC_R_NOTIMPLEMENTED);
		return false;
	}

	/* extract command */
	cmd = ver_cmd & 0xFU;

	fam = isc_buffer_getuint8(&handler->hdrbuf);
	len = isc_buffer_getuint16(&handler->hdrbuf);

	if (handler->max_size > 0 &&
	    (len + ISC_PROXY2_HEADER_SIZE) > handler->max_size)
	{
		goto error_range;
	}

	handler->expect_data = len;

	/* extract address family and socket type */
	addrfamily = (fam & 0xF0U) >> 4;
	socktype = fam & 0xFU;

	/* dispatch on the command value */
	switch (cmd) {
	case ISC_PROXY2_CMD_LOCAL:
		/* LOCAL implies "unspec" mode */
		handler->cmd = ISC_PROXY2_CMD_LOCAL;
		if (addrfamily != ISC_PROXY2_AF_UNSPEC ||
		    socktype != ISC_PROXY2_SOCK_UNSPEC)
		{
			goto error_unexpected;
		}
		handler->proxy_addr_family = ISC_PROXY2_AF_UNSPEC;
		handler->proxy_socktype = ISC_PROXY2_SOCK_UNSPEC;
		break;
	case ISC_PROXY2_CMD_PROXY:
		handler->cmd = ISC_PROXY2_CMD_PROXY;
		switch (addrfamily) {
		case ISC_PROXY2_AF_UNSPEC:
			if (socktype != ISC_PROXY2_SOCK_UNSPEC) {
				goto error_unexpected;
			}
			handler->proxy_addr_family = ISC_PROXY2_AF_UNSPEC;
			handler->proxy_socktype = ISC_PROXY2_SOCK_UNSPEC;
			break;
		case ISC_PROXY2_AF_INET:
		case ISC_PROXY2_AF_INET6:
		case ISC_PROXY2_AF_UNIX:
			handler->proxy_addr_family =
				(isc_proxy2_addrfamily_t)addrfamily;
			switch (socktype) {
			case ISC_PROXY2_SOCK_DGRAM:
			case ISC_PROXY2_SOCK_STREAM:
				handler->proxy_socktype =
					(isc_proxy2_socktype_t)socktype;
				break;
			default:
				goto error_unexpected;
			}
			break;
		default:
			goto error_unexpected;
		}
		break;
	default:
		goto error_unexpected;
	};

	/* verify if enough data will be available in the payload */
	switch (handler->proxy_addr_family) {
	case ISC_PROXY2_AF_INET:
		min_addr_payload_size = ISC_PROXY2_MIN_AF_INET_SIZE -
					ISC_PROXY2_HEADER_SIZE;
		break;
	case ISC_PROXY2_AF_INET6:
		min_addr_payload_size = ISC_PROXY2_MIN_AF_INET6_SIZE -
					ISC_PROXY2_HEADER_SIZE;
		break;
	case ISC_PROXY2_AF_UNIX:
		min_addr_payload_size = ISC_PROXY2_MIN_AF_UNIX_SIZE -
					ISC_PROXY2_HEADER_SIZE;
		break;
	default:
		break;
	}

	if (min_addr_payload_size > 0) {
		if (len < min_addr_payload_size) {
			goto error_range;
		}
		handler->tlv_data_size = len - min_addr_payload_size;
	}

	if (handler->tlv_data_size > 0 &&
	    handler->tlv_data_size < ISC_PROXY2_TLV_HEADER_SIZE)
	{
		goto error_range;
	}

	handler->header_size = ISC_PROXY2_HEADER_SIZE + len;

	handler->state++;

	return true;

error_unexpected:
	isc__proxy2_handler_error(handler, ISC_R_UNEXPECTED);
	return false;
error_range:
	isc__proxy2_handler_error(handler, ISC_R_RANGE);
	return false;
}

static inline isc_result_t
isc__proxy2_handler_get_addresses(isc_proxy2_handler_t *restrict handler,
				  isc_buffer_t *restrict hdrbuf,
				  isc_sockaddr_t *restrict src_addr,
				  isc_sockaddr_t *restrict dst_addr) {
	size_t addr_size = 0;
	void *psrc_addr = NULL, *pdst_addr = NULL;
	uint16_t src_port = 0, dst_port = 0;

	switch (handler->proxy_addr_family) {
	case ISC_PROXY2_AF_UNSPEC:
		/* in this case we are instructed to skip over the data */
		INSIST(handler->tlv_data_size == 0);
		isc_buffer_forward(hdrbuf, handler->expect_data);
		break;
	case ISC_PROXY2_AF_INET:
		addr_size = sizeof(src_addr->type.sin.sin_addr.s_addr);
		/*
		 * IPv4 source and destination endpoint addresses can be
		 * described as follows:
		 *
		 * struct {        // for TCP/UDP over IPv4, len = 12
		 *   uint32_t src_addr;
		 *   uint32_t dst_addr;
		 *   uint16_t src_port;
		 *   uint16_t dst_port;
		 * } ipv4_addr;
		 */
		psrc_addr = isc_buffer_current(hdrbuf);
		isc_buffer_forward(hdrbuf, addr_size);

		pdst_addr = isc_buffer_current(hdrbuf);
		isc_buffer_forward(hdrbuf, addr_size);

		src_port = isc_buffer_getuint16(hdrbuf);
		dst_port = isc_buffer_getuint16(hdrbuf);

		if (src_addr != NULL) {
			isc_sockaddr_fromin(src_addr, psrc_addr, src_port);
		}
		if (dst_addr != NULL) {
			isc_sockaddr_fromin(dst_addr, pdst_addr, dst_port);
		}
		break;
	case ISC_PROXY2_AF_INET6:
		addr_size = sizeof(src_addr->type.sin6.sin6_addr);
		/*
		 * IPv4 source and destination endpoint addresses can be
		 * described as follows:
		 *
		 * struct {        // for TCP/UDP over IPv6, len = 36
		 *    uint8_t  src_addr[16];
		 *    uint8_t  dst_addr[16];
		 *    uint16_t src_port;
		 *    uint16_t dst_port;
		 * } ipv6_addr;
		 */
		psrc_addr = isc_buffer_current(hdrbuf);
		isc_buffer_forward(hdrbuf, addr_size);

		pdst_addr = isc_buffer_current(hdrbuf);
		isc_buffer_forward(hdrbuf, addr_size);

		src_port = isc_buffer_getuint16(hdrbuf);
		dst_port = isc_buffer_getuint16(hdrbuf);

		if (src_addr != NULL) {
			isc_sockaddr_fromin6(src_addr, psrc_addr, src_port);
		}

		if (dst_addr != NULL) {
			isc_sockaddr_fromin6(dst_addr, pdst_addr, dst_port);
		}
		break;
	case ISC_PROXY2_AF_UNIX: {
		/*
		 * UNIX domain sockets source and destination endpoint
		 * addresses can be described as follows:
		 *
		 * struct {        // for AF_UNIX sockets, len = 216
		 *    uint8_t src_addr[108];
		 *    uint8_t dst_addr[108];
		 * } unix_addr;
		 *
		 * We currently have no use for this address type, but we can
		 * validate the data.
		 */
		unsigned char *ret = NULL;

		addr_size = ISC_PROXY2_AF_UNIX_MAX_PATH_LEN;

		ret = memchr(isc_buffer_current(hdrbuf), '\0', addr_size);
		if (ret == NULL) {
			/*
			 * Someone has attempted to send us a path string
			 * without a terminating '\0' byte - not a friend
			 * knocking at the door.
			 */
			return ISC_R_RANGE;
		}
		isc_buffer_forward(hdrbuf, addr_size);

		ret = memchr(isc_buffer_current(hdrbuf), '\0', addr_size);
		if (ret == NULL) {
			return ISC_R_RANGE;
		}
		isc_buffer_forward(hdrbuf, addr_size);
	} break;
	default:
		UNREACHABLE();
	}

	return ISC_R_SUCCESS;
}

static inline void
isc__proxy2_handler_handle_payload(isc_proxy2_handler_t *restrict handler) {
	isc_result_t result;
	isc_sockaddr_t src_addr = { 0 }, dst_addr = { 0 };

	result = isc__proxy2_handler_get_addresses(handler, &handler->hdrbuf,
						   &src_addr, &dst_addr);

	if (result != ISC_R_SUCCESS) {
		isc__proxy2_handler_error(handler, result);
		return;
	}

	if (handler->tlv_data_size > 0) {
		isc_buffer_remainingregion(&handler->hdrbuf,
					   &handler->tlv_data);
		handler->tlv_data.length = handler->tlv_data_size;
		isc_buffer_forward(&handler->hdrbuf, handler->tlv_data_size);
		result = isc_proxy2_tlv_data_verify(&handler->tlv_data);
		if (result != ISC_R_SUCCESS) {
			isc__proxy2_handler_error(handler, result);
			return;
		}
	}

	isc_buffer_remainingregion(&handler->hdrbuf, &handler->extra_data);
	handler->expect_data = 0;

	handler->state++;

	/*
	 * Treat AF_UNIX as AF_UNSPEC as we have no use for it, although
	 * at this point we have fully verified the header.
	 */
	if (handler->proxy_addr_family == ISC_PROXY2_AF_UNIX) {
		handler->proxy_addr_family = ISC_PROXY2_AF_UNSPEC;
		handler->proxy_socktype = ISC_PROXY2_SOCK_UNSPEC;
		handler->tlv_data = (isc_region_t){ 0 };
	}

	isc__proxy2_handler_callcb(
		handler, ISC_R_SUCCESS, handler->cmd, handler->proxy_socktype,
		&src_addr, &dst_addr, &handler->tlv_data, &handler->extra_data);

	return;
}

static inline bool
isc__proxy2_handler_handle_data(isc_proxy2_handler_t *restrict handler) {
	if (isc_buffer_remaininglength(&handler->hdrbuf) < handler->expect_data)
	{
		isc__proxy2_handler_error(handler, ISC_R_NOMORE);
		return false;
	}

	switch (handler->state) {
	case ISC_PROXY2_STATE_WAITING_SIGNATURE:
		/*
		 * We check for signature no matter how many bytes of it we
		 * have received. The idea is to not wait for the whole
		 * signature to verify it at once, but to detect, e.g. port
		 * scanners as early as possible. Should we receive data byte
		 * by byte, we would detect the problem when processing the
		 * first unexpected byte.
		 */
		return isc__proxy2_handler_handle_signature(handler);
	case ISC_PROXY2_STATE_WAITING_HEADER:
		/*
		 * Handle the rest of the header (except signature which we
		 * heave verified by now).
		 */
		return isc__proxy2_handler_handle_header(handler);
	case ISC_PROXY2_STATE_WAITING_PAYLOAD:
		/*
		 * Handle the PROXYv2 header payload - addresses and TLVs.
		 */
		isc__proxy2_handler_handle_payload(handler);
		break;
	default:
		UNREACHABLE();
		break;
	};

	return false;
}

static inline isc_result_t
isc__proxy2_handler_process_data(isc_proxy2_handler_t *restrict handler) {
	while (isc__proxy2_handler_handle_data(handler)) {
		if (handler->state == ISC_PROXY2_STATE_END) {
			break;
		}
	}

	return handler->result;
}

isc_result_t
isc_proxy2_handler_push_data(isc_proxy2_handler_t *restrict handler,
			     const void *restrict buf,
			     const unsigned int buf_size) {
	isc_result_t result;

	REQUIRE(handler != NULL);
	REQUIRE(buf != NULL && buf_size != 0);

	INSIST(!handler->calling_cb);

	if (handler->state == ISC_PROXY2_STATE_END) {
		isc_proxy2_handler_clear(handler);
	}

	isc_buffer_putmem(&handler->hdrbuf, buf, buf_size);

	result = isc__proxy2_handler_process_data(handler);

	return result;
}

isc_result_t
isc_proxy2_handler_push(isc_proxy2_handler_t *restrict handler,
			const isc_region_t *restrict region) {
	isc_result_t result;

	REQUIRE(handler != NULL);
	REQUIRE(region != NULL);

	result = isc_proxy2_handler_push_data(handler, region->base,
					      region->length);

	return result;
}

static inline bool
proxy2_payload_is_processed(const isc_proxy2_handler_t *restrict handler) {
	if (handler->state < ISC_PROXY2_STATE_END ||
	    handler->result != ISC_R_SUCCESS)
	{
		return false;
	}

	return true;
}

size_t
isc_proxy2_handler_header(const isc_proxy2_handler_t *restrict handler,
			  isc_region_t *restrict region) {
	REQUIRE(handler != NULL);
	REQUIRE(region == NULL ||
		(region->base == NULL && region->length == 0));

	if (!proxy2_payload_is_processed(handler)) {
		return 0;
	}

	if (region != NULL) {
		region->base = isc_buffer_base(&handler->hdrbuf);
		region->length = handler->header_size;
	}

	return handler->header_size;
}

size_t
isc_proxy2_handler_tlvs(const isc_proxy2_handler_t *restrict handler,
			isc_region_t *restrict region) {
	REQUIRE(handler != NULL);
	REQUIRE(region == NULL ||
		(region->base == NULL && region->length == 0));

	if (!proxy2_payload_is_processed(handler)) {
		return 0;
	}

	SET_IF_NOT_NULL(region, handler->tlv_data);

	return handler->tlv_data.length;
}

size_t
isc_proxy2_handler_extra(const isc_proxy2_handler_t *restrict handler,
			 isc_region_t *restrict region) {
	REQUIRE(handler != NULL);
	REQUIRE(region == NULL ||
		(region->base == NULL && region->length == 0));

	if (!proxy2_payload_is_processed(handler)) {
		return 0;
	}

	SET_IF_NOT_NULL(region, handler->extra_data);

	return handler->extra_data.length;
}

isc_result_t
isc_proxy2_handler_result(const isc_proxy2_handler_t *restrict handler) {
	REQUIRE(handler != NULL);

	return handler->result;
}

isc_result_t
isc_proxy2_handler_addresses(const isc_proxy2_handler_t *restrict handler,
			     int *restrict psocktype,
			     isc_sockaddr_t *restrict psrc_addr,
			     isc_sockaddr_t *restrict pdst_addr) {
	isc_result_t result;
	size_t ret;
	isc_region_t header_region = { 0 };
	isc_buffer_t buf = { 0 };

	REQUIRE(handler != NULL);

	if (!proxy2_payload_is_processed(handler)) {
		return ISC_R_UNEXPECTED;
	}

	ret = isc_proxy2_handler_header(handler, &header_region);
	RUNTIME_CHECK(ret > 0);

	isc_buffer_init(&buf, header_region.base, header_region.length);
	isc_buffer_add(&buf, header_region.length);
	isc_buffer_forward(&buf, ISC_PROXY2_HEADER_SIZE);

	INSIST(handler->expect_data == 0);

	result = isc__proxy2_handler_get_addresses(
		(isc_proxy2_handler_t *)handler, &buf, psrc_addr, pdst_addr);

	if (result != ISC_R_SUCCESS) {
		return result;
	}

	SET_IF_NOT_NULL(psocktype,
			proxy2_socktype_to_socktype(handler->proxy_socktype));

	return ISC_R_SUCCESS;
}

isc_result_t
isc_proxy2_tlv_iterate(const isc_region_t *restrict tlv_data,
		       const isc_proxy2_tlv_cb_t cb, void *cbarg) {
	isc_result_t result = ISC_R_SUCCESS;
	isc_buffer_t tlvs = { 0 };
	size_t remaining;

	/*
	 * TLV header can be described as follows:
	 *
	 *   struct {
	 *       uint8_t type;
	 *       uint8_t length_hi;
	 *       uint8_t length_lo;
	 *   };
	 *
	 */

	REQUIRE(tlv_data != NULL);
	REQUIRE(cb != NULL);

	isc_buffer_init(&tlvs, tlv_data->base, tlv_data->length);
	isc_buffer_add(&tlvs, tlv_data->length);

	while ((remaining = isc_buffer_remaininglength(&tlvs)) > 0) {
		uint8_t type = 0;
		uint16_t len = 0;
		isc_region_t current_tlv_data = { 0 };
		bool ret = false;

		/* not enough data for a TLV header */
		if (remaining < ISC_PROXY2_TLV_HEADER_SIZE) {
			result = ISC_R_RANGE;
			break;
		}

		type = isc_buffer_getuint8(&tlvs);
		len = isc_buffer_getuint16(&tlvs);

		if ((remaining - ISC_PROXY2_TLV_HEADER_SIZE) < len) {
			result = ISC_R_RANGE;
			break;
		}

		current_tlv_data.base = isc_buffer_current(&tlvs);
		current_tlv_data.length = len;
		isc_buffer_forward(&tlvs, len);

		ret = cb((isc_proxy2_tlv_type_t)type, &current_tlv_data, cbarg);
		if (!ret) {
			break;
		}
	}

	return result;
}

typedef struct proxy2_tls_cbarg {
	uint8_t client;
	bool client_cert_verified;
	isc_proxy2_tls_subtlv_cb_t cb;
	void *cbarg;
} tls_cbarg_t;

static bool
proxy2_tls_iter_cb(const isc_proxy2_tlv_type_t tlv_type,
		   const isc_region_t *restrict data, void *cbarg) {
	bool ret = false;
	tls_cbarg_t *tls_cbarg = (tls_cbarg_t *)cbarg;

	ret = tls_cbarg->cb(tls_cbarg->client, tls_cbarg->client_cert_verified,
			    (isc_proxy2_tlv_subtype_tls_t)tlv_type, data,
			    tls_cbarg->cbarg);

	return ret;
}

isc_result_t
isc_proxy2_subtlv_tls_header_data(const isc_region_t *restrict tls_tlv_data,
				  uint8_t *restrict pclient_flags,
				  bool *restrict pclient_cert_verified) {
	/*
	 * SSL/TLS TLV header can be described as follows:
	 *
	 *   struct {
	 *       uint8_t  client_flags;
	 *       uint32_t client_cert_not_verified;
	 *   }
	 */
	uint8_t *p = NULL;
	uint8_t client_flags = 0;
	bool client_cert_verified = false;
	uint32_t client_cert_verified_data = 0;

	REQUIRE(tls_tlv_data != NULL);
	REQUIRE(pclient_flags == NULL || *pclient_flags == 0);
	REQUIRE(pclient_cert_verified == NULL ||
		*pclient_cert_verified == false);

	if (tls_tlv_data->length < ISC_PROXY2_TLS_SUBHEADER_MIN_SIZE) {
		return ISC_R_RANGE;
	}

	p = tls_tlv_data->base;

	client_flags = *p;
	p++;
	/* We need this to avoid ASAN complain about unaligned access */
	memmove(&client_cert_verified_data, p, sizeof(uint32_t));
	client_cert_verified = ntohl(client_cert_verified_data) == 0;

	SET_IF_NOT_NULL(pclient_flags, client_flags);
	SET_IF_NOT_NULL(pclient_cert_verified, client_cert_verified);

	return ISC_R_SUCCESS;
}

isc_result_t
isc_proxy2_subtlv_tls_iterate(const isc_region_t *restrict tls_tlv_data,
			      const isc_proxy2_tls_subtlv_cb_t cb,
			      void *cbarg) {
	tls_cbarg_t tls_cbarg;
	isc_result_t result = ISC_R_SUCCESS;
	uint8_t *p = NULL;
	uint8_t client_flags = 0;
	bool client_cert_verified = false;

	REQUIRE(tls_tlv_data != NULL);
	REQUIRE(cb != NULL);

	if (tls_tlv_data->length < ISC_PROXY2_TLS_SUBHEADER_MIN_SIZE) {
		return ISC_R_RANGE;
	}

	result = isc_proxy2_subtlv_tls_header_data(tls_tlv_data, &client_flags,
						   &client_cert_verified);

	if (result != ISC_R_SUCCESS) {
		return result;
	}

	p = tls_tlv_data->base;
	p += ISC_PROXY2_TLS_SUBHEADER_MIN_SIZE;

	if (cb != NULL) {
		isc_region_t data = {
			.base = p,
			.length = tls_tlv_data->length -
				  ISC_PROXY2_TLS_SUBHEADER_MIN_SIZE
		};
		tls_cbarg = (tls_cbarg_t){ .client = client_flags,
					   .client_cert_verified =
						   client_cert_verified,
					   .cb = cb,
					   .cbarg = cbarg };
		result = isc_proxy2_tlv_iterate(&data, proxy2_tls_iter_cb,
						&tls_cbarg);
	}

	return result;
}

typedef struct tls_subtlv_verify_cbarg {
	uint16_t *count;
	isc_result_t verif_result;
} tls_subtlv_verify_cbarg_t;

static bool
proxy2_subtlv_verify_iter_cb(const uint8_t client,
			     const bool client_cert_verified,
			     const isc_proxy2_tlv_subtype_tls_t tls_subtlv_type,
			     const isc_region_t *restrict data, void *cbarg) {
	bool verify_count = false;
	tls_subtlv_verify_cbarg_t *restrict arg =
		(tls_subtlv_verify_cbarg_t *)cbarg;
	uint8_t type = tls_subtlv_type;

	UNUSED(client);
	UNUSED(client_cert_verified);

	if (type <= ISC_PROXY2_TLV_TYPE_TLS ||
	    type == ISC_PROXY2_TLV_TYPE_NETNS)
	{
		arg->verif_result = ISC_R_UNEXPECTED;
		return false;
	}

	switch (tls_subtlv_type) {
	case ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION:
	case ISC_PROXY2_TLV_SUBTYPE_TLS_CN:
	case ISC_PROXY2_TLV_SUBTYPE_TLS_SIG_ALG:
	case ISC_PROXY2_TLV_SUBTYPE_TLS_KEY_ALG:
		if (data->length == 0) {
			arg->verif_result = ISC_R_RANGE;
			return false;
		}
		arg->count[tls_subtlv_type]++;
		verify_count = true;
		break;
	default:
		break;
	};

	if (verify_count && arg->count[tls_subtlv_type] > 1) {
		arg->verif_result = ISC_R_UNEXPECTED;
		return false;
	}

	return true;
}

typedef struct tlv_verify_cbarg {
	uint16_t count[256];
	isc_result_t verify_result;
} tlv_verify_cbarg_t;

static bool
isc_proxy2_tlv_verify_cb(const isc_proxy2_tlv_type_t tlv_type,
			 const isc_region_t *restrict data, void *cbarg) {
	bool verify_count = false;
	uint8_t client = 0;
	tlv_verify_cbarg_t *arg = (tlv_verify_cbarg_t *)cbarg;

	if (tlv_type == 0) {
		/* the TLV values start from 1 */
		goto error_unexpected;
	}

	switch (tlv_type) {
	case ISC_PROXY2_TLV_TYPE_ALPN:
	case ISC_PROXY2_TLV_TYPE_AUTHORITY:
	case ISC_PROXY2_TLV_TYPE_NETNS:
		/* these values need to be more than 0 bytes long */
		if (data->length == 0) {
			goto error_range;
		}
		arg->count[tlv_type]++;
		verify_count = true;
		break;
	case ISC_PROXY2_TLV_TYPE_CRC32C:
		if (data->length != sizeof(uint32_t)) {
			goto error_range;
		}
		arg->count[tlv_type]++;
		verify_count = true;
		break;
	case ISC_PROXY2_TLV_TYPE_UNIQUE_ID:
		if (data->length > 128) {
			goto error_range;
		}
		arg->count[tlv_type]++;
		verify_count = true;
		break;
	case ISC_PROXY2_TLV_TYPE_TLS: {
		tls_subtlv_verify_cbarg_t tls_cbarg = {
			.verif_result = ISC_R_SUCCESS, .count = arg->count
		};
		size_t tls_version_count, tls_cn_count;

		arg->verify_result =
			isc_proxy2_subtlv_tls_header_data(data, &client, NULL);

		if (arg->verify_result != ISC_R_SUCCESS) {
			return false;
		}

		arg->verify_result = isc_proxy2_subtlv_tls_iterate(
			data, proxy2_subtlv_verify_iter_cb, &tls_cbarg);

		if (arg->verify_result != ISC_R_SUCCESS) {
			return false;
		} else if (tls_cbarg.verif_result != ISC_R_SUCCESS) {
			arg->verify_result = tls_cbarg.verif_result;
			return false;
		}

		/*
		 * if CLIENT_TLS flag is set - TLS version TLV must be present
		 */
		tls_version_count =
			arg->count[ISC_PROXY2_TLV_SUBTYPE_TLS_VERSION];

		if ((client & ISC_PROXY2_CLIENT_TLS) != 0) {
			if (tls_version_count != 1) {
				goto error_unexpected;
			}
		} else if (tls_version_count > 0) {
			/* unexpected TLS version TLV */
			goto error_unexpected;
		}

		/*
		 * If client cert was submitted, CLIENT_CERT_CONN or
		 * CLIENT_CERT_SESS flags must be present alongside the
		 * CLIENT_TLS flag.
		 */
		tls_cn_count = arg->count[ISC_PROXY2_TLV_SUBTYPE_TLS_CN];

		if ((client & (ISC_PROXY2_CLIENT_CERT_CONN |
			       ISC_PROXY2_CLIENT_CERT_SESS)) != 0)
		{
			if (tls_cn_count != 1 ||
			    (client & ISC_PROXY2_CLIENT_TLS) == 0)
			{
				goto error_unexpected;
			}
		} else if (tls_cn_count > 0) {
			/* unexpected Common Name TLV */
			goto error_unexpected;
		}

		arg->count[tlv_type]++;
		verify_count = true;
	} break;
	default:
		break;
	};

	if (verify_count && arg->count[tlv_type] > 1) {
		goto error_unexpected;
	}

	return true;

error_unexpected:
	arg->verify_result = ISC_R_UNEXPECTED;
	return false;

error_range:
	arg->verify_result = ISC_R_RANGE;
	return false;
}

isc_result_t
isc_proxy2_tlv_data_verify(const isc_region_t *restrict tlv_data) {
	isc_result_t result;
	tlv_verify_cbarg_t cbarg = { .verify_result = ISC_R_SUCCESS };

	result = isc_proxy2_tlv_iterate(tlv_data, isc_proxy2_tlv_verify_cb,
					&cbarg);
	if (result != ISC_R_SUCCESS) {
		return result;
	}

	return cbarg.verify_result;
}

isc_result_t
isc_proxy2_header_handle_directly(const isc_region_t *restrict header_data,
				  const isc_proxy2_handler_cb_t cb,
				  void *cbarg) {
	isc_result_t result;
	isc_proxy2_handler_t handler = { 0 };

	REQUIRE(header_data != NULL);
	REQUIRE(cb != NULL);

	isc__proxy2_handler_init_direct(&handler, 0, header_data, cb, cbarg);

	result = isc__proxy2_handler_process_data(&handler);

	return result;
}

isc_result_t
isc_proxy2_make_header(isc_buffer_t *restrict outbuf,
		       const isc_proxy2_command_t cmd, const int socktype,
		       const isc_sockaddr_t *restrict src_addr,
		       const isc_sockaddr_t *restrict dst_addr,
		       const isc_region_t *restrict tlv_data) {
	size_t total_size = ISC_PROXY2_HEADER_SIZE;
	uint8_t family = ISC_PROXY2_AF_UNSPEC;
	isc_proxy2_socktype_t proxy_socktype = ISC_PROXY2_SOCK_UNSPEC;

	uint8_t ver_cmd = 0;
	uint8_t fam_socktype = 0;
	uint16_t len = 0;

	size_t addr_size = 0;
	void *psrc_addr = NULL, *pdst_addr = NULL;
	/*
	 * The complete PROXYv2 header can be described as follows:
	 *
	 * 1. Header:
	 *
	 * struct proxy_hdr_v2 {
	 *   uint8_t sig[12];      // hex 0D 0A 0D 0A 00 0D 0A 51 55 49 54 0A
	 *   uint8_t ver_cmd;      // protocol version and command
	 *   uint8_t fam_socktype; // protocol family and socket type
	 *   uint16_t len;         // number of following bytes
	 * };
	 *
	 * 2. Addresses:
	 *
	 * union proxy_addr {
	 *   struct {        // for TCP/UDP over IPv4, len = 12
	 *       uint32_t src_addr;
	 *       uint32_t dst_addr;
	 *       uint16_t src_port;
	 *       uint16_t dst_port;
	 *   } ipv4_addr;
	 *   struct {        // for TCP/UDP over IPv6, len = 36
	 *        uint8_t  src_addr[16];
	 *        uint8_t  dst_addr[16];
	 *        uint16_t src_port;
	 *        uint16_t dst_port;
	 *   } ipv6_addr;
	 *   struct {        // for AF_UNIX sockets, len = 216
	 *        uint8_t src_addr[108];
	 *        uint8_t dst_addr[108];
	 *   } unix_addr;
	 * };
	 *
	 * 3. TLVs (optional)
	 */

	REQUIRE(outbuf != NULL);
	REQUIRE(cmd == ISC_PROXY2_CMD_PROXY || socktype == 0);
	REQUIRE((src_addr == NULL && dst_addr == NULL) ||
		(src_addr != NULL && dst_addr != NULL));
	REQUIRE(src_addr == NULL ||
		(isc_sockaddr_pf(src_addr) == isc_sockaddr_pf(dst_addr)));

	switch (cmd) {
	case ISC_PROXY2_CMD_LOCAL:
		family = ISC_PROXY2_AF_UNSPEC;
		break;
	case ISC_PROXY2_CMD_PROXY:
		if (socktype == 0) {
			family = ISC_PROXY2_AF_UNSPEC;
		} else {
			switch (isc_sockaddr_pf(src_addr)) {
			case AF_INET:
				family = ISC_PROXY2_AF_INET;
				addr_size = sizeof(src_addr->type.sin.sin_addr);
				total_size += addr_size * 2 +
					      sizeof(uint16_t) * 2;
				psrc_addr = (void *)&src_addr->type.sin.sin_addr
						    .s_addr;
				pdst_addr = (void *)&dst_addr->type.sin.sin_addr
						    .s_addr;
				break;
			case AF_INET6:
				family = ISC_PROXY2_AF_INET6;
				addr_size =
					sizeof(src_addr->type.sin6.sin6_addr);
				total_size += addr_size * 2 +
					      sizeof(uint16_t) * 2;
				psrc_addr =
					(void *)&src_addr->type.sin6.sin6_addr;
				pdst_addr =
					(void *)&dst_addr->type.sin6.sin6_addr;
				break;
			default:
				return ISC_R_UNEXPECTED;
			}
		}
		break;
	default:
		return ISC_R_UNEXPECTED;
	}

	switch (socktype) {
	case 0:
		proxy_socktype = ISC_PROXY2_SOCK_UNSPEC;
		break;
	case SOCK_STREAM:
		proxy_socktype = ISC_PROXY2_SOCK_STREAM;
		break;
	case SOCK_DGRAM:
		proxy_socktype = ISC_PROXY2_SOCK_DGRAM;
		break;
	default:
		return ISC_R_UNEXPECTED;
	}

	if (tlv_data != NULL) {
		if (tlv_data->length > UINT16_MAX) {
			return ISC_R_RANGE;
		}
		total_size += tlv_data->length;
	}

	if (isc_buffer_availablelength(outbuf) < total_size) {
		return ISC_R_NOSPACE;
	} else if (total_size > UINT16_MAX) {
		return ISC_R_RANGE;
	}

	/*
	 * Combine version 2 (highest four bits) and command (lowest four
	 * bits).
	 */
	ver_cmd = (((2 << 4) & 0xF0U) | cmd);

	/*
	 * Combine address family (highest four bits) and socket type
	 * (lowest four bits).
	 */
	fam_socktype = (((family << 4) & 0xF0U) | proxy_socktype);

	len = (uint16_t)(total_size - ISC_PROXY2_HEADER_SIZE);

	/* Write signature */
	isc_buffer_putmem(outbuf, (uint8_t *)ISC_PROXY2_HEADER_SIGNATURE,
			  ISC_PROXY2_HEADER_SIGNATURE_SIZE);
	/* Write version and command */
	isc_buffer_putuint8(outbuf, ver_cmd);
	/* Write address family and socket type */
	isc_buffer_putuint8(outbuf, fam_socktype);
	/* Write header payload size (addresses + TLVs) */
	isc_buffer_putuint16(outbuf, len);

	/* Write source and destination addresses (if we should) */
	if (psrc_addr != NULL) {
		isc_buffer_putmem(outbuf, psrc_addr, addr_size);
	}

	if (pdst_addr != NULL) {
		isc_buffer_putmem(outbuf, pdst_addr, addr_size);
	}

	/* Write source and destination ports (if we should) */
	if (family == ISC_PROXY2_AF_INET || family == ISC_PROXY2_AF_INET6) {
		isc_buffer_putuint16(outbuf, isc_sockaddr_getport(src_addr));
		isc_buffer_putuint16(outbuf, isc_sockaddr_getport(dst_addr));
	}

	if (tlv_data != NULL) {
		isc_buffer_putmem(outbuf, tlv_data->base, tlv_data->length);
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_proxy2_header_append(isc_buffer_t *restrict outbuf,
			 const isc_region_t *restrict data) {
	const size_t len_offset = ISC_PROXY2_HEADER_SIZE - sizeof(uint16_t);
	isc_region_t header_data = { 0 };
	uint16_t new_len = 0;

	REQUIRE(outbuf != NULL);

	isc_buffer_usedregion(outbuf, &header_data);

	REQUIRE(header_data.length >= ISC_PROXY2_HEADER_SIZE);
	REQUIRE(data != NULL);

	if (isc_buffer_availablelength(outbuf) < data->length) {
		return ISC_R_NOSPACE;
	} else if ((data->length + header_data.length) > UINT16_MAX) {
		return ISC_R_RANGE;
	}

	INSIST(memcmp(header_data.base, ISC_PROXY2_HEADER_SIGNATURE,
		      ISC_PROXY2_HEADER_SIGNATURE_SIZE) == 0);

	/* fixup length of the header payload */
	/* load */
	memmove(&new_len, &header_data.base[len_offset], sizeof(new_len));
	new_len = ntohs(new_len);
	/* check */
	if ((data->length + new_len) > UINT16_MAX) {
		return ISC_R_RANGE;
	}
	/* update */
	new_len += (uint16_t)data->length;
	/* store */
	new_len = htons(new_len);
	memmove(&header_data.base[len_offset], &new_len, sizeof(new_len));

	isc_buffer_putmem(outbuf, data->base, data->length);

	return ISC_R_SUCCESS;
}

static inline void
append_type_and_length(isc_buffer_t *restrict outbuf, const uint8_t type,
		       const uint16_t tlv_length, const bool update_header) {
	uint16_t length;
	isc_region_t type_region = { 0 }, length_region = { 0 };

	type_region = (isc_region_t){ .base = (uint8_t *)&type,
				      .length = sizeof(type) };
	length = htons(tlv_length);
	length_region = (isc_region_t){ .base = (uint8_t *)&length,
					.length = sizeof(length) };

	if (update_header) {
		isc_result_t result = isc_proxy2_header_append(outbuf,
							       &type_region);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		result = isc_proxy2_header_append(outbuf, &length_region);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	} else {
		isc_buffer_putmem(outbuf, type_region.base, type_region.length);
		isc_buffer_putmem(outbuf, length_region.base,
				  length_region.length);
	}
}

isc_result_t
isc_proxy2_header_append_tlv(isc_buffer_t *restrict outbuf,
			     const isc_proxy2_tlv_type_t tlv_type,
			     const isc_region_t *restrict tlv_data) {
	size_t new_data_len = 0;
	REQUIRE(outbuf != NULL);
	REQUIRE(tlv_data != NULL);

	/*
	 * TLV header can be described as follows:
	 *
	 *   struct {
	 *       uint8_t type;
	 *       uint8_t length_hi;
	 *       uint8_t length_lo;
	 *   };
	 *
	 */
	new_data_len = tlv_data->length + 3;

	if (isc_buffer_availablelength(outbuf) < (new_data_len)) {
		return ISC_R_NOSPACE;
	} else if ((isc_buffer_usedlength(outbuf) + new_data_len) > UINT16_MAX)
	{
		return ISC_R_RANGE;
	}

	append_type_and_length(outbuf, (uint8_t)tlv_type,
			       ((uint16_t)tlv_data->length), true);

	if (tlv_data->length > 0) {
		isc_result_t result = isc_proxy2_header_append(outbuf,
							       tlv_data);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_proxy2_header_append_tlv_string(isc_buffer_t *restrict outbuf,
				    const isc_proxy2_tlv_type_t tlv_type,
				    const char *restrict str) {
	isc_result_t result;
	isc_region_t region = { 0 };

	REQUIRE(str != NULL && *str != '\0');

	region.base = (uint8_t *)str;
	region.length = strlen(str);

	result = isc_proxy2_header_append_tlv(outbuf, tlv_type, &region);

	return result;
}

isc_result_t
isc_proxy2_make_tls_subheader(isc_buffer_t *restrict outbuf,
			      const uint8_t client_flags,
			      const bool client_cert_verified,
			      const isc_region_t *restrict tls_subtlvs_data) {
	size_t total_size = ISC_PROXY2_TLS_SUBHEADER_MIN_SIZE;
	uint32_t client_cert_not_verified = 1;
	REQUIRE(outbuf != NULL);

	if (tls_subtlvs_data != NULL) {
		total_size += tls_subtlvs_data->length;
	}

	if (isc_buffer_availablelength(outbuf) < total_size) {
		return ISC_R_NOSPACE;
	} else if (total_size > UINT16_MAX) {
		return ISC_R_RANGE;
	}

	isc_buffer_putuint8(outbuf, client_flags);
	client_cert_not_verified = htonl(!client_cert_verified);
	isc_buffer_putmem(outbuf, (uint8_t *)&client_cert_not_verified,
			  sizeof(client_cert_not_verified));

	if (tls_subtlvs_data != NULL) {
		isc_buffer_putmem(outbuf, tls_subtlvs_data->base,
				  tls_subtlvs_data->length);
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_proxy2_append_tlv(isc_buffer_t *restrict outbuf, const uint8_t type,
		      const isc_region_t *restrict data) {
	size_t new_data_len = 0;
	REQUIRE(outbuf != NULL);
	REQUIRE(data != NULL);

	new_data_len = (data->length + 3);

	if (isc_buffer_availablelength(outbuf) < new_data_len) {
		return ISC_R_NOSPACE;
	} else if ((isc_buffer_usedlength(outbuf) + (data->length + 3)) >
		   UINT16_MAX)
	{
		return ISC_R_RANGE;
	}

	append_type_and_length(outbuf, (uint8_t)type, ((uint16_t)data->length),
			       false);

	if (data->length > 0) {
		isc_buffer_putmem(outbuf, data->base, data->length);
	}

	return ISC_R_SUCCESS;
}

isc_result_t
isc_proxy2_append_tlv_string(isc_buffer_t *restrict outbuf, const uint8_t type,
			     const char *restrict str) {
	isc_result_t result;
	isc_region_t region = { 0 };

	REQUIRE(str != NULL && *str != '\0');

	region.base = (uint8_t *)str;
	region.length = strlen(str);

	result = isc_proxy2_append_tlv(outbuf, type, &region);

	return result;
}
