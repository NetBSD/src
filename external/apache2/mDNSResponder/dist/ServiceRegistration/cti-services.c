/* cti-services.c
 *
 * Copyright (c) 2020-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * Concise Thread Interface for Thread Border router control.
 */


#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#include <Block.h>
#include <os/log.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet6/in6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include "xpc_clients.h"
#include "cti-services.h"
typedef xpc_object_t object_t;
typedef void (*cti_internal_callback_t)(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status);


//*************************************************************************************************************
// Globals

#include "cti-common.h"

static int client_serial_number;

struct _cti_connection_t
{
    int ref_count;

    // Callback function ptr for Client
    cti_callback_t callback;

    // xpc_connection between client and daemon
    xpc_connection_t NULLABLE connection;

    // Before we can send commands, we have to check in, so when starting up, we stash the initial command
    // here until we get an acknowledgment for the checkin.
    object_t *first_command;

    // Queue specified by client for scheduling its Callback
    dispatch_queue_t NULLABLE client_queue;

    // For commands that fetch properties and also track properties, this will contain the name of the property
    // for which events are requested.
    const char *property_name;

    // For commands that fetch string properties, this will contain the name of the string property to look for
    // in the answer
    const char *return_property_name;

    cti_internal_callback_t NONNULL internal_callback;

    // Client context
    void *NULLABLE context;

    // Printed when debugging the event handler
    const char *NONNULL command_name;

    // This connection's serial number, based on the global client_serial_number above.
    int serial;

    // True if we've gotten a response to the check-in message.
    bool checked_in;
};

//*************************************************************************************************************
// Utility Functions

static void
cti_connection_finalize(cti_connection_t ref)
{
    if (ref->first_command != NULL) {
        xpc_release(ref->first_command);
        ref->first_command = NULL;
    }
    free(ref);
}

#define cti_connection_release(ref) cti_connection_release_(ref, __FILE__, __LINE__)
static void
cti_connection_release_(cti_connection_t ref, const char *file, int line)
{
    ref->callback.reply = NULL;
    RELEASE(ref, cti_connection);
}

static void
cti_xpc_connection_finalize(void *context)
{
    cti_connection_t ref = context;
    INFO("[CX%d] " PUB_S_SRP, ref->serial, ref->command_name);
    cti_connection_release(context);
}

static char *
cti_xpc_copy_description(object_t object)
{
    xpc_type_t type = xpc_get_type(object);
    if (type == XPC_TYPE_UINT64) {
        uint64_t num = xpc_uint64_get_value(object);
        char buf[23];
        snprintf(buf, sizeof buf, "%llu", num);
        return strdup(buf);
    } else if (type == XPC_TYPE_INT64) {
        int64_t num = xpc_int64_get_value(object);
        char buf[23];
        snprintf(buf, sizeof buf, "%lld", num);
        return strdup(buf);
    } else if (type == XPC_TYPE_STRING) {
        const char *str = xpc_string_get_string_ptr(object);
        size_t len = xpc_string_get_length(object);
        char *ret = malloc(len + 3);
        if (ret != NULL) {
            *ret = '"';
            strlcpy(ret + 1, str, len + 1);
            ret[len + 1] = '"';
            ret[len + 2] = 0;
            return ret;
        }
    } else if (type == XPC_TYPE_DATA) {
        const uint8_t *data = xpc_data_get_bytes_ptr(object);
        size_t i, len = xpc_data_get_length(object);
        char *ret = malloc(len * 2 + 3);
        if (ret != NULL) {
            ret[0] = '0';
            ret[1] = 'x';
            for (i = 0; i < len; i++) {
                snprintf(ret + i * 2, 3, "%02x", data[i]);
            }
            return ret;
        }
    } else if (type == XPC_TYPE_BOOL) {
        bool flag = xpc_bool_get_value(object);
        if (flag) {
            return strdup("true");
        } else {
            return strdup("false");
        }
    } else if (type == XPC_TYPE_ARRAY) {
        size_t avail, vlen, len = 0, i, count = xpc_array_get_count(object);
        char **values = malloc(count * sizeof(*values));
        char *ret, *p_ret;
        if (values == NULL) {
            return NULL;
        }
        xpc_array_apply(object, ^bool (size_t index, object_t value) {
                values[index] = cti_xpc_copy_description(value);
                return true;
            });
        for (i = 0; i < count; i++) {
            if (values[i] == NULL) {
                len += 6;
            } else {
                len += strlen(values[i]) + 2;
            }
        }
        ret = malloc(len + 3);
        p_ret = ret;
        avail = len + 1;
        *p_ret++ = '[';
        --avail;
        for (i = 0; i < count; i++) {
            if (p_ret != NULL) {
                snprintf(p_ret, avail, "%s%s%s", i == 0 ? "" : " ", values[i] != NULL ? values[i] : "NULL", (i + 1 == count) ? "" : ",");
                vlen = strlen(p_ret);
                p_ret += vlen;
                avail -= vlen;
            }
            if (values[i] != NULL) {
                free(values[i]);
            }
        }
        *p_ret++ = ']';
        *p_ret++ = 0;
        free(values);
        return ret;
    }
    return xpc_copy_description(object);
}

static void
cti_log_object(const char *context, int serial, const char *command, const char *preamble, const char *divide, object_t *object, char *indent)
{
    xpc_type_t type = xpc_get_type(object);
    static char no_indent[] = "";
    if (indent == NULL) {
        indent = no_indent;
    }
    char *new_indent;
    size_t depth;
    char *desc;
    char *compound_begin;
    char *compound_end;

    if (type == XPC_TYPE_DICTIONARY || type == XPC_TYPE_ARRAY) {
        bool compact = true;
        bool *p_compact = &compact;
        if (type == XPC_TYPE_DICTIONARY) {
            compound_begin = "{";
            compound_end = "}";
            xpc_dictionary_apply(object, ^bool (const char *__unused key, object_t value) {
                    xpc_type_t sub_type = xpc_get_type(value);
                    if (sub_type == XPC_TYPE_DICTIONARY) {
                        *p_compact = false;
                    } else if (sub_type == XPC_TYPE_ARRAY) {
                        xpc_array_apply(value, ^bool (size_t __unused index, object_t sub_value) {
                                xpc_type_t sub_sub_type = xpc_get_type(sub_value);
                                if (sub_sub_type == XPC_TYPE_DICTIONARY || sub_sub_type == XPC_TYPE_ARRAY) {
                                    *p_compact = false;
                                }
                                return true;
                            });
                    }
                    return true;
                });
        } else {
            compound_begin = "[";
            compound_end = "]";
            xpc_array_apply(object, ^bool (size_t __unused index, object_t value) {
                    xpc_type_t sub_type = xpc_get_type(value);
                    if (sub_type == XPC_TYPE_DICTIONARY || sub_type == XPC_TYPE_ARRAY) {
                        *p_compact = false;
                    }
                    return true;
                });
        }
        if (compact) {
            size_t i, count;
            const char **keys = NULL;
            char **values;
            char linebuf[160], *p_space;
            size_t space_avail = sizeof(linebuf);
            bool first = true;

            if (type == XPC_TYPE_DICTIONARY) {
                count = xpc_dictionary_get_count(object);
            } else {
                count = xpc_array_get_count(object);
            }

            values = malloc(count * sizeof(*values));
            if (values == NULL) {
                INFO("[CX%d] no memory", serial);
                return;
            }
            if (type == XPC_TYPE_DICTIONARY) {
                int index = 0, *p_index = &index;
                keys = malloc(count * sizeof(*keys));
                if (keys == NULL) {
                    free(values);
                    INFO("[CX%d] no memory", serial);
                }
                xpc_dictionary_apply(object, ^bool (const char *key, object_t value) {
                        values[*p_index] = cti_xpc_copy_description(value);
                        keys[*p_index] = key;
                        (*p_index)++;
                        return true;
                    });
            } else {
                xpc_array_apply(object, ^bool (size_t index, object_t value) {
                        values[index] = cti_xpc_copy_description(value);
                        return true;
                    });
            }
            p_space = linebuf;
            for (i = 0; i < count; i++) {
                char *str = values[i];
                size_t len;
                char *eol = "";
                bool emitted = false;
                if (str == NULL) {
                    str = "NULL";
                    len = 6;
                } else {
                    len = strlen(str) + 2;
                }
                if (type == XPC_TYPE_DICTIONARY) {
#ifdef __clang_analyzer__
                    len = 2;
#else
                    len += strlen(keys[i]) + 2; // "key: "
#endif
                }
                if (len + 1 > space_avail) {
                    if (i + 1 == count) {
                        eol = compound_end;
                    }
                    if (space_avail != sizeof(linebuf)) {
                        if (first) {
                            INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " " PUB_S_SRP
                                 PUB_S_SRP PUB_S_SRP, serial, context, command,
                                 indent, preamble, divide, compound_begin, linebuf, eol);
                            first = false;
                        } else {
                            INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " +" PUB_S_SRP
                                 PUB_S_SRP, serial, context, command,
                                 indent, preamble, divide, linebuf, eol);
                        }
                        space_avail = sizeof linebuf;
                        p_space = linebuf;
                    }
                    if (len + 1 > space_avail) {
                        if (type == XPC_TYPE_DICTIONARY) {
#ifndef __clang_analyzer__
                            if (first) {
                                INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " " PUB_S_SRP
                                     PUB_S_SRP ": " PUB_S_SRP PUB_S_SRP, serial, context, command,
                                     indent, preamble, divide, compound_begin, keys[i], str, eol);
                                first = false;
                            } else {
                                INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " +" PUB_S_SRP
                                     ": " PUB_S_SRP PUB_S_SRP, serial, context, command,
                                     indent, preamble, divide, keys[i], str, eol);
                            }
#endif
                        } else {
                            if (first) {
                                INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " " PUB_S_SRP
                                     PUB_S_SRP PUB_S_SRP, serial, context, command,
                                     indent, preamble, divide, compound_begin, str, eol);
                                first = false;
                            } else {
                                INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " +" PUB_S_SRP
                                     PUB_S_SRP, serial, context, command, indent, preamble, divide, str, eol);
                            }
                        }
                        emitted = true;
                    }
                }
                if (!emitted) {
                    if (type == XPC_TYPE_DICTIONARY) {
#ifndef __clang_analyzer__
                        snprintf(p_space, space_avail, "%s%s: %s%s", i == 0 ? "" : " ", keys[i], str, i + 1 == count ? "" : ",");
#endif
                    } else {
                        snprintf(p_space, space_avail, "%s%s%s", i == 0 ? "" : " ", str, i + 1 == count ? "" : ",");
                    }
                    len = strlen(p_space);
                    p_space += len;
                    space_avail -= len;
                }
                if (values[i] != NULL) {
                    free(values[i]);
                    values[i] = NULL;
                }
            }
            if (linebuf != p_space) {
                if (first) {
                    INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " " PUB_S_SRP PUB_S_SRP
                         PUB_S_SRP, serial, context, command,
                         indent, preamble, divide, compound_begin, linebuf, compound_end);
                } else {
                    INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " + " PUB_S_SRP PUB_S_SRP,
                         serial, context, command, indent, preamble, divide, linebuf, compound_end);
                }
            }
            free(values);
            if (keys != NULL) {
                free(keys);
            }
        } else {
            depth = strlen(indent);
            new_indent = malloc(depth + 3);
            if (new_indent == NULL) {
                new_indent = indent;
            } else {
                memset(new_indent, ' ', depth + 2);
                new_indent[depth + 2] = 0;
            }
            if (type == XPC_TYPE_DICTIONARY) {
                xpc_dictionary_apply(object, ^bool (const char *key, object_t value) {
                    cti_log_object(context, serial, command, key, ": ", value, new_indent);
                    return true;
                });
            } else {
                xpc_array_apply(object, ^bool (size_t index, object_t value) {
                    char numbuf[23];
                    snprintf(numbuf, sizeof(numbuf), "%zd", index);
                    cti_log_object(context, serial, command, numbuf, ": ", value, new_indent);
                    return true;
                });
            }
            if (new_indent != indent) {
                free(new_indent);
            }
        }
    } else {
        desc = cti_xpc_copy_description(object);
        INFO("[CX%d] " PUB_S_SRP "(" PUB_S_SRP "): " PUB_S_SRP PUB_S_SRP PUB_S_SRP " " PUB_S_SRP,
             serial, context, command, indent, preamble, divide, desc);
        free(desc);
    }
}

static void
cti_event_handler(object_t event, cti_connection_t conn_ref)
{
    if (event == XPC_ERROR_CONNECTION_INVALID) {
        INFO("[CX%d] (" PUB_S_SRP "): cleanup", conn_ref->serial, conn_ref->command_name);
        if (conn_ref->callback.reply != NULL) {
            conn_ref->internal_callback(conn_ref, event, kCTIStatus_Disconnected);
        } else {
            INFO("[CX%d] No callback", conn_ref->serial);
        }
        if (conn_ref->connection != NULL) {
            INFO("[CX%d] releasing connection %p", conn_ref->serial, conn_ref->connection);
            xpc_release(conn_ref->connection);
            conn_ref->connection = NULL;
        }
        return;
    }

    if (conn_ref->connection == NULL) {
        cti_log_object("cti_event_handler NULL connection",
                       conn_ref->serial, conn_ref->command_name, "", "", event, "");
        return;
    }

    if (xpc_get_type(event) == XPC_TYPE_DICTIONARY) {
        cti_log_object("cti_event_handler", conn_ref->serial, conn_ref->command_name, "", "", event, "");
        if (!conn_ref->checked_in) {
            object_t command_result = xpc_dictionary_get_value(event, "commandResult");
            int status = 0;
            if (command_result != NULL) {
                status = (int)xpc_int64_get_value(command_result);
                if (status == 0) {
                    object_t command_data = xpc_dictionary_get_value(event, "commandData");
                    if (command_data == NULL) {
                        status = 0;
                    } else {
                        object_t ret_value = xpc_dictionary_get_value(command_data, "ret");
                        if (ret_value == NULL) {
                            status = 0;
                        } else {
                            status = (int)xpc_int64_get_value(ret_value);
                        }
                    }
                }
            }

            if (status != 0) {
                conn_ref->internal_callback(conn_ref, event, kCTIStatus_UnknownError);
                INFO("[CX%d] canceling xpc connection %p", conn_ref->serial, conn_ref->connection);
                xpc_connection_cancel(conn_ref->connection);
            } else if (conn_ref->property_name != NULL) {
                // We're meant to both get the property and subscribe to events on it.
                object_t *dict = xpc_dictionary_create(NULL, NULL, 0);
                if (dict == NULL) {
                    ERROR("[CX%d] cti_event_handler(" PUB_S_SRP "): no memory, canceling %p.",
                          conn_ref->serial, conn_ref->command_name, conn_ref->connection);
                    xpc_connection_cancel(conn_ref->connection);
                } else {
                    object_t *array = xpc_array_create(NULL, 0);
                    if (array == NULL) {
                        ERROR("[CX%d] cti_event_handler(" PUB_S_SRP "): no memory, canceling %p.",
                              conn_ref->serial, conn_ref->command_name, conn_ref->connection);
                        xpc_connection_cancel(conn_ref->connection);
                    } else {
                        xpc_dictionary_set_string(dict, "command", "eventsOn");
                        xpc_dictionary_set_string(dict, "clientName", "srp-mdns-proxy");
                        xpc_dictionary_set_value(dict, "eventList", array);
                        xpc_array_set_string(array, XPC_ARRAY_APPEND, conn_ref->property_name);
                        conn_ref->property_name = NULL;
                        cti_log_object("cti_event_handler/events on",
                                       conn_ref->serial, conn_ref->command_name, "", "", dict, "");
                        INFO("[CX%d] sending message on connection %p", conn_ref->serial, conn_ref->connection);
                        xpc_connection_send_message_with_reply(conn_ref->connection, dict, conn_ref->client_queue,
                                                               ^(object_t in_event) {
                                                                   cti_event_handler(in_event, conn_ref);
                                                               });
                        xpc_release(array);
                    }
                    xpc_release(dict);
                }
            } else {
                object_t *message = conn_ref->first_command;
                conn_ref->first_command = NULL;
                cti_log_object("cti_event_handler/command is",
                               conn_ref->serial, conn_ref->command_name, "", "", message, "");
                conn_ref->checked_in = true;

                INFO("[CX%d] sending message on connection %p", conn_ref->serial, conn_ref->connection);
                xpc_connection_send_message_with_reply(conn_ref->connection, message, conn_ref->client_queue,
                                                       ^(object_t in_event) {
                                                           cti_event_handler(in_event, conn_ref);
                                                       });
                xpc_release(message);
            }
        } else {
            conn_ref->internal_callback(conn_ref, event, kCTIStatus_NoError);
        }
    } else {
        cti_log_object("cti_event_handler/other", conn_ref->serial, conn_ref->command_name, "", "", event, "");
        ERROR("[CX%d] cti_event_handler: Unexpected Connection Error [" PUB_S_SRP "]",
              conn_ref->serial, xpc_dictionary_get_string(event, XPC_ERROR_KEY_DESCRIPTION));
        conn_ref->internal_callback(conn_ref, NULL, kCTIStatus_DaemonNotRunning);
        if (event != XPC_ERROR_CONNECTION_INTERRUPTED) {
            INFO("[CX%d] canceling xpc connection %p", conn_ref->serial, conn_ref->connection);
            xpc_connection_cancel(conn_ref->connection);
        }
    }
}

// Creates a new cti_ Connection Reference(cti_connection_t)
static cti_status_t
init_connection(cti_connection_t *ref, const char *servname, object_t *dict, const char *command_name,
                const char *property_name, const char *return_property_name, void *context, cti_callback_t app_callback,
                cti_internal_callback_t internal_callback, run_context_t client_queue,
                const char *file, int line)
{
    // Use an cti_connection_t on the stack to be captured in the blocks below, rather than
    // capturing the cti_connection_t* owned by the client
#ifdef MALLOC_DEBUG_LOGGING
    cti_connection_t conn_ref = debug_calloc(1, sizeof(struct _cti_connection_t), file, line);
#else
    cti_connection_t conn_ref = calloc(1, sizeof(struct _cti_connection_t));
#endif
    if (conn_ref == NULL) {
        ERROR("no memory to allocate!");
        return kCTIStatus_NoMemory;
    }
    conn_ref->serial = client_serial_number;
    client_serial_number++;

    // We always retain a reference for the caller, even if the caller doesn't actually hold the reference.
    // Calls that do not result in repeated callbacks release this reference after calling the callback.
    // Such calls do not return a reference to the caller, so there is no chance of a double release.
    // Calls that result in repeated callbacks have to release the reference by calling cti_events_discontinue.
    // If this isn't done, the reference will never be released.
    RETAIN(conn_ref, cti_connection);

    if (client_queue == NULL) {
        client_queue = dispatch_get_main_queue();
    }

    // Initialize the cti_connection_t
    dispatch_retain(client_queue);
    conn_ref->command_name = command_name;
    conn_ref->property_name = property_name;
    conn_ref->return_property_name = return_property_name;
    conn_ref->context = context;
    conn_ref->client_queue = client_queue;
    conn_ref->callback = app_callback;
    conn_ref->internal_callback = internal_callback;
    conn_ref->connection = xpc_connection_create_mach_service(servname, conn_ref->client_queue,
                                                              XPC_CONNECTION_MACH_SERVICE_PRIVILEGED);
    INFO("[CX%d] xpc connection: %p", conn_ref->serial, conn_ref->connection);
    conn_ref->first_command = dict;
    xpc_retain(dict);

    cti_log_object("init_connection/command", conn_ref->serial, conn_ref->command_name, "", "", dict, "");

    if (conn_ref->connection == NULL)
    {
        ERROR("conn_ref/lib_q is NULL");
        if (conn_ref != NULL) {
            RELEASE_HERE(conn_ref, cti_connection);
        }
        return kCTIStatus_NoMemory;
    }

    RETAIN_HERE(conn_ref, cti_connection); // For the event handler.
    xpc_connection_set_event_handler(conn_ref->connection, ^(object_t event) { cti_event_handler(event, conn_ref); });
    xpc_connection_set_finalizer_f(conn_ref->connection, cti_xpc_connection_finalize);
    xpc_connection_set_context(conn_ref->connection, conn_ref);
    xpc_connection_resume(conn_ref->connection);

    char srp_name[] = "srp-mdns-proxy";
    char client_name[sizeof(srp_name) + 20];
    snprintf(client_name, sizeof client_name, "%s-%d", srp_name, conn_ref->serial);

    object_t checkin_command = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(checkin_command, "command", "checkIn");
    xpc_dictionary_set_string(checkin_command, "clientName", client_name);

    cti_log_object("init_connection/checkin", conn_ref->serial, conn_ref->command_name, "", "", checkin_command, "");
    INFO("[CX%d] sending message on connection %p", conn_ref->serial, conn_ref->connection);
    xpc_connection_send_message_with_reply(conn_ref->connection, checkin_command, conn_ref->client_queue,
                                           ^(object_t event) { cti_event_handler(event, conn_ref); });

    xpc_release(checkin_command);
    if (ref) {
        *ref = conn_ref;
    }
    return kCTIStatus_NoError;
}

static cti_status_t
setup_for_command(cti_connection_t *ref, run_context_t client_queue, const char *command_name,
                  const char *property_name, const char *return_property_name, object_t dict, const char *command,
                  void *context, cti_callback_t app_callback, cti_internal_callback_t internal_callback,
                  bool events_only, const char *file, int line)
{
    cti_status_t errx = kCTIStatus_NoError;

    // Sanity Checks
    if (app_callback.reply == NULL || internal_callback == NULL)
    {
        ERROR(PUB_S_SRP ": NULL cti_connection_t OR Callback OR Client_Queue parameter", command_name);
        return kCTIStatus_BadParam;
    }

    // Get conn_ref from init_connection()
    if (events_only) {
        xpc_dictionary_set_string(dict, "command", "eventsOn");
        object_t *array = xpc_array_create(NULL, 0);
        if (array != NULL) {
            xpc_array_set_string(array, XPC_ARRAY_APPEND, property_name);
            xpc_dictionary_set_value(dict, "eventList", array);
            property_name = NULL;
            xpc_release(array);
        } else {
            return kCTIStatus_NoMemory;
        }
    } else {
        xpc_dictionary_set_string(dict, "command", command);
    }

    errx = init_connection(ref, "com.apple.wpantund.xpc", dict, command_name, property_name, return_property_name,
                           context, app_callback, internal_callback, client_queue, file, line);
    if (errx) // On error init_connection() leaves *conn_ref set to NULL
    {
        ERROR(PUB_S_SRP ": Since init_connection() returned %d error returning w/o sending msg", command_name, errx);
        return errx;
    }

    return errx;
}

static void
cti_internal_event_reply_callback(cti_connection_t NONNULL conn_ref, object_t __unused reply, cti_status_t status)
{
    cti_reply_t callback;
    INFO("[CX%d] conn_ref = %p", conn_ref != NULL ? conn_ref->serial : 0, conn_ref);
    callback = conn_ref->callback.reply;
    if (callback != NULL) {
        callback(conn_ref->context, status);
    }
}

static void
cti_internal_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status)
{
    cti_internal_event_reply_callback(conn_ref, reply, status);
    conn_ref->callback.reply = NULL;
    if (conn_ref->connection != NULL) {
        INFO("[CX%d] canceling connection %p", conn_ref->serial, conn_ref->connection);
        xpc_connection_cancel(conn_ref->connection);
    }
    cti_connection_release(conn_ref);
}

cti_status_t
cti_add_service_(srp_server_t *UNUSED server, void *context, cti_reply_t callback, run_context_t client_queue,
                 uint32_t enterprise_number, const uint8_t *NONNULL service_data, size_t service_data_length,
                 const uint8_t *server_data, size_t server_data_length, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_data(dict, "service_data", service_data, service_data_length);
    if (server_data != NULL) {
        xpc_dictionary_set_data(dict, "server_data", server_data, server_data_length);
    }
    xpc_dictionary_set_uint64(dict, "enterprise_number", enterprise_number);
    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "ServiceAdd");
    xpc_dictionary_set_bool(dict, "stable", true);

    errx = setup_for_command(NULL, client_queue, "add_service", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_remove_service_(srp_server_t *UNUSED server, void *context, cti_reply_t callback, run_context_t client_queue,
                    uint32_t enterprise_number, const uint8_t *NONNULL service_data, size_t service_data_length,
                    const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_data(dict, "service_data", service_data, service_data_length);
    xpc_dictionary_set_uint64(dict, "enterprise_number", enterprise_number);
    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "ServiceRemove");

    errx = setup_for_command(NULL, client_queue, "remove_service", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

static cti_status_t
cti_do_prefix(void *context, cti_reply_t callback, run_context_t client_queue,
              struct in6_addr *prefix, int prefix_length, bool on_mesh, bool preferred, bool slaac, bool stable, bool adding,
              int priority, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    if (dict == NULL) {
        ERROR("cti_do_prefix: no memory for command dictionary.");
        return kCTIStatus_NoMemory;
    }
    xpc_dictionary_set_bool(dict, "preferred", preferred);
    if (adding) {
        xpc_dictionary_set_uint64(dict, "preferredLifetime", ND6_INFINITE_LIFETIME);
        xpc_dictionary_set_uint64(dict, "validLifetime", ND6_INFINITE_LIFETIME);
    } else {
        xpc_dictionary_set_uint64(dict, "preferredLifetime", 0);
        xpc_dictionary_set_uint64(dict, "validLifetime", 0);
    }
    xpc_dictionary_set_int64(dict, "prefix_length", 16);
    xpc_dictionary_set_bool(dict, "dhcp", false);
    xpc_dictionary_set_data(dict, "prefix", prefix, sizeof(*prefix));
    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_uint64(dict, "prefix_len_in_bits", prefix_length);
    xpc_dictionary_set_bool(dict, "slaac", slaac);
    xpc_dictionary_set_bool(dict, "onMesh", on_mesh);
    xpc_dictionary_set_bool(dict, "configure", false);
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "ConfigGateway");
    xpc_dictionary_set_bool(dict, "stable", stable);
    xpc_dictionary_set_bool(dict, "defaultRoute", true);
    xpc_dictionary_set_int64(dict, "priority", priority);

    errx = setup_for_command(NULL, client_queue, adding ? "add_prefix" : "remove_prefix", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_add_prefix_(srp_server_t *UNUSED server, void *context, cti_reply_t callback, run_context_t client_queue,
                struct in6_addr *prefix, int prefix_length, bool on_mesh, bool preferred, bool slaac, bool stable,
                int priority, const char *file, int line)
{
    return cti_do_prefix(context, callback, client_queue, prefix, prefix_length, on_mesh, preferred, slaac, stable,
                         true, priority, file, line);
}

cti_status_t
cti_remove_prefix_(srp_server_t *UNUSED server, void *NULLABLE context, cti_reply_t NONNULL callback,
                   run_context_t NULLABLE client_queue, struct in6_addr *NONNULL prefix, int prefix_length,
                   const char *file, int line)
{
    return cti_do_prefix(context, callback, client_queue, prefix, prefix_length, false, false, false, false, false, 0,
                         file, line);
}

cti_status_t
cti_add_route_(srp_server_t *UNUSED server, void *context, cti_reply_t callback, run_context_t client_queue,
               struct in6_addr *prefix, int prefix_length, int priority, int domain_id,
               bool stable, bool nat64, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    if (dict == NULL) {
        ERROR("cti_do_prefix: no memory for command dictionary.");
        return kCTIStatus_NoMemory;
    }
    xpc_dictionary_set_string(dict, "method", "RouteAdd");
    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_uint64(dict, "prefix_bits_len", prefix_length);
    xpc_dictionary_set_data(dict, "prefix_bits", prefix, sizeof(*prefix));
    xpc_dictionary_set_uint64(dict, "domain_id", domain_id);
    xpc_dictionary_set_int64(dict, "priority", priority);
    xpc_dictionary_set_bool(dict, "stable", stable);
    xpc_dictionary_set_bool(dict, "nat64", nat64);

    errx = setup_for_command(NULL, client_queue, "add_route", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_remove_route_(srp_server_t *UNUSED server, void *NULLABLE context, cti_reply_t NONNULL callback,
                  run_context_t NULLABLE client_queue, struct in6_addr *NONNULL prefix, int prefix_length,
                  int domain_id, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    if (dict == NULL) {
        ERROR("cti_do_prefix: no memory for command dictionary.");
        return kCTIStatus_NoMemory;
    }
    xpc_dictionary_set_string(dict, "method", "RouteRemove");
    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_uint64(dict, "prefix_len_in_bits", prefix_length);
    xpc_dictionary_set_data(dict, "prefix_bytes", prefix, sizeof(*prefix));
    xpc_dictionary_set_uint64(dict, "domain_id", domain_id);

    errx = setup_for_command(NULL, client_queue, "remove_route", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

static void
cti_internal_string_event_reply(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_string_property_reply_t callback = conn_ref->callback.string_property_reply;
    xpc_retain(reply);
    cti_status_t status = status_in;
    const char *string_value = NULL;
    if (status == kCTIStatus_NoError) {
        // Initial reply (events do a get as well as subscribing to events, and the responses look different.
        xpc_object_t command_result_value = xpc_dictionary_get_value(reply, "commandResult");
        if (command_result_value != NULL) {
            uint64_t command_result = xpc_int64_get_value(command_result_value);
            if (command_result != 0) {
                ERROR("[CX%d] nonzero result %llu", conn_ref->serial, command_result);
                status = kCTIStatus_UnknownError;
            } else {
                object_t result_dictionary = xpc_dictionary_get_dictionary(reply, "commandData");
                if (status == kCTIStatus_NoError) {
                    if (result_dictionary != NULL) {
                        const char *property_name = xpc_dictionary_get_string(result_dictionary, "property_name");
                        if (property_name == NULL || strcmp(property_name, conn_ref->return_property_name)) {
                            status = kCTIStatus_UnknownError;
                        } else {
                            string_value = xpc_dictionary_get_string(result_dictionary, "value");
                            if (string_value == NULL) {
                                status = kCTIStatus_UnknownError;
                            }
                        }
                    } else {
                        status = kCTIStatus_UnknownError;
                    }
                }
            }
        } else {
            xpc_object_t event_data_dict = xpc_dictionary_get_dictionary(reply, "eventData");
            if (event_data_dict == NULL) {
                ERROR("[CX%d] no eventData dictionary", conn_ref->serial);
                status = kCTIStatus_UnknownError;
            } else {
                // Event data looks like { "v_type" : 13, "path": <string>, "value": <string>, "key": <property_name> }
                xpc_object_t value_array = xpc_dictionary_get_array(event_data_dict, "value");
                if (value_array == NULL) {
                    ERROR("[CX%d] eventData dictionary contains no 'value' key", conn_ref->serial);
                    status = kCTIStatus_UnknownError;
                } else {
                    size_t count = xpc_array_get_count(value_array);
                    if (count < 1) {
                        ERROR("[CX%d] eventData value array has no elements", conn_ref->serial);
                        status = kCTIStatus_UnknownError;
                    } else {
                        if (count > 1) {
                            ERROR("[CX%d] eventData value array has %zd elements", conn_ref->serial, count);
                            // This is weird, but we're not going to deliberately fail because of it.
                        }
                        string_value = xpc_array_get_string(value_array, 0);
                        if (string_value == NULL) {
                            ERROR("[CX%d] eventData value array's first element is not a string", conn_ref->serial);
                            status = kCTIStatus_UnknownError;
                        }
                    }
                }
            }
        }
    }
    if (callback != NULL) {
        callback(conn_ref->context, string_value, status);
    }
    xpc_release(reply);
}

static void
cti_internal_string_property_reply(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_internal_string_event_reply(conn_ref, reply, status_in);
    conn_ref->callback.reply = NULL;
    if (conn_ref->connection != NULL) {
        INFO("[CX%d] canceling connection %p", conn_ref->serial, conn_ref->connection);
        xpc_connection_cancel(conn_ref->connection);
    }
    RELEASE_HERE(conn_ref, cti_connection);
}

cti_status_t
cti_get_tunnel_name_(srp_server_t *UNUSED server, void *NULLABLE context, cti_string_property_reply_t NONNULL callback,
                     run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.string_property_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "Config:TUN:InterfaceName");

    errx = setup_for_command(NULL, client_queue, "get_tunnel_name", NULL, "Config:TUN:InterfaceName", dict, "WpanctlCmd",
                             context, app_callback, cti_internal_string_property_reply, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_track_active_data_set_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
                           cti_reply_t NONNULL callback,
                           run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");

    errx = setup_for_command(ref, client_queue, "track_active_data_set", "ActiveDataSetChanged", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_event_reply_callback, true, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_get_mesh_local_prefix_(srp_server_t *UNUSED server, void *NULLABLE context,
                           cti_string_property_reply_t NONNULL callback,
                           run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.string_property_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "IPv6:MeshLocalPrefix");

    errx = setup_for_command(NULL, client_queue, "get_mesh_local_prefix", NULL,
                             "IPv6:MeshLocalPrefix", dict, "WpanctlCmd",
                             context, app_callback, cti_internal_string_property_reply, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_get_mesh_local_address_(srp_server_t *UNUSED server, void *NULLABLE context,
                            cti_string_property_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                            const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.string_property_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "IPv6:MeshLocalAddress");

    errx = setup_for_command(NULL, client_queue, "get_mesh_local_address", NULL,
                             "IPv6:MeshLocalAddress", dict, "WpanctlCmd", context, app_callback,
                             cti_internal_string_property_reply, false, file, line);
    xpc_release(dict);

    return errx;
}

static cti_status_t
cti_event_or_response_extract(object_t *reply, object_t *result_dictionary)
{
    object_t *result = xpc_dictionary_get_dictionary(reply, "commandData");
    if (result == NULL) {
        result = xpc_dictionary_get_dictionary(reply, "eventData");
    } else {
        int command_status = (int)xpc_dictionary_get_int64(reply, "commandResult");
        if (command_status != 0) {
            INFO("nonzero status %d", command_status);
            return kCTIStatus_UnknownError;
        }
    }
    if (result != NULL) {
        *result_dictionary = result;
        return kCTIStatus_NoError;
    }
    INFO("null result");
    return kCTIStatus_UnknownError;
}

static void
cti_internal_state_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_state_reply_t callback = conn_ref->callback.state_reply;
    cti_network_state_t state = kCTI_NCPState_Unknown;
    cti_status_t status = status_in;
    if (status == kCTIStatus_NoError) {
        object_t result_dictionary = NULL;
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            const char *state_name = xpc_dictionary_get_string(result_dictionary, "value");
            if (state_name == NULL) {
                status = kCTIStatus_UnknownError;
            } else if (!strcmp(state_name, "uninitialized")) {
                state = kCTI_NCPState_Uninitialized;
            } else if (!strcmp(state_name, "uninitialized:fault")) {
                state = kCTI_NCPState_Fault;
            } else if (!strcmp(state_name, "uninitialized:upgrading")) {
                state = kCTI_NCPState_Upgrading;
            } else if (!strcmp(state_name, "offline:deep-sleep")) {
                state = kCTI_NCPState_DeepSleep;
            } else if (!strcmp(state_name, "offline")) {
                state = kCTI_NCPState_Offline;
            } else if (!strcmp(state_name, "offline:commissioned")) {
                state = kCTI_NCPState_Commissioned;
            } else if (!strcmp(state_name, "associating")) {
                state = kCTI_NCPState_Associating;
            } else if (!strcmp(state_name, "associating:credentials-needed")) {
                state = kCTI_NCPState_CredentialsNeeded;
            } else if (!strcmp(state_name, "associated")) {
                state = kCTI_NCPState_Associated;
            } else if (!strcmp(state_name, "associated:no-parent")) {
                state = kCTI_NCPState_Isolated;
            } else if (!strcmp(state_name, "associated:netwake-asleep")) {
                state = kCTI_NCPState_NetWake_Asleep;
            } else if (!strcmp(state_name, "associated:netwake-waking")) {
                state = kCTI_NCPState_NetWake_Waking;
            }
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, state, status);
    }
}

cti_status_t
cti_get_state_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
               cti_state_reply_t NONNULL callback, run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.state_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "NCP:State");

    errx = setup_for_command(ref, client_queue, "get_state", "NCP:State", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_state_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

typedef const char *cti_property_name_t;

static void
cti_internal_uint64_property_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_uint64_property_reply_t callback = conn_ref->callback.uint64_property_reply;
    uint64_t uint64_val = 0;
    cti_status_t status = status_in;
    if (status == kCTIStatus_NoError) {
        object_t result_dictionary = NULL;
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t value = xpc_dictionary_get_value(result_dictionary, "value");
            if (value == NULL) {
                ERROR("[CX%d] no property value.", conn_ref->serial);
            } else {
                if (xpc_get_type(value) == XPC_TYPE_UINT64) {
                    uint64_val = xpc_dictionary_get_uint64(result_dictionary, "value");
                } else if (xpc_get_type(value) == XPC_TYPE_ARRAY) {
                    size_t count = xpc_array_get_count(value);
                    if (count != sizeof(uint64_val)) {
                        goto fail;
                    }
                    for (size_t i = 0; i < sizeof(uint64_val); i++) {
                        object_t element = xpc_array_get_value(value, i);
                        if (xpc_get_type(element) != XPC_TYPE_UINT64) {
                            goto fail;
                        }
                        uint64_t ev = xpc_array_get_uint64(value, i);
                        if (ev > 255) {
                            goto fail;
                        }
                        uint64_val = (uint64_val << 8) | ev;
                    }
                } else {
                    char *value_string;
                fail:
                    value_string = xpc_copy_description(value);
                    ERROR("[CX%d] property value is " PUB_S_SRP " instead of uint64_t or array of uint64_t byte values.",
                          conn_ref->serial, value_string);
                    free(value_string);
                    status = kCTIStatus_Invalid;
                }
            }
        }
    }
    if (callback != NULL) {
        callback(conn_ref->context, uint64_val, status);
    }
}

static cti_status_t
cti_get_uint64_property(cti_connection_t *ref, void *NULLABLE context, cti_uint64_property_reply_t NONNULL callback,
                        run_context_t NULLABLE client_queue, cti_property_name_t property_name, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.uint64_property_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", property_name);

    errx = setup_for_command(ref, client_queue, "get_uint64_property", property_name, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_uint64_property_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_get_partition_id_(srp_server_t *UNUSED server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                      cti_uint64_property_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                      const char *NONNULL file, int line)
{
    return cti_get_uint64_property(ref, context, callback, client_queue, kCTIPropertyPartitionID, file, line);
}

cti_status_t
cti_get_extended_pan_id_(srp_server_t *UNUSED server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                         cti_uint64_property_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                         const char *NONNULL file, int line)
{
    return cti_get_uint64_property(ref, context, callback, client_queue, kCTIPropertyExtendedPANID, file, line);
}

static void
cti_internal_network_node_type_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_network_node_type_reply_t callback = conn_ref->callback.network_node_type_reply;
    cti_network_node_type_t network_node_type = kCTI_NetworkNodeType_Unknown;
    cti_status_t status = status_in;
    if (status == kCTIStatus_NoError) {
        object_t result_dictionary = NULL;
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t value = xpc_dictionary_get_value(result_dictionary, "value");
            if (value == NULL) {
                ERROR("[CX%d] No node type returned.", conn_ref->serial);
            } else if (xpc_get_type(value) != XPC_TYPE_STRING) {
                char *value_string = xpc_copy_description(value);
                ERROR("[CX%d] node type type is " PUB_S_SRP " instead of string.", conn_ref->serial, value_string);
                free(value_string);
            } else {
                const char *node_type_name = xpc_dictionary_get_string(result_dictionary, "value");
                if (!strcmp(node_type_name, "unknown")) {
                    network_node_type = kCTI_NetworkNodeType_Unknown;
                } else if (!strcmp(node_type_name, "router")) {
                        network_node_type = kCTI_NetworkNodeType_Router;
                } else if (!strcmp(node_type_name, "end-device")) {
                    network_node_type = kCTI_NetworkNodeType_EndDevice;
                } else if (!strcmp(node_type_name, "sleepy-end-device")) {
                    network_node_type = kCTI_NetworkNodeType_SleepyEndDevice;
                } else if (!strcmp(node_type_name, "synchronized-sleepy-end-device")) {
                    network_node_type = kCTI_NetworkNodeType_SynchronizedSleepyEndDevice;
                } else if (!strcmp(node_type_name, "nl-lurker")) {
                    network_node_type = kCTI_NetworkNodeType_NestLurker;
                } else if (!strcmp(node_type_name, "commissioner")) {
                    network_node_type = kCTI_NetworkNodeType_Commissioner;
                } else if (!strcmp(node_type_name, "leader")) {
                    network_node_type = kCTI_NetworkNodeType_Leader;
                } else if (!strcmp(node_type_name, "sleepy-router")) {
                    network_node_type = kCTI_NetworkNodeType_SleepyRouter;
                }
            }
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, network_node_type, status);
    }
}

cti_status_t
cti_get_network_node_type_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
                           cti_network_node_type_reply_t NONNULL callback,
                           run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.network_node_type_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "Network:NodeType");

    errx = setup_for_command(ref, client_queue, "get_network_node_type", "Network:NodeType", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_network_node_type_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

static void
cti_service_finalize(cti_service_t *service)
{
    if (service->server != NULL) {
        free(service->server);
    }
    if (service->service != NULL) {
        free(service->service);
    }
    free(service);
}

static void
cti_service_vec_finalize(cti_service_vec_t *services)
{
    size_t i;

    if (services->services != NULL) {
        for (i = 0; i < services->num; i++) {
            if (services->services[i] != NULL) {
                RELEASE_HERE(services->services[i], cti_service);
            }
        }
        free(services->services);
    }
    free(services);
}

cti_service_vec_t *
cti_service_vec_create_(size_t num_services, const char *file, int line)
{
    cti_service_vec_t *services = calloc(1, sizeof(*services));
    if (services != NULL) {
        if (num_services != 0) {
            services->services = calloc(num_services, sizeof(cti_service_t *));
            if (services->services == NULL) {
                free(services);
                return NULL;
            }
        }
        services->num = num_services;
        RETAIN(services, cti_service_vec);
    }
    return services;
}

void
cti_service_vec_release_(cti_service_vec_t *services, const char *file, int line)
{
    RELEASE(services, cti_service_vec);
}

cti_service_t *
cti_service_create_(uint64_t enterprise_number, uint16_t rloc16, uint16_t service_type,
                    uint16_t service_version, uint8_t *service_data, size_t service_length,
                    uint8_t *server, size_t server_length, uint16_t service_id, int flags, const char *file, int line)
{
    cti_service_t *service = calloc(1, sizeof(*service));
    if (service != NULL) {
        service->enterprise_number = enterprise_number;
        service->service_type = service_type;
        service->service_version = service_version;
        service->rloc16 = rloc16;
        service->service = service_data;
        service->service_length = service_length;
        service->server = server;
        service->server_length = server_length;
        service->service_id = service_id;
        service->flags = flags;
        RETAIN(service, cti_service);
    }
    return service;
}

void
cti_service_release_(cti_service_t *service, const char *file, int line)
{
    RELEASE(service, cti_service);
}

static uint8_t *
cti_array_to_bytes(object_t array, size_t *length_ret, const char *log_name)
{
    size_t length = xpc_array_get_count(array);
    size_t i;
    uint8_t *ret;

    ret = malloc(length);
    if (ret == NULL) {
        ERROR(PUB_S_SRP ": no memory for return buffer", log_name);
        return NULL;
    }

    for (i = 0; i < length; i++) {
        uint64_t v = xpc_array_get_uint64(array, i);
        ret[i] = v;
    }
    *length_ret = length;
    return ret;
}

static cti_status_t
cti_parse_services_array(cti_service_vec_t **services, object_t services_array)
{
    size_t services_array_length = xpc_array_get_count(services_array);
    size_t i, j;
    cti_service_vec_t *service_vec;
    cti_service_t *service;
    cti_status_t status = kCTIStatus_NoError;

    service_vec = cti_service_vec_create(services_array_length);
    if (service_vec == NULL) {
        return kCTIStatus_NoMemory;
    }

    // Array of arrays
    for (i = 0; i < services_array_length; i++) {
        object_t service_array = xpc_array_get_value(services_array, i);
        int match_count = 0;
        bool matched_enterprisenum = false;
        bool matched_origin = false;
        bool matched_rloc16 = false;
        bool matched_serverdata = false;
        bool matched_servicedata = false;
        bool matched_stable = false;
        bool matched_service_id = false;
        uint64_t enterprise_number = 0;
        uint16_t rloc16 = 0;
        uint8_t *server_data = NULL;
        size_t server_data_length = 0;
        uint8_t *service_data = NULL;
        size_t service_data_length = 0;
        int flags = 0;
        uint16_t service_id = 0;

        if (service_array == NULL) {
            ERROR("Unable to get service array %zd", i);
        } else {
            size_t service_array_length = xpc_array_get_count(service_array);
            for (j = 0; j < service_array_length; j++) {
                object_t *array_sub_dict = xpc_array_get_value(service_array, j);
                if (array_sub_dict == NULL) {
                    ERROR("can't get service_array %zd subdictionary %zd", i, j);
                    goto service_array_element_failed;
                } else {
                    const char *key = xpc_dictionary_get_string(array_sub_dict, "key");
                    if (key == NULL) {
                        ERROR("Invalid services array %zd subdictionary %zd: no key", i, j);
                        goto service_array_element_failed;
                    } else if (!strcmp(key, "EnterpriseNumber")) {
                        if (matched_enterprisenum) {
                            ERROR("services array %zd: Enterprise number appears twice.", i);
                            goto service_array_element_failed;
                        }
                        enterprise_number = xpc_dictionary_get_uint64(array_sub_dict, "value");
                        matched_enterprisenum = true;
                    } else if (!strcmp(key, "Origin")) {
                        if (matched_origin) {
                            ERROR("Services array %zd: Origin appears twice.", i);
                            goto service_array_element_failed;
                        }
                        const char *origin_string = xpc_dictionary_get_string(array_sub_dict, "value");
                        if (origin_string == NULL) {
                            ERROR("Unable to get origin string from services array %zd", i);
                            goto service_array_element_failed;
                        } else if (!strcmp(origin_string, "user")) {
                            // Not NCP
                        } else if (!strcmp(origin_string, "ncp")) {
                            flags |= kCTIFlag_NCP;
                        } else {
                            ERROR("unknown origin " PUB_S_SRP, origin_string);
                            goto service_array_element_failed;
                        }
                        matched_origin = true;
                    } else if (!strcmp(key, "ServerData")) {
                        if (matched_serverdata) {
                            ERROR("Services array %zd: Server data appears twice.", i);
                            goto service_array_element_failed;
                        }
                        server_data = cti_array_to_bytes(xpc_dictionary_get_array(array_sub_dict, "value"),
                                                         &server_data_length, "Server data");
                        if (server_data == NULL) {
                            goto service_array_element_failed;
                        }
                        matched_serverdata = true;
                    } else if (!strcmp(key, "ServiceData")) {
                        if (matched_servicedata) {
                            ERROR("Services array %zd: Service data appears twice.", i);
                            goto service_array_element_failed;
                        }
                        service_data = cti_array_to_bytes(xpc_dictionary_get_array(array_sub_dict, "value"),
                                                          &service_data_length, "Service data");
                        if (service_data == NULL) {
                            goto service_array_element_failed;
                        }
                        matched_servicedata = true;
                    } else if (!strcmp(key, "Stable")) {
                        if (matched_stable) {
                            ERROR("Services array %zd: Stable state appears twice.", i);
                            goto service_array_element_failed;
                        }
                        if (xpc_dictionary_get_bool(array_sub_dict, "value")) {
                            flags |= kCTIFlag_Stable;
                        }
                        matched_stable = true;
                    } else if (!strcmp(key, "RLOC16")) {
                        if (matched_rloc16) {
                            ERROR("Services array %zd: Stable state appears twice.", i);
                            goto service_array_element_failed;
                        }
                        rloc16 = (uint16_t)xpc_dictionary_get_uint64(array_sub_dict, "value");
                        matched_rloc16 = true;
                    } else if (!strcmp(key, "ServiceId")) {
                        if (matched_service_id) {
                            ERROR("Services array %zd: Stable state appears twice.", i);
                            goto service_array_element_failed;
                        }
                        service_id = (uint16_t)xpc_dictionary_get_int64(array_sub_dict, "value");
                        matched_service_id = true;
                    } else {
                        INFO("Unknown key in service array %zd subdictionary %zd: " PUB_S_SRP, i, j, key);
                        // Not a failure, but don't count it.
                        continue;
                    }
                    match_count++;
                }
            }
            if (match_count != 7) {
                ERROR("expecting %d sub-dictionaries to service array %zd, but got %d.",
                      7, i, match_count);
                // No service ID is not fatal (yet), but if some other data is missing, that /is/ fatal.
                if (match_count < 6 || (match_count == 6 && matched_service_id)) {
                    goto service_array_element_failed;
                }
            }
            uint16_t service_type, service_version;
            if (enterprise_number == THREAD_ENTERPRISE_NUMBER) {
                // two-byte service data for anycast service while on-byte for unicast
                // and pref-id
                if (service_data_length != 1 && service_data_length != 2) {
                    INFO("Invalid service data: length = %zd", service_data_length);
                    goto service_array_element_failed;
                }
                service_type = service_data[0];
                service_version = 1;
            } else {
                // We don't support any other enterprise numbers.
                service_type = service_version = 0;
            }

            service = cti_service_create(enterprise_number, rloc16, service_type, service_version,
                                         service_data, service_data_length, server_data,
                                         server_data_length, service_id, flags);
            if (service == NULL) {
                ERROR("Unable to store service %lld %d %d: out of memory.", enterprise_number,
                      service_type, service_version);
            } else {
                service_data = NULL;
                server_data = NULL;
                service_vec->services[i] = service;
            }
            goto done_with_service_array;
        service_array_element_failed:
            if (status == kCTIStatus_NoError) {
                status = kCTIStatus_UnknownError;
            }
        done_with_service_array:
            if (server_data != NULL) {
                free(server_data);
            }
            if (service_data != NULL) {
                free(service_data);
            }
        }
    }
    if (status == kCTIStatus_NoError) {
        *services = service_vec;
    } else {
        if (service_vec != NULL) {
            RELEASE_HERE(service_vec, cti_service_vec);
        }
    }
    return status;
}

static void
cti_internal_service_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_service_reply_t callback = conn_ref->callback.service_reply;
    cti_service_vec_t *vec = NULL;
    cti_status_t status = status_in;
    if (status == kCTIStatus_NoError) {
        object_t result_dictionary = NULL;
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t *value = xpc_dictionary_get_array(result_dictionary, "value");
            if (value == NULL) {
                INFO("[CX%d] services array not present in Thread:Services event.", conn_ref->serial);
            } else {
                status = cti_parse_services_array(&vec, value);
            }
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, vec, status);
    }
    if (vec != NULL) {
        RELEASE_HERE(vec, cti_service_vec);
    }
}

cti_status_t
cti_get_service_list_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
                      cti_service_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                      const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.service_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "Thread:Services");

    errx = setup_for_command(ref, client_queue, "get_service_list", "Thread:Services", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_service_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

static void
cti_prefix_finalize(cti_prefix_t *prefix)
{
    free(prefix);
}

static void
cti_prefix_vec_finalize(cti_prefix_vec_t *prefixes)
{
    size_t i;

    if (prefixes->prefixes != NULL) {
        for (i = 0; i < prefixes->num; i++) {
            if (prefixes->prefixes[i] != NULL) {
                RELEASE_HERE(prefixes->prefixes[i], cti_prefix);
            }
        }
        free(prefixes->prefixes);
    }
    free(prefixes);
}

cti_prefix_vec_t *
cti_prefix_vec_create_(size_t num_prefixes, const char *file, int line)
{
    cti_prefix_vec_t *prefixes = calloc(1, sizeof(*prefixes));
    if (prefixes != NULL) {
        if (num_prefixes != 0) {
            prefixes->prefixes = calloc(num_prefixes, sizeof(cti_prefix_t *));
            if (prefixes->prefixes == NULL) {
                free(prefixes);
                return NULL;
            }
        }
        prefixes->num = num_prefixes;
        RETAIN(prefixes, cti_prefix_vec);
    }
    return prefixes;
}

void
cti_prefix_vec_release_(cti_prefix_vec_t *prefixes, const char *file, int line)
{
    RELEASE(prefixes, cti_prefix_vec);
}

cti_prefix_t *
cti_prefix_create_(struct in6_addr *prefix, int prefix_length, int metric, int flags, int rloc, bool stable, bool ncp,
                   const char *file, int line)
{
    cti_prefix_t *prefix_ret = calloc(1, sizeof(*prefix_ret));
    if (prefix != NULL) {
        prefix_ret->prefix = *prefix;
        prefix_ret->prefix_length = prefix_length;
        prefix_ret->metric = metric;
        prefix_ret->flags = flags;
        prefix_ret->rloc = rloc;
        prefix_ret->stable = stable;
        prefix_ret->ncp = ncp;
        RETAIN(prefix_ret, cti_prefix);
    }
    return prefix_ret;
}

void
cti_prefix_release_(cti_prefix_t *prefix, const char *file, int line)
{
    RELEASE(prefix, cti_prefix);
}

static cti_status_t
cti_parse_prefixes_array(cti_prefix_vec_t **vec_ret, object_t prefixes_array)
{
    size_t prefixes_array_length = xpc_array_get_count(prefixes_array);
    size_t i, j;
    cti_prefix_vec_t *prefixes = cti_prefix_vec_create(prefixes_array_length);
    cti_status_t status = kCTIStatus_NoError;

    if (prefixes == NULL) {
        INFO("no memory.");
        status = kCTIStatus_NoMemory;
    } else {
        // Array of arrays
        for (i = 0; i < prefixes_array_length; i++) {
            object_t prefix_array = xpc_array_get_value(prefixes_array, i);
            int match_count = 0;
            bool matched_address = false;
            bool matched_metric = false;
            const char *destination = NULL;
            int metric = 0;
            struct in6_addr prefix_addr;

            if (prefix_array == NULL) {
                ERROR("Unable to get prefix array %zu", i);
            } else {
                size_t prefix_array_length = xpc_array_get_count(prefix_array);
                for (j = 0; j < prefix_array_length; j++) {
                    object_t *array_sub_dict = xpc_array_get_value(prefix_array, j);
                    if (array_sub_dict == NULL) {
                        ERROR("can't get prefix_array %zu subdictionary %zu", i, j);
                        goto done_with_prefix_array;
                    } else {
                        const char *key = xpc_dictionary_get_string(array_sub_dict, "key");
                        if (key == NULL) {
                            ERROR("Invalid prefixes array %zu subdictionary %zu: no key", i, j);
                            goto done_with_prefix_array;
                        }
                        // Fix me: when <rdar://problem/59371674> is fixed, remove Addreess key test.
                        else if (!strcmp(key, "Addreess") || !strcmp(key, "Address")) {
                            if (matched_address) {
                                ERROR("prefixes array %zu: Address appears twice.", i);
                                goto done_with_prefix_array;
                            }
                            destination = xpc_dictionary_get_string(array_sub_dict, "value");
                            if (destination == NULL) {
                                INFO("null address");
                                goto done_with_prefix_array;
                            }
                            matched_address = true;
                        } else if (!strcmp(key, "Metric")) {
                            if (matched_metric) {
                                ERROR("prefixes array %zu: Metric appears twice.", i);
                                goto done_with_prefix_array;
                            }
                            metric = (int)xpc_dictionary_get_uint64(array_sub_dict, "value");
                            matched_metric = true;
                        } else {
                            ERROR("Unknown key in prefix array %zu subdictionary %zu: " PUB_S_SRP, i, j, key);
                            goto done_with_prefix_array;
                        }
                        match_count++;
                    }
                }
                if (match_count != 2) {
                    ERROR("expecting %d sub-dictionaries to prefix array %zu, but got %d.",
                          2, i, match_count);
                    goto done_with_prefix_array;
                }

                // The prefix is in IPv6 address presentation form, so convert it to bits.
                char prefix_buffer[INET6_ADDRSTRLEN];
                const char *slash = strchr(destination, '/');
                size_t prefix_pres_len = slash - destination;
                if (prefix_pres_len >= INET6_ADDRSTRLEN - 1) {
                    ERROR("prefixes array %zu: destination is longer than maximum IPv6 address string: " PUB_S_SRP,
                          j, destination);
                    goto done_with_prefix_array;
                }
#ifndef __clang_analyzer__ // destination is never null at this point
                memcpy(prefix_buffer, destination, prefix_pres_len);
#endif
                prefix_buffer[prefix_pres_len] = 0;
                inet_pton(AF_INET6, prefix_buffer, &prefix_addr);

                // Also convert the prefix.
                char *endptr = NULL;
                int prefix_len = (int)strtol(slash + 1, &endptr, 10);
                if (endptr == slash + 1 || *endptr != 0 || prefix_len != 64) {
                    INFO("bogus prefix length provided by thread: " PUB_S_SRP, destination);
                    prefix_len = 64;
                }

                cti_prefix_t *prefix = cti_prefix_create(&prefix_addr, prefix_len, metric, 0, 0, false, false);
                if (prefix != NULL) {
                    prefixes->prefixes[i] = prefix;
                }
                continue;
            done_with_prefix_array:
                status = kCTIStatus_UnknownError;
            }
        }
    }
    if (status == kCTIStatus_NoError) {
        *vec_ret = prefixes;
    } else {
        if (prefixes != NULL) {
            RELEASE_HERE(prefixes, cti_prefix_vec);
        }
    }
    return status;
}

static void
cti_internal_prefix_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_prefix_reply_t callback = conn_ref->callback.prefix_reply;
    cti_status_t status = status_in;
    cti_prefix_vec_t *vec = NULL;
    object_t result_dictionary = NULL;
    if (status == kCTIStatus_NoError) {
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t *value = xpc_dictionary_get_array(result_dictionary, "value");
            if (value == NULL) {
                INFO("prefixes array not present in IPv6:Routes event.");
            } else {
                status = cti_parse_prefixes_array(&vec, value);
            }
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, vec, status);
    } else {
        INFO("[CX%d] Not calling callback.", conn_ref->serial);
    }
    if (vec != NULL) {
        RELEASE_HERE(vec, cti_prefix_vec);
    }
}

cti_status_t
cti_get_prefix_list_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
                     cti_prefix_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                     const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.prefix_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "IPv6:Routes");

    errx = setup_for_command(ref, client_queue, "get_prefix_list", "IPv6:Routes", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_prefix_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

static void
cti_route_finalize(cti_route_t *route)
{
    free(route);
}

static void
cti_route_vec_finalize(cti_route_vec_t *routes)
{
    size_t i;

    if (routes->routes != NULL) {
        for (i = 0; i < routes->num; i++) {
            if (routes->routes[i] != NULL) {
                RELEASE_HERE(routes->routes[i], cti_route);
            }
        }
        free(routes->routes);
    }
    free(routes);
}

cti_route_vec_t *
cti_route_vec_create_(size_t num_routes, const char *file, int line)
{
    cti_route_vec_t *routes = calloc(1, sizeof(*routes));
    if (routes != NULL) {
        if (num_routes != 0) {
            routes->routes = calloc(num_routes, sizeof(cti_route_t *));
            if (routes->routes == NULL) {
                free(routes);
                return NULL;
            }
        }
        routes->num = num_routes;
        RETAIN(routes, cti_route_vec);
    }
    return routes;
}

void
cti_route_vec_release_(cti_route_vec_t *routes, const char *file, int line)
{
    RELEASE(routes, cti_route_vec);
}

cti_route_t *
cti_route_create_(struct in6_addr *prefix, int prefix_length, offmesh_route_origin_t origin,
                  bool nat64, bool stable, offmesh_route_preference_t preference, int rloc,
                  bool next_hop_is_host, const char *file, int line)
{
    cti_route_t *route_ret = calloc(1, sizeof(*route_ret));
    if (prefix != NULL) {
        route_ret->prefix = *prefix;
        route_ret->prefix_length = prefix_length;
        route_ret->origin = origin;
        route_ret->nat64 = nat64;
        route_ret->stable = stable;
        route_ret->preference = preference;
        route_ret->rloc = rloc;
        route_ret->next_hop_is_host = next_hop_is_host;
        RETAIN(route_ret, cti_route);
    }
    return route_ret;
}

static cti_status_t
cti_parse_offmesh_routes_array(cti_route_vec_t **vec_ret, object_t routes_array)
{
    size_t i, j, routes_array_length = xpc_array_get_count(routes_array);
    cti_route_vec_t *routes = cti_route_vec_create(routes_array_length);
    cti_status_t status = kCTIStatus_NoError;

    if (routes == NULL) {
        INFO("no memory.");
        status = kCTIStatus_NoMemory;
    } else {
        // Array of arrays
        for (i = 0; i < routes_array_length; i++) {
            object_t route_array = xpc_array_get_value(routes_array, i);
            if (route_array == NULL) {
                ERROR("Unable to get route array %zu", i);
            } else {
                bool nat64 = false, stable = false, next_hop_is_host = false;
                int rloc = 0, prefix_len = 0;
                offmesh_route_preference_t pref = offmesh_route_preference_low;
                offmesh_route_origin_t origin = offmesh_route_origin_user;
                struct in6_addr prefix_addr = { };
                size_t route_array_length = xpc_array_get_count(route_array);
                for (j = 0; j < route_array_length; j++) {
                    object_t *array_sub_dict = xpc_array_get_value(route_array, j);
                    if (array_sub_dict == NULL) {
                        ERROR("can't get routes_array %zu subdictionary %zu", i, j);
                        goto done_with_route_array;
                    }
                    const char *key = xpc_dictionary_get_string(array_sub_dict, "key");
                    if (key == NULL) {
                        ERROR("Invalid routes array %zu subdictionary %zu: no key", i, j);
                        goto done_with_route_array;
                    } else if (!strcmp(key, "nat64")) {
                        if (xpc_dictionary_get_uint64(array_sub_dict, "value") == 1) {
                            nat64 = true;
                        }
                    } else if (!strcmp(key, "origin")) {
                        const char *origin_str = xpc_dictionary_get_string(array_sub_dict, "value");
                        if (origin_str == NULL) {
                            ERROR("Invalid routes array %zu subdictionary %zu: NULL origin", i, j);
                            goto done_with_route_array;
                        }
                        if (!strcmp(origin_str, "ncp")) {
                            origin = offmesh_route_origin_ncp;
                        }
                    } else if (!strcmp(key, "stable")) {
                        if (xpc_dictionary_get_uint64(array_sub_dict, "value") == 1) {
                            stable = true;
                        }
                    } else if (!strcmp(key, "nextHopIsHost")) {
                        if (xpc_dictionary_get_bool(array_sub_dict, "value") == true) {
                            next_hop_is_host = true;
                        }
                    } else if (!strcmp(key, "rloc")) {
                        rloc = (int)xpc_dictionary_get_uint64(array_sub_dict, "value");
                    } else if (!strcmp(key, "preference")) {
                        const char *pref_str = xpc_dictionary_get_string(array_sub_dict, "value");
                        if (pref_str == NULL) {
                            ERROR("Invalid routes array %zu subdictionary %zu: NULL preference", i, j);
                            goto done_with_route_array;
                        }
                        if (!strcmp(pref_str, "high")) {
                            pref = offmesh_route_preference_high;
                        } else if (!strcmp(pref_str, "medium")) {
                            pref = offmesh_route_preference_medium;
                        }
                    } else if (!strcmp(key, "address")) {
                        const char *addr_str = xpc_dictionary_get_string(array_sub_dict, "value");
                        if (addr_str == NULL) {
                            ERROR("Invalid routes array %zu subdictionary %zu: NULL address", i, j);
                            goto done_with_route_array;
                        }
                        // The prefix is in IPv6 address presentation form, so convert it to bits.
                        char prefix_buffer[INET6_ADDRSTRLEN];
                        const char *slash = strchr(addr_str, '/');
                        if (slash == NULL) {
                            ERROR("bad address " PRI_S_SRP, addr_str);
                            goto done_with_route_array;
                        }
                        size_t prefix_pres_len = slash - addr_str;
                        if (prefix_pres_len > INET6_ADDRSTRLEN - 1) {
                            ERROR("routes array %zu: destination is longer than maximum IPv6 address string: " PRI_S_SRP,
                                 i, addr_str);
                                goto done_with_route_array;
                        }
#ifndef __clang_analyzer__ // destination is never null at this point
                        memcpy(prefix_buffer, addr_str, prefix_pres_len);
#endif
                        prefix_buffer[prefix_pres_len] = 0;
                        if (inet_pton(AF_INET6, prefix_buffer, &prefix_addr) != 1) {
                            ERROR("invalid ipv6 address " PRI_S_SRP, prefix_buffer);
                            goto done_with_route_array;
                        }
                        // Convert the prefix.
                        char *endptr = NULL;
                        long tmp = strtol(slash + 1, &endptr, 10);
                        if (endptr == slash + 1 || *endptr != 0 || tmp < 0 || tmp > 128) {
                            ERROR("bogus ipv6 prefix length provided by thread: " PRI_S_SRP, addr_str);
                            goto done_with_route_array;
                        } else {
                            prefix_len = (int)tmp;
                        }
                    } else {
                        ERROR("Invalid routes array %zu subdictionary %zu: unknown key " PUB_S_SRP, i, j, key);
                        goto done_with_route_array;
                    }
                }
                cti_route_t *r = cti_route_create(&prefix_addr, prefix_len, origin, nat64, stable, pref, rloc, next_hop_is_host);
                if (r == NULL) {
                    ERROR("No memory when create route.");
                    goto done_with_route_array;
                } else {
                    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix_addr.s6_addr, prefix_buf);
                    INFO("Got offmesh route " PRI_SEGMENTED_IPv6_ADDR_SRP " len %d origin:" PUB_S_SRP " nat64:"
                         PUB_S_SRP " stable:" PUB_S_SRP " preference:%d rloc:0x%04x next_hop_is_host:" PUB_S_SRP,
                         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix_addr.s6_addr, prefix_buf), prefix_len,
                         origin == offmesh_route_origin_user ? "user":"ncp", nat64 ? "yes" : "no",
                         stable ? "yes" : "no", pref, rloc, next_hop_is_host ? "yes" : "no");
                         routes->routes[i] = r;
                }
                continue;
            done_with_route_array:
                status = kCTIStatus_UnknownError;
                break;
            }
        }
    }
    if (status == kCTIStatus_NoError) {
        *vec_ret = routes;
    } else {
        if (routes != NULL) {
            RELEASE_HERE(routes, cti_route_vec);
        }
    }
    return status;
}

static void
cti_internal_offmesh_route_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_offmesh_route_reply_t callback = conn_ref->callback.offmesh_route_reply;
    cti_status_t status = status_in;
    cti_route_vec_t *vec = NULL;
    object_t result_dictionary = NULL;
    if (status == kCTIStatus_NoError) {
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t *value = xpc_dictionary_get_array(result_dictionary, "value");
            if (value == NULL) {
                INFO("offmesh route array not present in Thread:OffMeshroutes event.");
            } else {
                status = cti_parse_offmesh_routes_array(&vec, value);
            }
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, vec, status);
    } else {
        INFO("[CX%d] Not calling callback for %p", conn_ref->serial, conn_ref);
    }
    if (vec != NULL) {
        RELEASE_HERE(vec, cti_route_vec);
    }
}

cti_status_t
cti_get_offmesh_route_list_(srp_server_t *UNUSED server, cti_connection_t *ref,
                            void *NULLABLE context, cti_offmesh_route_reply_t NONNULL callback,
                            run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.offmesh_route_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "Thread:OffMeshroutes");

    errx = setup_for_command(ref, client_queue, "get_offmesh_route_list", "Thread:OffMeshroutes", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_offmesh_route_reply_callback, false, file, line);
    INFO("get_offmesh_route_list result %d", errx);
    xpc_release(dict);

    return errx;
}

static cti_status_t
cti_parse_onmesh_prefix_array(cti_prefix_vec_t **vec_ret, object_t prefix_array)
{
    size_t i, prefix_array_length = xpc_array_get_count(prefix_array);
    cti_prefix_vec_t *prefixes = cti_prefix_vec_create(prefix_array_length);
    cti_status_t status = kCTIStatus_NoError;

    if (prefixes == NULL) {
        INFO("no memory.");
        status = kCTIStatus_NoMemory;
    } else {
        for (i = 0; i < prefix_array_length; i++) {
            const char *route = xpc_array_get_string(prefix_array, i);
            bool stable = false;
            int rloc = 0, prefix_len = 0, flags = 0;
            bool ncp = false;
            struct in6_addr prefix_addr = { };

            char *dup = strdup(route);
            if (dup == NULL) {
                INFO("no memory when strdup");
                goto done_with_prefix_array;
            }
            char *token, *remain = dup;
            int index = 0;
            while ((token = strtok_r(remain, " ", &remain))) {
                if (index == 0) {
                    if (!inet_pton(AF_INET6, token, &prefix_addr)) {
                        ERROR("invalid ipv6 address " PUB_S_SRP, token);
                        goto done_with_prefix_array;
                    }
                } else if (index == 1) {
                    if (strncmp(token, "prefix_len:", 11)) {
                        ERROR("expecting prefix_len rather than " PRI_S_SRP " at %d", token, index);
                        goto done_with_prefix_array;
                    }
                    char *endptr = NULL;
                    prefix_len = (int)strtol(token + 11, &endptr, 10);
                } else if (index == 2) {
                    if (strncmp(token, "origin:", 7)) {
                        ERROR("expecting origin rather than " PRI_S_SRP " at %d", token, index);
                        goto done_with_prefix_array;
                    }
                    if (!strcmp(token + 7, "ncp")){
                        ncp = true;
                    } else if (strcmp(token + 7, "user")) {
                        ERROR("unexpected origin: " PRI_S_SRP, token + 7);
                        goto done_with_prefix_array;
                    }
                } else if (index == 3) {
                    if (strncmp(token, "stable:", 7)) {
                        ERROR("expecting table rather than " PRI_S_SRP " at %d", token, index);
                        goto done_with_prefix_array;
                    }
                    if (!strcmp(token + 7, "yes")){
                        stable = true;
                    } else if (strcmp(token + 7, "no")) {
                        ERROR("unexpected boolean state: " PRI_S_SRP, token + 7);
                        goto done_with_prefix_array;
                    }
                } else if (index == 4) {
                    if (strncmp(token, "flags:", 6)) {
                        ERROR("expecting flags rather than " PRI_S_SRP " at %d", token, index);
                        goto done_with_prefix_array;
                    }
                    char *endptr = NULL;
                    flags = ntohs((int)strtol(token + 6, &endptr, 16));
                } else if (index == 14) {
                    if (strncmp(token, "rloc:", 5)) {
                        ERROR("expecting rloc rather than " PRI_S_SRP " at %d", token, index);
                        goto done_with_prefix_array;
                    }
                    char *endptr = NULL;
                    rloc = (int)strtol(token + 5, &endptr, 16);
                } else {
                    // Anything else is in the parsed flags, which we don't care about, or is a bogon, which will probably result
                    // in a failure elsewhere. We're not really expecting bogons, so putting in extra code here to flag them would
                    // be useless.
                }
                index++;
            }
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix_addr.s6_addr, prefix_buf);
            INFO("got prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " len %d origin:" PUB_S_SRP " stable:" PUB_S_SRP " flags:%x rloc:%04x",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix_addr.s6_addr, prefix_buf), prefix_len,
                 ncp ? "ncp" : "user", stable ? "yes" : "no", flags, rloc);
            cti_prefix_t *r = cti_prefix_create(&prefix_addr, prefix_len, 0, flags, rloc, stable, ncp);
            if (r) {
                prefixes->prefixes[i] = r;
            } else {
                ERROR("no memory for parsed prefix!");
                goto done_with_prefix_array;
            }
            free(dup);
            continue;
        done_with_prefix_array:
            status = kCTIStatus_UnknownError;
            free(dup);
            break;
        }
    }
    if (status == kCTIStatus_NoError) {
        *vec_ret = prefixes;
    } else {
        if (prefixes != NULL) {
            RELEASE_HERE(prefixes, cti_prefix_vec);
        }
    }
    return status;
}

static void
cti_internal_onmesh_prefix_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_onmesh_prefix_reply_t callback = conn_ref->callback.onmesh_prefix_reply;
    cti_status_t status = status_in;
    cti_prefix_vec_t *vec = NULL;
    object_t result_dictionary = NULL;
    if (status == kCTIStatus_NoError) {
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t *value = xpc_dictionary_get_array(result_dictionary, "value");
            if (value == NULL) {
                INFO("onmesh prefix array not present in Thread:OnMeshPrefixes event.");
            } else {
                status = cti_parse_onmesh_prefix_array(&vec, value);
            }
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, vec, status);
    } else {
        INFO("[CX%d] not calling callback for %p", conn_ref->serial, conn_ref);
    }
    if (vec != NULL) {
        RELEASE_HERE(vec, cti_prefix_vec);
    }
}

cti_status_t
cti_get_onmesh_prefix_list_(srp_server_t *UNUSED server, cti_connection_t NULLABLE *NULLABLE ref,
                            void *NULLABLE context, cti_onmesh_prefix_reply_t NONNULL callback,
                            run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.onmesh_prefix_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "Thread:OnMeshPrefixes");

    errx = setup_for_command(ref, client_queue, "get_onmesh_prefix_list", "Thread:OnMeshPrefixes", NULL, dict,
                             "WpanctlCmd", context, app_callback, cti_internal_onmesh_prefix_reply_callback, false,
                             file, line);
    INFO("result %d", errx);
    xpc_release(dict);

    return errx;
}

static void
cti_internal_rloc16_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_rloc16_reply_t callback = conn_ref->callback.rloc16_reply;
    cti_status_t status = status_in;
    object_t result_dictionary = NULL;
    uint16_t rloc16 = 0;
    if (status == kCTIStatus_NoError) {
        status = cti_event_or_response_extract(reply, &result_dictionary);
        // The reply format is
        // commandData:  {response: "> e000
        // Done", method: "OtCtlCmd"}
        // where e000 is the rloc16 that we want to extract
        if (status == kCTIStatus_NoError) {
            const char *rloc16_str = xpc_dictionary_get_string(result_dictionary, "response");
            char *endptr = "\n";
            rloc16 = (uint16_t)strtol(&rloc16_str[2], &endptr, 16);
        }
    }
    if (callback != NULL) {
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, rloc16, status);
    } else {
        INFO("[CX%d] not calling callback for %p", conn_ref->serial, conn_ref);
    }
}

cti_status_t
cti_get_rloc16_(srp_server_t *UNUSED server, cti_connection_t *ref,
                void *NULLABLE context, cti_rloc16_reply_t NONNULL callback,
                run_context_t NULLABLE client_queue, const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.rloc16_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "OtCtlCmd");
    xpc_dictionary_set_string(dict, "otctl_cmd", "rloc16");

    errx = setup_for_command(ref, client_queue, "get_rloc16", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_rloc16_reply_callback, false, file, line);
    INFO("get_rloc16 result %d", errx);
    xpc_release(dict);

    return errx;
}

static void
cti_internal_wed_reply_callback(cti_connection_t NONNULL conn_ref, object_t reply, cti_status_t status_in)
{
    cti_wed_reply_t callback = conn_ref->callback.wed_reply;
    cti_service_vec_t *vec = NULL;
    cti_status_t status = status_in;

    const char *extended_mac = NULL;
    const char *ml_eid = NULL;
    bool added = false;

    if (status == kCTIStatus_NoError) {
        object_t result_dictionary = NULL;
        status = cti_event_or_response_extract(reply, &result_dictionary);
        if (status == kCTIStatus_NoError) {
            object_t *value_array = xpc_dictionary_get_array(result_dictionary, "value");
            if (value_array == NULL) {
                INFO("[CX%d] wed status array not present in wed status event.", conn_ref->serial);
                goto out;
            } else {
                size_t value_array_length = xpc_array_get_count(value_array);
                for (size_t i = 0; i < value_array_length; i++) {
                    object_t *elt = xpc_array_get_value(value_array, i);
                    xpc_type_t type = xpc_get_type(elt);
                    if (type != XPC_TYPE_DICTIONARY) {
                        ERROR("non-dictionary element of value array");
                        goto out;
                    }
                    const char *key = xpc_dictionary_get_string(elt, "key");
                    if (key == NULL) {
                        ERROR("no key in value array");
                        goto out;
                    }
                    const char *value = xpc_dictionary_get_string(elt, "value");
                    if (!strcmp(key, "extendedMACAddress")) {
                        extended_mac = value;
                    } else if (!strcmp(key, "mleid")) {
                        ml_eid = value;
                    } else if (!strcmp(key, "status")) {
                        if (!strcmp(value, "wed_added")) {
                            added = true;
                        } else if (!strcmp(value, "wed_removed")) {
                            added = false;
                        } else {
                            ERROR("unknown wed status " PUB_S_SRP, value);
                            goto out;
                        }
                    } else {
                        if (value == NULL) {
                            INFO("unknown key in response: " PUB_S_SRP, key);
                        } else {
                            INFO("unknown key " PUB_S_SRP " with value " PRI_S_SRP, key, value);
                        }
                    }
                }
            }
        }
    }
out:
    if (callback != NULL) {
        if (added && (ml_eid == NULL || extended_mac == NULL)) {
            added = false;
        }
        INFO("[CX%d] calling callback for %p", conn_ref->serial, conn_ref);
        callback(conn_ref->context, extended_mac, ml_eid, added, status);
    }
    if (vec != NULL) {
        RELEASE_HERE(vec, cti_service_vec);
    }
}

cti_status_t
cti_track_wed_status_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
                      cti_wed_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                      const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.wed_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "WakeOnDeviceConnectionStatus");

    errx = setup_for_command(ref, client_queue, "get_wed_status", "WakeOnDeviceConnectionStatus", NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_wed_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_track_neighbor_ml_eid_(srp_server_t *UNUSED server, cti_connection_t *ref, void *NULLABLE context,
                           cti_string_property_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                           const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.string_property_reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "PropGet");
    xpc_dictionary_set_string(dict, "property_name", "ThreadNeighborMeshLocalAddress");

    errx = setup_for_command(ref, client_queue, "get_neighbor_ml_eid", "ThreadNeighborMeshLocalAddress", "ThreadNeighborMeshLocalAddress", dict, "WpanctlCmd",
                             context, app_callback, cti_internal_string_event_reply, false, file, line);
    xpc_release(dict);

    return errx;
}

cti_status_t
cti_add_ml_eid_mapping_(srp_server_t *UNUSED server, void *NULLABLE context,
                        cti_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                        struct in6_addr *omr_addr, struct in6_addr *ml_eid, const char *hostname,
                        const char *file, int line)
{
    cti_callback_t app_callback;
    app_callback.reply = callback;
    cti_status_t errx;
    object_t dict = xpc_dictionary_create(NULL, NULL, 0);
    char addrbuf[INET6_ADDRSTRLEN];

    xpc_dictionary_set_string(dict, "interface", "org.wpantund.v1");
    xpc_dictionary_set_string(dict, "path", "/org/wpantund/utun2");
    xpc_dictionary_set_string(dict, "method", "UpdateAccessoryData");
    inet_ntop(AF_INET6, omr_addr, addrbuf, sizeof(addrbuf));
    xpc_dictionary_set_string(dict, "ipaddr_add", addrbuf);
    inet_ntop(AF_INET6, ml_eid, addrbuf, sizeof(addrbuf));
    xpc_dictionary_set_string(dict, "ipaddr_lookup", addrbuf);
    xpc_dictionary_set_string(dict, "host_info", hostname);

    errx = setup_for_command(NULL, client_queue, "add_mle_mapping", NULL, NULL, dict, "WpanctlCmd",
                             context, app_callback, cti_internal_reply_callback, false, file, line);
    xpc_release(dict);

    return errx;
}


cti_status_t
cti_events_discontinue(cti_connection_t ref)
{
    if (ref->connection != NULL) {
        INFO("[CX%d] canceling connection %p", ref->serial, ref->connection);
        xpc_connection_cancel(ref->connection);
    }
    // This is releasing the caller's reference.
    cti_connection_release(ref);
    return kCTIStatus_NoError;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
