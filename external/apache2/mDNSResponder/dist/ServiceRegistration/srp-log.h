/* srp-log.h
 *
 * Copyright (c) 2020-2021 Apple Inc. All rights reserved.
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
 * This file contains the utilities that are used to print log that
 * redacts private information about the user, and function prototypes that
 * helps to create better logs.
 */


#ifndef __SRP_LOG_H__
#define __SRP_LOG_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef POSIX_BUILD
#include <limits.h>
#include <sys/param.h>
#endif

#ifdef __APPLE__
#include <os/log.h> // For os_log related APIs
#endif // #ifdef __APPLE__

#ifdef DEBUG
#    undef DEBUG
    // #define DEBUG_VERBOSE
#endif

// We always want this until we start shipping
#define DEBUG_VERBOSE

//======================================================================================================================
// MARK: - Log Macros

#ifdef FUZZING
#    define OPENLOG(progname, consolep)
#    define ERROR(fmt, ...)
#    define INFO(fmt, ...)
#    define DEBUG(fmt, ...)
#    define FAULT(fmt, ...)
#elif defined(THREAD_DEVKIT_ADK)

#    include "srp-platform.h"

#    define OPENLOG(progname, consolep) srp_openlog(option)
#    define ERROR(fmt, ...)   srp_log_error(fmt, ##__VA_ARGS__)
#    define INFO(fmt, ...)    srp_log_info(fmt, ##__VA_ARGS__)
#    ifdef DEBUG_VERBOSE
#        define DEBUG(fmt, ...) srp_log_debug(fmt, ##__VA_ARGS__)
#    else
#        define DEBUG(fmt, ...)
#    endif // DEBUG VERBOSE
#    define FAULT(fmt, ...) srp_log_error(fmt, ##__VA_ARGS__)
#    define NO_CLOCK
#else // ifdef THREAD_DEVKIT_ADK

#    ifdef LOG_FPRINTF_STDERR
void srp_log_timestamp(char *buf, size_t bufsize);
extern bool srp_log_timestamp_relative;
#        define OPENLOG(progname, consolep) do { (void)(consolep); (void)progname; } while (0)
#        define SRP_LOG_TIME char srp_log_time_buf[32]; srp_log_timestamp(srp_log_time_buf, sizeof(srp_log_time_buf))
#        define ERROR(fmt, ...) do { SRP_LOG_TIME; fprintf(stderr, "%s %s: " fmt "\n", srp_log_time_buf, __FUNCTION__, ##__VA_ARGS__); } while (0)
#        define INFO(fmt, ...)  do { SRP_LOG_TIME; fprintf(stderr, "%s %s: " fmt "\n", srp_log_time_buf, __FUNCTION__, ##__VA_ARGS__); } while (0)
#        ifdef DEBUG_VERBOSE
#            ifdef IOLOOP_MACOS
                int get_num_fds(void);
#            endif // ifdef IOLOOP_MACOS
#            define DEBUG(fmt, ...)  do { SRP_LOG_TIME; fprintf(stderr, "%s %s: " fmt "\n", srp_log_time_buf, __FUNCTION__, ##__VA_ARGS__); } while (0)
#        else // ifdef DEBUG_VERBOSE
#            define DEBUG(fmt, ...)
#        endif
#        define FAULT(fmt, ...) do { SRP_LOG_TIME; fprintf(stderr, "%s %s: " fmt "\n", srp_log_time_buf, __FUNCTION__, ##__VA_ARGS__); } while (0)
#    else // ifdef LOG_FPRINTF_STDERR
#        include <syslog.h>

        // Apple device always has OS_LOG support.
#        ifdef __APPLE__
#            define OS_LOG_ENABLED 1
#            include <os/log.h>
extern os_log_t global_os_log;


            // Define log level
#            define LOG_TYPE_FAULT      OS_LOG_TYPE_FAULT
#            define LOG_TYPE_ERROR      OS_LOG_TYPE_ERROR
#            define LOG_TYPE_INFO       OS_LOG_TYPE_DEFAULT
#            define LOG_TYPE_DEBUG      OS_LOG_TYPE_DEBUG
            // Define log macro
#            define SRP_OS_LOG(component, type, format, ...) \
                os_log_with_type((component), (type), ("%{public}s: " format), __FUNCTION__, ##__VA_ARGS__)

#            define OPENLOG(progname, consolep) \
                do { \
                    if (consolep) {                                         \
                        putenv("ACTIVITY_LOG_STDERR=1"); \
                    } \
                    (void)progname; \
                    global_os_log = os_log_create("com.apple.srp-mdns-proxy", "0"); \
                } while (0)
#            define FAULT(format, ...)  SRP_OS_LOG(global_os_log, LOG_TYPE_FAULT, format, ##__VA_ARGS__)
#            define ERROR(format, ...)  SRP_OS_LOG(global_os_log, LOG_TYPE_ERROR, format, ##__VA_ARGS__)

#            ifdef DEBUG_VERBOSE
#                ifdef DEBUG_FD_LEAKS
                    int get_num_fds(void);
#                    define INFO(format, ...) \
                        do { \
                            int foo = get_num_fds(); \
                            SRP_OS_LOG(global_os_log, LOG_TYPE_INFO, "%d " format, foo, ##__VA_ARGS__); \
                        } while(0)
#                else // ifdef IOLOOP_MACOS
#                    define INFO(format, ...) SRP_OS_LOG(global_os_log, LOG_TYPE_INFO, format, ##__VA_ARGS__)
#                endif // ifdef IOLOOP_MACOS

#                define DEBUG(format, ...) SRP_OS_LOG(global_os_log, LOG_TYPE_DEBUG, format, ##__VA_ARGS__)
#            else // ifdef DEBUG_VERBOSE
#                define INFO(format, ...) SRP_OS_LOG(global_os_log, LOG_TYPE_INFO, format, ##__VA_ARGS__)
#                define DEBUG(format, ...)  do {} while(0)
#            endif // ifdef DEBUG_VERBOSE
#        else // ifdef __APPLE__
#            define OS_LOG_ENABLED 0

#            define OPENLOG(progname, consolep) openlog(progname, (consolep ? LOG_PERROR : 0) | LOG_PID, LOG_DAEMON)
#            define FAULT(fmt, ...) syslog(LOG_CRIT, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#            define ERROR(fmt, ...) syslog(LOG_ERR, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)

#            ifdef DEBUG_VERBOSE
#                ifdef DEBUG_FD_LEAKS
                    int get_num_fds(void);
#                    define INFO(fmt, ...) \
                        do { \
                            int foo = get_num_fds(); \
                            syslog(LOG_INFO, "%s: %d " fmt, __FUNCTION__, foo, ##__VA_ARGS__); \
                        } while (0)
#                else // ifdef IOLOOP_MACOS
#                    define INFO(fmt, ...)  syslog(LOG_INFO, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#                endif // ifdef IOLOOP_MACOS
#                define DEBUG(fmt, ...) syslog(LOG_DEBUG, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#            else // ifdef DEBUG_VERBOSE
#                define INFO(fmt, ...)  syslog(LOG_INFO, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#                define DEBUG(fmt, ...) do {} while(0)
#            endif // ifdef DEBUG_VERBOSE
#        endif // ifdef __APPLE__
#    endif // ifdef LOG_FPRINTF_STDERR
#endif // ifdef THREAD_DEVKIT_ADK

//======================================================================================================================
// MARK: - Log Specifiers
/**
 * With the logging routines defined above, the logging macros are defined to facilitate the log redaction enforced by
 * os_log on Apple platforms. By using the specifier "%{mask.hash}", the "<private>" text in the logs of customer device
 * would be shown as a hashing value, which could be used as a way to associate other SRP logs even if it's redacted.
 *
 * On Apple platforms, the current existing log routines will be defined as:
 * #define log_with_component_and_type(CATEGORY, LEVEL, format, ...) os_log_with_type((CATEGORY), (LEVEL), (format), \
 *                                                                      ##__VA_ARGS__)
 * #define ERROR(format, ...)  log_with_component_and_type(OS_LOG_DEFAULT, LOG_TYPE_ERROR, format, ##__VA_ARGS__)
 * #define INFO(format, ...)   log_with_component_and_type(OS_LOG_DEFAULT, LOG_TYPE_DEFAULT, format, ##__VA_ARGS__)
 * #define DEBUG(format, ...)  log_with_component_and_type(OS_LOG_DEFAULT, LOG_TYPE_DEBUG, format, ##__VA_ARGS__)
 * And to follow the same log level with os_log, FAULT() is defined.
 * #define FAULT(format, ...)  log_with_component_and_type(OS_LOG_DEFAULT, LOG_TYPE_FAULT, format, ##__VA_ARGS__)
 * Therefore, all the previous logs would be put under OS_LOG_DEFAULT category.
 * FAULT level lof will be mapped to  LOG_TYPE_FAULT in os_log.
 * ERROR level log will be mapped to LOG_TYPE_ERROR in os_log.
 * INFO level log will be mapped to LOG_TYPE_DEFAULT in os_log.
 * DEBUG level log woll be mapped to LOG_TYPE_DEBUG in os_log.
 *
 * On platforms other than Apple, syslog will be used to write logs.
 * FAULT level lof will be mapped to  LOG_CRIT in syslog.
 * ERROR level log will be mapped to LOG_ERR in syslog.
 * INFO level log will be mapped to LOG_INFO in syslog.
 * DEBUG level log woll be mapped to LOG_DEBUG in syslog.
 *
 * The defined specifiers are:
 * String specifier:
 *      PUB_S_SRP: Use this in the format string when trying to log string and do not want it to be redacted.
 *      PRI_S_SRP: Use this when trying to log string and redact it to a hash string.
 *      Usage:
 *          INFO("Public string: " PUB_S_SRP, ", private string: " PRI_S_SRP, string_ptr, string_ptr);
 *
 * DNS name (with dns_label_t type) specifier:
 *      DNS_NAME_GEN_SRP: Always call this before logging DNS name.
 *      PUB_DNS_NAME_SRP: Use this in the format string when trying to log DNS name and do not want it to be redacted.
 *      PRI_DNS_NAME_SRP: Use this in the format string when trying to log DNS name and redact it to a hash string.
 *      DNS_NAME_PARAM_SRP: Always use DNS_NAME_PARAM_SRP in the paramter list.
 *      Usage:
 *          DNS_NAME_GEN_SRP(dns_name_ptr, dns_name_buf);
 *          INFO("Public DNS name: " PUB_DNS_NAME_SRP, ", private DNS name: " PRI_DNS_NAME_SRP,
 *              DNS_NAME_PARAM_SRP(dns_name_ptr, dns_name_buf), DNS_NAME_PARAM_SRP(dns_name_ptr, dns_name_buf));
 *
 * IPv4 address specifier (with in_addr * type or a pointer to uint8_t[4]):
 *      IPv4_ADDR_GEN_SRP: Always call this before logging IPv4 address.
 *      PUB_IPv4_ADDR_SRP: Use this in the format string when trying to log IPv4 address and do not want it to be
 *                         redacted.
 *      PRI_IPv4_ADDR_SRP: Use this in the format string when trying to log IPv4 address and redact it to a hash string.
 *      IPv4_ADDR_PARAM_SRP: Always use IPv4_ADDR_PARAM_SRP in the paramter list.
 *      Usage:
 *          IPv4_ADDR_GEN_SRP(in_addr_ptr_1, in_addr_buf_1);
 *          IPv4_ADDR_GEN_SRP(in_addr_ptr_2, in_addr_buf_2);
 *          INFO("Public IPv4 address: " PUB_IPv4_ADDR_SRP, ", private IPv4 address: " PRI_IPv4_ADDR_SRP,
 *              IPv4_ADDR_PARAM_SRP(in_addr_ptr_1, in_addr_buf_1), IPv4_ADDR_PARAM_SRP(in_addr_ptr_2, in_addr_buf_2));
 *
 * IPv6 address specifier (with in6_addr * type or a pointer to uint8_t[16]):
 *      IPv6_ADDR_GEN_SRP: Always call this before logging IPv6 address.
 *      PUB_IPv6_ADDR_SRP: Use this in the format string when trying to log IPv6 address and do not want it to be
 *                         redacted.
 *      PRI_IPv6_ADDR_SRP: Use this in the format string when trying to log IPv6 address and redact it to a hash string.
 *      IPv6_ADDR_PARAM_SRP: Always use IPv6_ADDR_PARAM_SRP in the paramter list.
 *      Usage:
 *          IPv6_ADDR_GEN_SRP(in6_addr_ptr_1, in6_addr_buf_1);
 *          IPv6_ADDR_GEN_SRP(in6_addr_ptr_2, in6_addr_buf_2);
 *          INFO("Public IPv6 address: " PUB_IPv6_ADDR_SRP, ", private IPv6 address: " PRI_IPv6_ADDR_SRP,
 *              IPv6_ADDR_PARAM_SRP(in6_addr_ptr_1, in6_addr_buf_1),
 *              IPv6_ADDR_PARAM_SRP(in6_addr_ptr_2, in6_addr_buf_2));
 *
 * Segmented IPv6  address specifier (with in6_addr * type or a pointer to uint8_t[16]):
 *      SEGMENTED_IPv6_ADDR_GEN_SRP: Always call this before logging segmented IPv6 address.
 *      PUB_SEGMENTED_IPv6_ADDR_SRP: Use this in the format string when trying to log segmented IPv6 address and do not
 *                                   want it to be redacted.
 *      PRI_SEGMENTED_IPv6_ADDR_SRP: Use this in the format string when trying to log segmented IPv6 address and redact
 *                                   it to a hash string.
 *      SEGMENTED_IPv6_ADDR_PARAM_SRP: Always use SEGMENTED_IPv6_ADDR_PARAM_SRP in the paramter list.
 *      Usage:
 *          SEGMENTED_IPv6_ADDR_GEN_SRP(in6_addr_ptr_1, in6_addr_buf_1);
 *          SEGMENTED_IPv6_ADDR_GEN_SRP(in6_addr_ptr_2, in6_addr_buf_2);
 *          INFO("Public IPv6 address: " PUB_SEGMENTED_IPv6_ADDR_SRP, ", private IPv6 address: "
 *               PRI_SEGMENTED_IPv6_ADDR_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(in6_addr_ptr_1, in6_addr_buf_1),
 *               SEGMENTED_IPv6_ADDR_PARAM_SRP(in6_addr_ptr_2, in6_addr_buf_2));
 *      Note:
 *          Segmented IPv6 is prefered when logging IPv6 address in SRP, because the address is divided to: 48 bit
 *          prefix, 16 bit subnet, 64 bit host, which makes it easier to match the prefix even when log redaction is
 *          turned on and the address is hashed to a string.
 *
 * IPv6 prefix specifier (with a pointer to uint8_t[] array):
 *      IPv6_PREFIX_GEN_SRP: Always call this before logging IPv6 prefix (which is also segmented).
 *      PUB_IPv6_PREFIX_SRP: Use this in the format string when trying to log IPv6 prefix and do not want it to be
 *                           redacted.
 *      PRI_IPv6_PREFIX_SRP: Use this in the format string when trying to log IPv6 prefix and redact it to a hash
 *                           string.
 *      IPv6_PREFIX_PARAM_SRP: Always use IPv6_PREFIX_PARAM_SRP in the paramter list.
 *      Usage:
 *          IPv6_PREFIX_GEN_SRP(in6_prefix_ptr_1, sizeof(in6_prefix_1), in6_prefix_buf_1);
 *          IPv6_PREFIX_GEN_SRP(in6_prefix_ptr_2, sizeof(in6_prefix_2), in6_prefix_buf_2);
 *          INFO("Public IPv6 prefix: " PUB_IPv6_PREFIX_SRP, ", private IPv6 prefix: " PRI_IPv6_PREFIX_SRP,
 *              IPv6_PREFIX_PARAM_SRP(in6_prefix_buf_1), IPv6_PREFIX_PARAM_SRP(in6_prefix_buf_2));
 *
 * Mac address specifier (with a pointer to uint8_t[6] array):
 *      PUB_MAC_ADDR_SRP: Use this in the format string when trying to log Mac address and do not want it to be
 *                        redacted.
 *      PRI_MAC_ADDR_SRP: Use this in the format string when trying to log Mac address and redact it to a hash string.
 *      MAC_ADDR_PARAM_SRP: Always use MAC_ADDR_PARAM_SRP in the paramter list.
 *      Usage:
 *          INFO("Public MAC address: " PUB_MAC_ADDR_SRP, ", private MAC address: " PRI_MAC_ADDR_SRP,
 *              MAC_ADDR_PARAM_SRP(mac_addr), MAC_ADDR_PARAM_SRP(mac_addr));
 */

// Helper macro to display if the correspoding IPv6 is ULA (Unique Local Address), LUA (Link Local Address)
// or GUA (Global Unicast Address).
// ULA starts with FC00::/7.
// LUA starts with fe80::/10.
// GUA starts with 2000::/3.
#define IS_IPV6_ADDR_ULA(ADDR) ( ((ADDR)[0] & 0xFE) == 0xFC )
#define IS_IPV6_ADDR_LUA(ADDR) ( ((ADDR)[0] == 0xFE) && ((uint8_t)(ADDR)[1] & 0xC0) == 0x80 )
#define IS_IPV6_ADDR_GUA(ADDR) ( ((ADDR)[0] & 0xE0) == 0x20 )
#define ADDRESS_RANGE_STR(ADDR) (                                                           \
                                    IS_IPV6_ADDR_ULA(ADDR) ?                                \
                                        "ULA: " :                                           \
                                        (( IS_IPV6_ADDR_LUA(ADDR) ) ?                       \
                                                "LUA: " :                                   \
                                                ( IS_IPV6_ADDR_GUA(ADDR) ? "GUA: " : "" ))  \
                                )

// Logging macros
#if OS_LOG_ENABLED
    // Define log specifier
    // String
#    define PUB_S_SRP "%{public}s"
#    define PRI_S_SRP "%{private, mask.hash}s"
    // DNS name, when the pointer to DNS name is NULL, <NULL> will be logged.
#    define DNS_NAME_GEN_SRP(NAME, BUF_NAME) \
        char BUF_NAME[DNS_MAX_NAME_SIZE_ESCAPED + 1]; \
        if (NAME != NULL) { \
            dns_name_print(NAME, BUF_NAME, sizeof(BUF_NAME)); \
        } else { \
            snprintf(BUF_NAME, sizeof(BUF_NAME), "<null>"); \
        }
#    define PUB_DNS_NAME_SRP PUB_S_SRP
#    define PRI_DNS_NAME_SRP PRI_S_SRP
#    define DNS_NAME_PARAM_SRP(NAME, BUF) (BUF)
    // IP address
    // IPv4
#    define IPv4_ADDR_GEN_SRP(ADDR, BUF_NAME) do {} while(0)
#    define PUB_IPv4_ADDR_SRP "%{public, network:in_addr}.4P"
#    define PRI_IPv4_ADDR_SRP "%{private, mask.hash, network:in_addr}.4P"
#    define IPv4_ADDR_PARAM_SRP(ADDR, BUF) ((uint8_t *)ADDR)
    // IPv6
#    define IPv6_ADDR_GEN_SRP(ADDR, BUF_NAME) do {} while(0)
#    define PUB_IPv6_ADDR_SRP "%{public, network:in6_addr}.16P%{public}s"
#    define PRI_IPv6_ADDR_SRP "%{public}s%{private, mask.hash, network:in6_addr}.16P"
#    define IPv6_ADDR_PARAM_SRP(ADDR, BUF) ADDRESS_RANGE_STR((uint8_t *)(ADDR)), ((uint8_t *)(ADDR))
    // Segmented IPv6
    // Subnet part can always be public.
#    define SEGMENTED_IPv6_ADDR_GEN_SRP(ADDR, BUF_NAME) do {} while(0)
#    define PUB_SEGMENTED_IPv6_ADDR_SRP "{%{public, srp:in6_addr_segment}.6P%{public}s, " \
                                            "%{public, srp:in6_addr_segment}.2P, " \
                                            "%{public, srp:in6_addr_segment}.8P}"
#    define PRI_SEGMENTED_IPv6_ADDR_SRP "{%{public}s%{private, mask.hash, srp:in6_addr_segment}.6P:" \
                                            "%{public, mask.hash, srp:in6_addr_segment}.2P:" \
                                            "%{private, mask.hash, srp:in6_addr_segment}.8P}"
#    define SEGMENTED_IPv6_ADDR_PARAM_SRP(ADDR, BUF) ADDRESS_RANGE_STR((uint8_t *)(ADDR)), ((uint8_t *)(ADDR)), \
                                                        ((uint8_t *)(ADDR) + 6), ((uint8_t *)(ADDR) + 8)
    // MAC address
#    define PUB_MAC_ADDR_SRP "%{public, srp:mac_addr}.6P"
#    define PRI_MAC_ADDR_SRP "%{private, mask.hash, srp:mac_addr}.6P"
#    define MAC_ADDR_PARAM_SRP(ADDR) ((uint8_t *)ADDR)

#else // ifdef OS_LOG_ENABLED
    // When os_log is not available, all logs would be public.
    // Define log specifier
    // String
#    define PUB_S_SRP "%s"
#    define PRI_S_SRP PUB_S_SRP
    // DNS name, when the pointer to DNS name is NULL, <NULL> will be logged.
#    if defined(MDNS_NO_STRICT) && (!MDNS_NO_STRICT)
#        define SRP_LOG_STRNCPY_STRICT mdns_strlcpy
#    else
#        define SRP_LOG_STRNCPY_STRICT strlcpy
#    endif
#    ifdef IOLOOP_MACOS
#        define SRP_LOG_STRNCPY SRP_LOG_STRNCPY_STRICT
#    else
#        define SRP_LOG_STRNCPY strncpy
#    endif // IOLOOP_MACOS
#    define DNS_NAME_GEN_SRP(NAME, BUF_NAME) \
        char BUF_NAME[DNS_MAX_NAME_SIZE_ESCAPED + 1]; \
        if (NAME != NULL) { \
            dns_name_print(NAME, BUF_NAME, sizeof(BUF_NAME)); \
        } else { \
            SRP_LOG_STRNCPY(BUF_NAME, "<null>", \
                            sizeof("<null>") < sizeof(BUF_NAME) ? sizeof("<null>") : sizeof(BUF_NAME)); \
        }
#    define PUB_DNS_NAME_SRP "%s"
#    define PRI_DNS_NAME_SRP PUB_DNS_NAME_SRP
#    define DNS_NAME_PARAM_SRP(NAME, BUF) (BUF)
    // IP address
    // IPv4
#    define IPv4_ADDR_GEN_SRP(ADDR, BUF_NAME) char BUF_NAME[INET_ADDRSTRLEN]; \
                                                    inet_ntop(AF_INET, ((uint8_t *)ADDR), BUF_NAME, sizeof(BUF_NAME))
#    define PUB_IPv4_ADDR_SRP "%s"
#    define PRI_IPv4_ADDR_SRP PUB_IPv4_ADDR_SRP
#    define IPv4_ADDR_PARAM_SRP(ADDR, BUF) (BUF)
    // IPv6
#    define IPv6_ADDR_GEN_SRP(ADDR, BUF_NAME) char BUF_NAME[INET6_ADDRSTRLEN]; \
                                                    inet_ntop(AF_INET6, ((uint8_t *)ADDR), BUF_NAME, sizeof(BUF_NAME))
#    define PUB_IPv6_ADDR_SRP "%s%s"
#    define PRI_IPv6_ADDR_SRP PUB_IPv6_ADDR_SRP
#    define IPv6_ADDR_PARAM_SRP(ADDR, BUF) (BUF), ADDRESS_RANGE_STR((uint8_t *)ADDR)
    // Segmented IPv6
#    define SEGMENTED_IPv6_ADDR_GEN_SRP(ADDR, BUF_NAME) IPv6_ADDR_GEN_SRP(ADDR, BUF_NAME)

#    define PUB_SEGMENTED_IPv6_ADDR_SRP PUB_IPv6_ADDR_SRP
#    define PRI_SEGMENTED_IPv6_ADDR_SRP PRI_IPv6_ADDR_SRP
#    define SEGMENTED_IPv6_ADDR_PARAM_SRP(ADDR, BUF) IPv6_ADDR_PARAM_SRP(ADDR, BUF)
    // MAC address
#    define PUB_MAC_ADDR_SRP "%02x:%02x:%02x:%02x:%02x:%02x"
#    define PRI_MAC_ADDR_SRP PUB_MAC_ADDR_SRP
#    define MAC_ADDR_PARAM_SRP(ADDR) ((uint8_t *)ADDR)[0], ((uint8_t *)ADDR)[1], ((uint8_t *)ADDR)[2], \
                                        ((uint8_t *)ADDR)[3], ((uint8_t *)ADDR)[4], ((uint8_t *)ADDR)[5]

#endif // ifdef OS_LOG_ENABLED

// IPv6 ULA 48-bit prefix
#define IPv6_PREFIX_GEN_SRP(PREFIX, PREFIX_LEN, BUF_NAME) \
    struct in6_addr _in6_addr_##BUF_NAME##_full_addr = {0}; \
    memcpy(_in6_addr_##BUF_NAME##_full_addr.s6_addr, (PREFIX), \
        MIN(sizeof(_in6_addr_##BUF_NAME##_full_addr.s6_addr), (PREFIX_LEN))); \
    SEGMENTED_IPv6_ADDR_GEN_SRP(_in6_addr_##BUF_NAME##_full_addr.s6_addr, BUF_NAME);
#define PUB_IPv6_PREFIX_SRP PUB_SEGMENTED_IPv6_ADDR_SRP
#define PRI_IPv6_PREFIX_SRP PRI_SEGMENTED_IPv6_ADDR_SRP
#define IPv6_PREFIX_PARAM_SRP(BUF_NAME) SEGMENTED_IPv6_ADDR_PARAM_SRP(_in6_addr_##BUF_NAME##_full_addr.s6_addr, \
                                            BUF_NAME)

//======================================================================================================================
// MARK: - To String Helpers

/*!
 *  @brief
 *      Convert DNS question class to its corresponding text description.
 *
 *  @param qclass
 *      The DNS question class value in the DNS message.
 *
 *  @result
 *      The corresponding text description for the given DNS question class if it is valid. Otherwise, an error string will be returned.
 */
const char *
dns_qclass_to_string(uint16_t qclass);

/*!
 *  @brief
 *      Convert DNS record type to its corresponding text description.
 *
 *  @param rrtype
 *      The DNS record type value of the DNS record.
 *
 *  @result
 *      The corresponding text description for the given DNS record type if it is valid. Otherwise, an error string will be returned.
 */
const char *
dns_rrtype_to_string(uint16_t rrtype);

#endif // __SRP_LOG_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
