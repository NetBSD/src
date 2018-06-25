/*
 * This file generated automatically from ge.xml by c_client.py.
 * Edit at your peril.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>  /* for offsetof() */
#include "xcbext.h"
#include "ge.h"

#define ALIGNOF(type) offsetof(struct { char dummy; type member; }, member)

xcb_extension_t xcb_genericevent_id = { "Generic Event Extension", 0 };

xcb_genericevent_query_version_cookie_t
xcb_genericevent_query_version (xcb_connection_t *c,
                                uint16_t          client_major_version,
                                uint16_t          client_minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_genericevent_id,
        .opcode = XCB_GENERICEVENT_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_genericevent_query_version_cookie_t xcb_ret;
    xcb_genericevent_query_version_request_t xcb_out;

    xcb_out.client_major_version = client_major_version;
    xcb_out.client_minor_version = client_minor_version;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_genericevent_query_version_cookie_t
xcb_genericevent_query_version_unchecked (xcb_connection_t *c,
                                          uint16_t          client_major_version,
                                          uint16_t          client_minor_version)
{
    static const xcb_protocol_request_t xcb_req = {
        .count = 2,
        .ext = &xcb_genericevent_id,
        .opcode = XCB_GENERICEVENT_QUERY_VERSION,
        .isvoid = 0
    };

    struct iovec xcb_parts[4];
    xcb_genericevent_query_version_cookie_t xcb_ret;
    xcb_genericevent_query_version_request_t xcb_out;

    xcb_out.client_major_version = client_major_version;
    xcb_out.client_minor_version = client_minor_version;

    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;

    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

xcb_genericevent_query_version_reply_t *
xcb_genericevent_query_version_reply (xcb_connection_t                         *c,
                                      xcb_genericevent_query_version_cookie_t   cookie  /**< */,
                                      xcb_generic_error_t                     **e)
{
    return (xcb_genericevent_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

