/*	$NetBSD: ppm.h,v 1.3 2025/09/05 21:16:18 christos Exp $	*/

/*
 * ppm.h for OpenLDAP
 *
 * See LICENSE, README and INSTALL files
 */

#ifndef PPM_H_
#define PPM_H_

#include <stdlib.h>             // for type conversion, such as atoi...
#include <regex.h>              // for matching allowedParameters / conf file
#include <string.h>
#include <ctype.h>
#include <portable.h>
#include <slap.h>

#if defined(DEBUG)
#include <syslog.h>
#endif

// Get OpenLDAP version
#define OLDAP_VERSION ((LDAP_VENDOR_VERSION_MAJOR << 8) | LDAP_VENDOR_VERSION_MINOR)
// OLDAP_VERSION = 0x0205 // (v2.5)
// OLDAP_VERSION = 0x0206 // (v2.6)

//#define PPM_READ_FILE 1       // old deprecated configuration mode
                                // 1: (deprecated) don't read pwdCheckModuleArg
                                //    attribute, instead read config file
                                // 0: read pwdCheckModuleArg attribute

/* config file parameters (DEPRECATED) */
#ifndef CONFIG_FILE
#define CONFIG_FILE                       "/etc/openldap/ppm.example"
#endif
#define FILENAME_MAX_LEN                  512

#define DEFAULT_QUALITY                   3
#define MEMORY_MARGIN                     50
#if OLDAP_VERSION == 0x0205
  #define MEM_INIT_SZ                     64
#endif
#define DN_MAX_LEN                        512

#define CONF_MAX_SIZE                      50
#define PARAM_MAX_LEN                      32
#define VALUE_MAX_LEN                      512
#define ATTR_NAME_MAX_LEN                  150

#define PARAM_PREFIX_CLASS                "class-"
#define TOKENS_DELIMITERS                 " ,;-_£\t"
#define ATTR_TOKENS_DELIMITERS            " ,;-_@\t"


#define DEBUG_MSG_MAX_LEN                 256

#define PASSWORD_QUALITY_SZ \
  "Password for dn=\"%s\" does not pass required number of strength checks (%d of %d)"
#define PASSWORD_MIN_CRITERIA \
  "Password for dn=\"%s\" has not reached the minimum number of characters (%d) for class %s"
#define PASSWORD_MAX_CRITERIA \
  "Password for dn=\"%s\" has reached the maximum number of characters (%d) for class %s"
#define PASSWORD_MAXCONSECUTIVEPERCLASS \
  "Password for dn=\"%s\" has reached the maximum number of characters (%d) for class %s"
#define PASSWORD_FORBIDDENCHARS \
  "Password for dn=\"%s\" contains %d forbidden characters in %s"
#define RDN_TOKEN_FOUND \
  "Password for dn=\"%s\" contains tokens from the RDN"
#define ATTR_TOKEN_FOUND \
  "Password for dn=\"%s\" is too simple: it contains part of an attribute"
#define GENERIC_ERROR \
  "Error while checking password"
#define PASSWORD_CRACKLIB \
  "Password for dn=\"%s\" is too weak"
#define BAD_PASSWORD_SZ \
  "Bad password for dn=\"%s\" because %s"



typedef union genValue {
    int iVal;
    char sVal[VALUE_MAX_LEN];
} genValue;

typedef enum {
    typeInt,
    typeStr
} valueType;

typedef struct params {
    char param[PARAM_MAX_LEN];
    valueType iType;
} params;

// allowed parameters loaded into configuration structure
// it also contains the type of the corresponding value
params allowedParameters[8] = {
    {"^minQuality", typeInt},
    {"^checkRDN", typeInt},
    {"^checkAttributes", typeStr},
    {"^forbiddenChars", typeStr},
    {"^maxConsecutivePerClass", typeInt},
    {"^useCracklib", typeInt},
    {"^cracklibDict", typeStr},
    {"^class-.*", typeStr}
};


// configuration structure, containing a parameter, a value,
// a corresponding min and minForPoint indicators if necessary
// and a type for the value (typeInt or typeStr)
typedef struct conf {
    char param[PARAM_MAX_LEN];
    valueType iType;
    genValue value;
    int min;
    int minForPoint;
    int max;
} conf;

void ppm_log(int priority, const char *format, ...);
int min(char *str1, char *str2);
#ifndef PPM_READ_FILE
  static void read_config_attr(conf * fileConf, int *numParam, char *ppm_config_attr);
#endif
#ifdef PPM_READ_FILE
  static void read_config_file(conf * fileConf, int *numParam, char *ppm_config_file);
#endif

#if OLDAP_VERSION == 0x0205
  int check_password(char *pPasswd, char **ppErrStr, Entry *e, void *pArg);
#else
  int check_password(char *pPasswd, struct berval *ppErrmsg, Entry *e, void *pArg);
#endif
int maxConsPerClass(char *password, char *charClass);
void storeEntry(char *param, char *value, valueType valType, 
           char *min, char *minForPoint, char *max, conf * fileConf,
           int *numParam);
int typeParam(char* param);
genValue* getValue(conf *fileConf, int numParam, char* param);
void strcpy_safe(char *dest, char *src, int length_dest);


int ppm_test = 0;

#endif
