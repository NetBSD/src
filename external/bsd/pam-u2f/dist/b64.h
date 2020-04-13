/*
 * Copyright (C) 2018 Yubico AB - See COPYING
 */

#ifndef B64_H
#define B64_H

int b64_encode(const void *, size_t, char **);
int b64_decode(const char *, void **, size_t *);

#endif /* B64_H */
