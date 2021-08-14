/*	$NetBSD: ppm.c,v 1.2 2021/08/14 16:14:53 christos Exp $	*/

/*
 * ppm.c for OpenLDAP
 *
 * See LICENSE, README and INSTALL files
 */


/*
  password policy module is called with:
  int check_password (char *pPasswd, char **ppErrStr, Entry *e, void *pArg)

  *pPasswd: new password
  **ppErrStr: pointer to the string containing the error message
  *e: pointer to the current user entry
  *pArg: pointer to a struct berval holding the value of pwdCheckModuleArg attr
*/

#include <stdlib.h>             // for type conversion, such as atoi...
#include <regex.h>              // for matching allowedParameters / conf file
#include <string.h>
#include <ctype.h>
#include <portable.h>
#include <slap.h>
#include <stdarg.h>             // for variable nb of arguments functions
#include "ppm.h"

#ifdef CRACKLIB
#include "crack.h"              // use cracklib to check password
#endif

void
ppm_log(int priority, const char *format, ...)
{
  // if DEBUG flag is set
  // logs into syslog (for OpenLDAP) or to stdout (for tests)
#if defined(DEBUG)
  if(ppm_test != 1)
  {
    va_list syslog_args;
    va_start(syslog_args, format);
    vsyslog(priority, format, syslog_args);
    va_end(syslog_args);
  }
  else
  {
    va_list stdout_args;
    va_start(stdout_args, format);
    vprintf(format, stdout_args);
    printf("\n");
    fflush(stdout);
    va_end(stdout_args);
  }
#endif
}

void
strcpy_safe(char *dest, char *src, int length_dest)
{
    if(src == NULL)
    {
        dest[0] = '\0';
    }
    else
    {
        int length_src = strlen(src);
        int n = (length_dest < length_src) ? length_dest : length_src;
        // Copy the string — don’t copy too many bytes.
        strncpy(dest, src, n);
        // Ensure null-termination.
        dest[n] = '\0';
    }
}

genValue*
getValue(conf *fileConf, int numParam, char* param)
{
    int i = 0;

    // First scan parameters
    for (i = 0; i < numParam; i++) {
        if ((strlen(param) == strlen(fileConf[i].param))
            && (strncmp(param, fileConf[i].param, strlen(fileConf[i].param))
                == 0)) {
            return &(fileConf[i].value);
        }
    }
    return NULL;
}

int maxConsPerClass(char *password, char *charClass)
{
  // find maximum number of consecutive class characters in the password

  int bestMax = 0;
  int max = 0;
  int i;

  for(i=0 ; i<strlen(password) ; i++)
  {
    if(strchr(charClass,password[i]) != NULL)
    {
      // current character is in class
      max++;
      // is the new max a better candidate to maxConsecutivePerClass?
      if(max > bestMax)
      {
        // found a better maxConsecutivePerClass
        bestMax = max;
      }
    }
    else
    {
      // current character is not in class
      // reinitialize max
      max=0;
    }
  }
  return bestMax;
}

void
storeEntry(char *param, char *value, valueType valType, 
           char *min, char *minForPoint, conf * fileConf, int *numParam)
{
    int i = 0;
    int iMin;
    int iMinForPoint;
    if (min == NULL || strcmp(min,"") == 0)
      iMin = 0;
    else
      iMin = atoi(min);

    if (minForPoint == NULL || strcmp(minForPoint,"") == 0)
      iMinForPoint = 0;
    else
      iMinForPoint = atoi(minForPoint);

    // First scan parameters
    for (i = 0; i < *numParam; i++) {
        if ((strlen(param) == strlen(fileConf[i].param))
            && (strncmp(param, fileConf[i].param, strlen(fileConf[i].param))
                == 0)) {
            // entry found, replace values
            if(valType == typeInt)
                fileConf[i].value.iVal = atoi(value);
            else
                strcpy_safe(fileConf[i].value.sVal, value, VALUE_MAX_LEN);
            fileConf[i].min = iMin;
            fileConf[i].minForPoint = iMinForPoint;
            if(valType == typeInt)
                ppm_log(LOG_NOTICE, "ppm:  Accepted replaced value: %d",
                               fileConf[i].value.iVal);
            else
                ppm_log(LOG_NOTICE, "ppm:  Accepted replaced value: %s",
                               fileConf[i].value.sVal);
            return;
        }
    }
    // entry not found, add values
    strcpy_safe(fileConf[*numParam].param, param, PARAM_MAX_LEN);
    fileConf[*numParam].iType = valType;
    if(valType == typeInt)
        fileConf[i].value.iVal = atoi(value);
    else
        strcpy_safe(fileConf[i].value.sVal, value, VALUE_MAX_LEN);
    fileConf[*numParam].min = iMin;
    fileConf[*numParam].minForPoint = iMinForPoint;
    ++(*numParam);
            if(valType == typeInt)
                ppm_log(LOG_NOTICE, "ppm:  Accepted new value: %d",
                               fileConf[*numParam].value.iVal);
            else
                ppm_log(LOG_NOTICE, "ppm:  Accepted new value: %s",
                               fileConf[*numParam].value.sVal);
}

int
typeParam(char* param)
{
    int i;
    int n = sizeof(allowedParameters)/sizeof(params);

    regex_t regex;
    int reti;

    for(i = 0 ; i < n ; i++ )
    {
        // Compile regular expression
        reti = regcomp(&regex, allowedParameters[i].param, 0);
        if (reti) {
            ppm_log(LOG_ERR, "ppm: Cannot compile regex: %s",
                   allowedParameters[i].param);
            return n;
        }
        
        // Execute regular expression
        reti = regexec(&regex, param, 0, NULL, 0);
        if (!reti)
        {
            regfree(&regex);
            return i;
        } 
        regfree(&regex);
    }
    return n;
}

#ifndef PPM_READ_FILE

  /*
   * read configuration into pwdCheckModuleArg attribute
   * */
  static void
  read_config_attr(conf * fileConf, int *numParam, char *ppm_config_attr)
  {
    int nParam = 0;       // position of found parameter in allowedParameters
    int sAllowedParameters = sizeof(allowedParameters)/sizeof(params);
    char arg[260*256];
    char *token;
    char *saveptr1;
    char *saveptr2;
  
    strcpy_safe(arg, ppm_config_attr, 260*256);
    ppm_log(LOG_NOTICE, "ppm: Parsing pwdCheckModuleArg attribute");
    token = strtok_r(arg, "\n", &saveptr1);
  
    while (token != NULL) {
        ppm_log(LOG_NOTICE, "ppm: get line: %s",token);
        char *start = token;
        char *word, *value;
        char *min, *minForPoint;;
  
        while (isspace(*start) && isascii(*start))
            start++;
  
        if (!isascii(*start))
        {
            token = strtok_r(NULL, "\n", &saveptr1);
            continue;
        }
        if (start[0] == '#')
        {
            token = strtok_r(NULL, "\n", &saveptr1);
            continue;
        }
  
        if ((word = strtok_r(start, " \t", &saveptr2))) {
            if ((value = strtok_r(NULL, " \t", &saveptr2)) == NULL)
            {
                saveptr2 = NULL;
                ppm_log(LOG_NOTICE, "ppm: No value, goto next parameter");
                token = strtok_r(NULL, "\n", &saveptr1);
                continue;
            }
            if (strchr(value, '\n') != NULL)
                strchr(value, '\n')[0] = '\0';
            min = strtok_r(NULL, " \t", &saveptr2);
            if (min != NULL)
                if (strchr(min, '\n') != NULL)
                    strchr(min, '\n')[0] = '\0';
            minForPoint = strtok_r(NULL, " \t", &saveptr2);
            if (minForPoint != NULL)
                if (strchr(minForPoint, '\n') != NULL)
                    strchr(minForPoint, '\n')[0] = '\0';
  
  
            nParam = typeParam(word); // search for param in allowedParameters
            if (nParam != sAllowedParameters) // param has been found
            {
                ppm_log(LOG_NOTICE,
                   "ppm: Param = %s, value = %s, min = %s, minForPoint= %s",
                   word, value, min, minForPoint);
  
                storeEntry(word, value, allowedParameters[nParam].iType,
                           min, minForPoint, fileConf, numParam);
            }
            else
            {
                ppm_log(LOG_NOTICE,
                   "ppm: Parameter '%s' rejected", word);
            }
  
        }
        token = strtok_r(NULL, "\n", &saveptr1);
    }
  
  }

#endif

#ifdef PPM_READ_FILE

  /*
   * read configuration file (DEPRECATED)
   * */
  static void
  read_config_file(conf * fileConf, int *numParam, char *ppm_config_file)
  {
    FILE *config;
    char line[260] = "";
    int nParam = 0;       // position of found parameter in allowedParameters
    int sAllowedParameters = sizeof(allowedParameters)/sizeof(params);
  
    ppm_log(LOG_NOTICE, "ppm: Opening file %s", ppm_config_file);
    if ((config = fopen(ppm_config_file, "r")) == NULL) {
        ppm_log(LOG_ERR, "ppm: Opening file %s failed", ppm_config_file);
        exit(EXIT_FAILURE);
    }
  
    while (fgets(line, 256, config) != NULL) {
        char *start = line;
        char *word, *value;
        char *min, *minForPoint;;
  
        while (isspace(*start) && isascii(*start))
            start++;
  
        if (!isascii(*start))
            continue;
        if (start[0] == '#')
            continue;
  
        if ((word = strtok(start, " \t"))) {
            if ((value = strtok(NULL, " \t")) == NULL)
                continue;
            if (strchr(value, '\n') != NULL)
                strchr(value, '\n')[0] = '\0';
            min = strtok(NULL, " \t");
            if (min != NULL)
                if (strchr(min, '\n') != NULL)
                    strchr(min, '\n')[0] = '\0';
            minForPoint = strtok(NULL, " \t");
            if (minForPoint != NULL)
                if (strchr(minForPoint, '\n') != NULL)
                    strchr(minForPoint, '\n')[0] = '\0';
  
  
            nParam = typeParam(word); // search for param in allowedParameters
            if (nParam != sAllowedParameters) // param has been found
            {
                ppm_log(LOG_NOTICE,
                   "ppm: Param = %s, value = %s, min = %s, minForPoint= %s",
                   word, value, min, minForPoint);
  
                storeEntry(word, value, allowedParameters[nParam].iType,
                           min, minForPoint, fileConf, numParam);
            }
            else
            {
                ppm_log(LOG_NOTICE,
                   "ppm: Parameter '%s' rejected", word);
            }
  
        }
    }
  
    fclose(config);
  }

#endif

static int
realloc_error_message(char **target, int curlen, int nextlen)
{
    if (curlen < nextlen + MEMORY_MARGIN) {
        ppm_log(LOG_WARNING,
               "ppm: Reallocating szErrStr from %d to %d", curlen,
               nextlen + MEMORY_MARGIN);
        ber_memfree(*target);
        curlen = nextlen + MEMORY_MARGIN;
        *target = (char *) ber_memalloc(curlen);
    }

    return curlen;
}

// Does the password contains a token from the RDN ?
int
containsRDN(char* passwd, char* DN)
{
    char lDN[DN_MAX_LEN];
    char * tmpToken;
    char * token;
    regex_t regex;
    int reti;
 
    strcpy_safe(lDN, DN, DN_MAX_LEN);
 
    // Extract the RDN from the DN
    tmpToken = strtok(lDN, ",+");
    tmpToken = strtok(tmpToken, "=");
    tmpToken = strtok(NULL, "=");
 
    // Search for each token in the password */
    token = strtok(tmpToken, TOKENS_DELIMITERS);
 
    while (token != NULL)
    {
      if (strlen(token) > 2)
      {
        ppm_log(LOG_NOTICE, "ppm: Checking if %s part of RDN matches the password", token);
        // Compile regular expression
        reti = regcomp(&regex, token, REG_ICASE);
        if (reti) {
          ppm_log(LOG_ERR, "ppm: Cannot compile regex: %s", token);
          return 0;
        }
 
        // Execute regular expression
        reti = regexec(&regex, passwd, 0, NULL, 0);
        if (!reti)
        {
          regfree(&regex);
          return 1;
        }
 
        regfree(&regex);
      }
      else
      {
        ppm_log(LOG_NOTICE, "ppm: %s part of RDN is too short to be checked", token);
      }
      token = strtok(NULL, TOKENS_DELIMITERS);
    }
 
    return 0;
}


int
check_password(char *pPasswd, char **ppErrStr, Entry *e, void *pArg)
{

    Entry *pEntry = e;
    ppm_log(LOG_NOTICE, "ppm: entry %s", pEntry->e_nname.bv_val);

    struct berval *pwdCheckModuleArg = pArg;
    /* Determine if config file is to be read (DEPRECATED) */
    #ifdef PPM_READ_FILE
      ppm_log(LOG_NOTICE, "ppm: Not reading pwdCheckModuleArg attribute");
      ppm_log(LOG_NOTICE, "ppm: instead, read configuration file (deprecated)");
    #else
      ppm_log(LOG_NOTICE, "ppm: Reading pwdCheckModuleArg attribute");
      ppm_log(LOG_NOTICE, "ppm: RAW configuration: %s",
                          (*(struct berval*)pwdCheckModuleArg).bv_val);
    #endif

    char *szErrStr = (char *) ber_memalloc(MEM_INIT_SZ);
    int mem_len = MEM_INIT_SZ;
    int numParam = 0; // Number of params in current configuration

    int useCracklib;
    char cracklibDict[VALUE_MAX_LEN];
    char cracklibDictFiles[3][(VALUE_MAX_LEN+5)];
    char const* cracklibExt[] = { ".hwm", ".pwd", ".pwi" };
    FILE* fd;
    char* res;
    int minQuality;
    int checkRDN;
    char forbiddenChars[VALUE_MAX_LEN];
    int nForbiddenChars = 0;
    int nQuality = 0;
    int maxConsecutivePerClass;
    int nbInClass[CONF_MAX_SIZE];
    int i,j;

    /* Determine config file (DEPRECATED) */
    #ifdef PPM_READ_FILE
      char ppm_config_file[FILENAME_MAX_LEN];
      strcpy_safe(ppm_config_file, getenv("PPM_CONFIG_FILE"), FILENAME_MAX_LEN);
      if (ppm_config_file[0] == '\0') {
        strcpy_safe(ppm_config_file, CONFIG_FILE, FILENAME_MAX_LEN);
      }
      ppm_log(LOG_NOTICE, "ppm: reading config file from %s", ppm_config_file);
    #endif

    for (i = 0; i < CONF_MAX_SIZE; i++)
        nbInClass[i] = 0;

    /* Set default values */
    conf fileConf[CONF_MAX_SIZE] = {
        {"minQuality", typeInt, {.iVal = DEFAULT_QUALITY}, 0, 0
         }
        ,
        {"checkRDN", typeInt, {.iVal = 0}, 0, 0
         }
        ,
        {"forbiddenChars", typeStr, {.sVal = ""}, 0, 0
         }
        ,
        {"maxConsecutivePerClass", typeInt, {.iVal = 0}, 0, 0
         }
        ,
        {"useCracklib", typeInt, {.iVal = 0}, 0, 0
         }
        ,
        {"cracklibDict", typeStr, {.sVal = "/var/cache/cracklib/cracklib_dict"}, 0, 0
         }
        ,
        {"class-upperCase", typeStr, {.sVal = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, 0, 1
         }
        ,
        {"class-lowerCase", typeStr, {.sVal = "abcdefghijklmnopqrstuvwxyz"}, 0, 1
         }
        ,
        {"class-digit", typeStr, {.sVal = "0123456789"}, 0, 1
         }
        ,
        {"class-special", typeStr,
         {.sVal = "<>,?;.:/!§ù%*µ^¨$£²&é~\"#'{([-|è`_\\ç^à@)]°=}+"}, 0, 1
         }
    };
    numParam = 10;

    #ifdef PPM_READ_FILE
      /* Read configuration file (DEPRECATED) */
      read_config_file(fileConf, &numParam, ppm_config_file);
    #else
      /* Read configuration attribute (pwdCheckModuleArg) */
      read_config_attr(fileConf, &numParam, (*(struct berval*)pwdCheckModuleArg).bv_val);
    #endif

    minQuality = getValue(fileConf, numParam, "minQuality")->iVal;
    checkRDN = getValue(fileConf, numParam, "checkRDN")->iVal;
    strcpy_safe(forbiddenChars,
                getValue(fileConf, numParam, "forbiddenChars")->sVal,
                VALUE_MAX_LEN);
    maxConsecutivePerClass = getValue(fileConf, numParam, "maxConsecutivePerClass")->iVal;
    useCracklib = getValue(fileConf, numParam, "useCracklib")->iVal;
    strcpy_safe(cracklibDict,
                getValue(fileConf, numParam, "cracklibDict")->sVal,
                VALUE_MAX_LEN);


    /*The password must have at least minQuality strength points with one
     * point granted if the password contains at least minForPoint characters for each class
     * It must contains at least min chars of each class
     * It must not contain any char in forbiddenChar */

    for (i = 0; i < strlen(pPasswd); i++) {

        int n;
        for (n = 0; n < numParam; n++) {
            if (strstr(fileConf[n].param, "class-") != NULL) {
                if (strchr(fileConf[n].value.sVal, pPasswd[i]) != NULL) {
                    ++(nbInClass[n]);
                }
            }
        }
        if (strchr(forbiddenChars, pPasswd[i]) != NULL) {
            nForbiddenChars++;
        }
    }

    // Password checking done, now loocking for minForPoint criteria
    for (i = 0; i < CONF_MAX_SIZE; i++) {
        if (strstr(fileConf[i].param, "class-") != NULL) {
            if ((nbInClass[i] >= fileConf[i].minForPoint)
                && strlen(fileConf[i].value.sVal) != 0) {
                // 1 point granted
                ++nQuality;
                ppm_log(LOG_NOTICE, "ppm: 1 point granted for class %s",
                       fileConf[i].param);
            }
        }
    }

    if (nQuality < minQuality) {
        mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(PASSWORD_QUALITY_SZ) +
                                        strlen(pEntry->e_nname.bv_val) + 4);
        sprintf(szErrStr, PASSWORD_QUALITY_SZ, pEntry->e_nname.bv_val,
                nQuality, minQuality);
        goto fail;
    }
    // Password checking done, now loocking for constraintClass criteria
    for (i = 0; i < CONF_MAX_SIZE; i++) {
        if (strstr(fileConf[i].param, "class-") != NULL) {
            if ((nbInClass[i] < fileConf[i].min) &&
                 strlen(fileConf[i].value.sVal) != 0) {
                // constraint is not satisfied... goto fail
                mem_len = realloc_error_message(&szErrStr, mem_len,
                                                strlen(PASSWORD_CRITERIA) +
                                                strlen(pEntry->e_nname.bv_val) + 
                                                2 + PARAM_MAX_LEN);
                sprintf(szErrStr, PASSWORD_CRITERIA, pEntry->e_nname.bv_val,
                        fileConf[i].min, fileConf[i].param);
                goto fail;
            }
        }
    }

    // Password checking done, now loocking for forbiddenChars criteria
    if (nForbiddenChars > 0) {  // at least 1 forbidden char... goto fail
        mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(PASSWORD_FORBIDDENCHARS) +
                                        strlen(pEntry->e_nname.bv_val) + 2 +
                                        VALUE_MAX_LEN);
        sprintf(szErrStr, PASSWORD_FORBIDDENCHARS, pEntry->e_nname.bv_val,
                nForbiddenChars, forbiddenChars);
        goto fail;
    }

    // Password checking done, now loocking for maxConsecutivePerClass criteria
    for (i = 0; i < CONF_MAX_SIZE; i++) {
        if (strstr(fileConf[i].param, "class-") != NULL) {
            if ( maxConsecutivePerClass != 0 &&
                (maxConsPerClass(pPasswd,fileConf[i].value.sVal)
                                                 > maxConsecutivePerClass)) {
                // Too much consecutive characters of the same class
                ppm_log(LOG_NOTICE, "ppm: Too much consecutive chars for class %s",
                       fileConf[i].param);
                mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(PASSWORD_MAXCONSECUTIVEPERCLASS) +
                                        strlen(pEntry->e_nname.bv_val) + 2 +
                                        PARAM_MAX_LEN);
                sprintf(szErrStr, PASSWORD_MAXCONSECUTIVEPERCLASS, pEntry->e_nname.bv_val,
                        maxConsecutivePerClass, fileConf[i].param);
                goto fail;
            }
        }
    }
#ifdef CRACKLIB
    // Password checking done, now loocking for cracklib criteria
    if ( useCracklib > 0 ) {

        for( j = 0 ; j < 3 ; j++) {
            strcpy_safe(cracklibDictFiles[j], cracklibDict, VALUE_MAX_LEN);
            strcat(cracklibDictFiles[j], cracklibExt[j]);
            if (( fd = fopen ( cracklibDictFiles[j], "r")) == NULL ) {
                ppm_log(LOG_NOTICE, "ppm: Error while reading %s file",
                       cracklibDictFiles[j]);
                mem_len = realloc_error_message(&szErrStr, mem_len,
                                strlen(GENERIC_ERROR));
                sprintf(szErrStr, GENERIC_ERROR);
                goto fail;

            }
            else {
                fclose (fd);
            }
        }
        res = (char *) FascistCheck (pPasswd, cracklibDict);
        if ( res != NULL ) {
                ppm_log(LOG_NOTICE, "ppm: cracklib does not validate password for entry %s",
                       pEntry->e_nname.bv_val);
                mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(PASSWORD_CRACKLIB) +
                                        strlen(pEntry->e_nname.bv_val));
                sprintf(szErrStr, PASSWORD_CRACKLIB, pEntry->e_nname.bv_val);
                goto fail;
        
        }

    }
#endif

    // Password checking done, now looking for checkRDN criteria
    if (checkRDN == 1 && containsRDN(pPasswd, pEntry->e_nname.bv_val))
    // RDN check enabled and a token from RDN is found in password: goto fail
    {
        mem_len = realloc_error_message(&szErrStr, mem_len,
                                        strlen(RDN_TOKEN_FOUND) +
                                        strlen(pEntry->e_nname.bv_val));
        sprintf(szErrStr, RDN_TOKEN_FOUND, pEntry->e_nname.bv_val);

        goto fail;
    }

    *ppErrStr = strdup("");
    ber_memfree(szErrStr);
    return (LDAP_SUCCESS);

  fail:
    *ppErrStr = strdup(szErrStr);
    ber_memfree(szErrStr);
    return (EXIT_FAILURE);

}
