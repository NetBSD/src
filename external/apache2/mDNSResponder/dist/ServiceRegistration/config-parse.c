/* -*- Mode: C; tab-width: 4; c-file-style: "bsd"; c-basic-offset: 4; fill-column: 108 -*-
 *
 * Copyright (c) 2002-2004 Apple Computer, Inc. All rights reserved.
 * Copyright (c) 2018 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//*************************************************************************************************************
//
// General purpose stupid little parser, currently used by dnssd-proxy for its configuration file
//
//*************************************************************************************************************
// Headers

#include <stdio.h>          // For printf()
#include <stdlib.h>         // For malloc()
#include <string.h>         // For strrchr(), strcmp()
#include <time.h>           // For "struct tm" etc.
#include <signal.h>         // For SIGINT, SIGTERM
#include <assert.h>
#include <netdb.h>           // For gethostbyname()
#include <sys/socket.h>      // For AF_INET, AF_INET6, etc.
#include <net/if.h>          // For IF_NAMESIZE
#include <netinet/in.h>      // For INADDR_NONE
#include <netinet/tcp.h>     // For SOL_TCP, TCP_NOTSENT_LOWAT
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#include "srp.h"
#include "config-parse.h"

#ifdef STANDALONE
#undef LogMsg
#define LogMsg(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#endif // STANDALONE

// Parse one line of a config file.
// A line consists of a verb followed by one or more hunks of text.
// We parse the verb first, then that tells us how many hunks of text to expect.
// Each hunk is space-delineated; the last hunk can contain spaces.
static bool config_parse_line(void *context, const char *filename, char *line, int lineno,
                              config_file_verb_t *verbs, int num_verbs)
{
    char *sp;
#define MAXCFHUNKS 10
    char *hunks[MAXCFHUNKS];
    int num_hunks = 0;
    config_file_verb_t *config_file_verb = NULL;
    int i;

    sp = line;
    do {
        // Skip leading spaces.
        while (*sp && (*sp == ' ' || *sp == '\t'))
            sp++;
        if (num_hunks == 0) {
            // If this is a blank line with spaces on it or a comment line, we ignore it.
            if (!*sp || *sp == '#')
                return true;
        }
        hunks[num_hunks++] = sp;
        // Find EOL or hunk
        while (*sp && (*sp != ' ' && *sp != '\t')) {
            sp++;
        }
        if (*sp) {
            *sp++ = 0;
        }
        if (num_hunks == 1) {
            for (i = 0; i < num_verbs; i++) {
                // If the verb name matches, or the verb name is NULL (meaning whatever doesn't
                // match a preceding verb), we've found our verb.
                if (verbs[i].name == NULL || !strcmp(verbs[i].name, hunks[0])) {
                    config_file_verb = &verbs[i];
                    break;
                }
            }
            if (config_file_verb == NULL) {
                INFO("unknown verb %s at line %d", hunks[0], lineno);
                return false;
            }
        }
    } while (*sp && num_hunks < MAXCFHUNKS && config_file_verb->max_hunks > num_hunks);

    // If we didn't get the hunks we needed, bail.
    if (config_file_verb->min_hunks > num_hunks) {
        INFO("error: verb %s requires between %d and %d modifiers; %d given at line %d",
             hunks[0], config_file_verb->min_hunks, config_file_verb->max_hunks, num_hunks, lineno);
        return false;
    }

    return config_file_verb->handler(context, filename, hunks, num_hunks, lineno);
}

// Parse a configuration file
bool config_parse(void *context, const char *filename, config_file_verb_t *verbs, int num_verbs)
{
    int file;
    char *buf, *line, *eof, *eol, *nextCR, *nextNL;
    off_t flen;
    ssize_t len;
    size_t have;
    int lineno;
    bool success = true;

    file = open(filename, O_RDONLY);
    if (file < 0) {
        INFO("fatal: %s: %s", filename, strerror(errno));
        return false;
    }

    // Get the length of the file.
    flen = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);
    if (flen > 500 * 1024 || (buf = malloc((size_t)flen + 1)) == NULL) {
        INFO("fatal: not enough memory for %s", filename);
        goto outclose;
    }
    size_t fsize = (size_t)flen;

    // Just in case we have a read() syscall that doesn't always read the whole file at once
    have = 0;
    while (have < fsize) {
        len = read(file, &buf[have], fsize - have);
        if (len < 0) {
            INFO("fatal: read of %s at %lld len %lld: %s",
                 filename, (long long)have, (long long)(fsize - have), strerror(errno));
            goto outfree;
        }
        if (len == 0) {
            INFO("fatal: read of %s at %lld len %lld: zero bytes read",
                 filename, (long long)have, (long long)(fsize - have));
        outfree:
            free(buf);
        outclose:
            close(file);
            return false;
        }
        have += len;
    }
    close(file);
    buf[flen] = 0; // NUL terminate.
    eof = buf + flen;

    // Parse through the file line by line.
    line = buf;
    lineno = 1;
    while (line < eof) { // < because NUL at eof could be last eol.
        nextCR = strchr(line, '\r');
        nextNL = strchr(line, '\n');

        // Added complexity for CR/LF agnostic line endings.   Necessary?
        if (nextNL != NULL) {
            if (nextCR != NULL && nextCR < nextNL)
                eol = nextCR;
            else
                eol = nextNL;
        } else {
            if (nextCR != NULL)
                eol = nextCR;
            else
                eol = buf + flen;
        }

        // If this isn't a blank line or a comment line, parse it.
        if (eol - line != 1 && line[0] != '#') {
            *eol = 0;
            // If we get a bad config line, we're going to return failure later, but continue parsing now.
            if (!config_parse_line(context, filename, line, lineno, verbs, num_verbs))
                success = false;
        }
        line = eol + 1;
        lineno++;
    }
    free(buf);
    return success;
}
