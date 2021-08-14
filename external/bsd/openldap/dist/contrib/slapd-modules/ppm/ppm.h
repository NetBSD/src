/*	$NetBSD: ppm.h,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

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
#define MEM_INIT_SZ                       64
#define DN_MAX_LEN                        512

#define CONF_MAX_SIZE                      50
#define PARAM_MAX_LEN                      32
#define VALUE_MAX_LEN                      128
#define ATTR_NAME_MAX_LEN                  150

#define PARAM_PREFIX_CLASS                "class-"
#define TOKENS_DELIMITERS                 " ,;-_£\t"


#define DEBUG_MSG_MAX_LEN                 256

#define PASSWORD_QUALITY_SZ \
  "Password for dn=\"%s\" does not pass required number of strength checks (%d of %d)"
#define PASSWORD_CRITERIA \
  "Password for dn=\"%s\" has not reached the minimum number of characters (%d) for class %s"
#define PASSWORD_MAXCONSECUTIVEPERCLASS \
  "Password for dn=\"%s\" has reached the maximum number of characters (%d) for class %s"
#define PASSWORD_FORBIDDENCHARS \
  "Password for dn=\"%s\" contains %d forbidden characters in %s"
#define RDN_TOKEN_FOUND \
  "Password for dn=\"%s\" contains tokens from the RDN"
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
params allowedParameters[7] = {
    {"^minQuality", typeInt},
    {"^checkRDN", typeInt},
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
} conf;

void ppm_log(int priority, const char *format, ...);
int min(char *str1, char *str2);
#ifndef PPM_READ_FILE
  static void read_config_attr(conf * fileConf, int *numParam, char *ppm_config_attr);
#endif
#ifdef PPM_READ_FILE
  static void read_config_file(conf * fileConf, int *numParam, char *ppm_config_file);
#endif
int check_password(char *pPasswd, char **ppErrStr, Entry *e, void *pArg);
int maxConsPerClass(char *password, char *charClass);
void storeEntry(char *param, char *value, valueType valType, 
           char *min, char *minForPoint, conf * fileConf, int *numParam);
int typeParam(char* param);
genValue* getValue(conf *fileConf, int numParam, char* param);
void strcpy_safe(char *dest, char *src, int length_dest);


int ppm_test = 0;

#endif
