#ifndef _IFCONFIG_ENV_H
#define _IFCONFIG_ENV_H

#include <prop/proplib.h>

const char *getifname(prop_dictionary_t);
ssize_t getargstr(prop_dictionary_t, const char *, char *, size_t);
ssize_t getargdata(prop_dictionary_t, const char *, uint8_t *, size_t);
int getaf(prop_dictionary_t);
int getifflags(prop_dictionary_t, prop_dictionary_t, unsigned short *);
const char *getifinfo(prop_dictionary_t, prop_dictionary_t, unsigned short *);
prop_dictionary_t prop_dictionary_augment(prop_dictionary_t, prop_dictionary_t);

#endif /* _IFCONFIG_ENV_H */
