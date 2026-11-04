---
title: ppm
section: 5
header: File Formats Manual
footer: ppm
date: August 24, 2021
---

# NAME

ppm (Password Policy Module) - extension of the password policy overlay

# SYNOPSIS

ETCDIR/ppm.example

# DESCRIPTION

**ppm** is an OpenLDAP module for checking password quality when they are modified.
Passwords are checked against the presence or absence of certain character classes.

This module is used as an extension of the OpenLDAP password policy controls,
see slapo-ppolicy(5) section **pwdCheckModule**.


# USAGE

Create a password policy entry and indicate the path of the ppm.so library
and the content of the desired policy.
Use a base64 tool to code / decode the content of the policy stored into
**pwdCheckModuleArg**.

Here is an example for OpenLDAP 2.6:

```
dn: cn=default,ou=policies,dc=my-domain,dc=com
objectClass: pwdPolicy
objectClass: top
objectClass: pwdPolicyChecker
objectClass: person
pwdCheckQuality: 2
pwdAttribute: userPassword
sn: default
cn: default
pwdMinLength: 6
pwdCheckModuleArg:: bWluUXVhbGl0eSAzCmNoZWNrUkROIDAKY2hlY2tBdHRyaWJ1dGVzCmZvcmJpZGRlbkNoYXJzCm1heENvbnNlY3V0aXZlUGVyQ2xhc3MgMAp1c2VDcmFja2xpYiAwCmNyYWNrbGliRGljdCAvdmFyL2NhY2hlL2NyYWNrbGliL2NyYWNrbGliX2RpY3QKY2xhc3MtdXBwZXJDYXNlIEFCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaIDAgMSAwCmNsYXNzLWxvd2VyQ2FzZSBhYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5eiAwIDEgMApjbGFzcy1kaWdpdCAwMTIzNDU2Nzg5IDAgMSAwCmNsYXNzLXNwZWNpYWwgPD4sPzsuOi8hwqfDuSUqwrVewqgkwqPCsibDqX4iIyd7KFstfMOoYF9cw6dew6BAKV3CsD19KyAwIDEgMAoK
pwdUseCheckModule: TRUE
```

For OpenLDAP 2.5, you must add a **pwdCheckModule** attribute pointing
to the ppm module (for example /usr/local/lib/ppm.so),
and remove the **pwdUseCheckModule** attribute.



See **slapo-ppolicy** for more information, but to sum up:

- enable ppolicy overlay in your database.
- define a default password policy in OpenLDAP configuration or use pwdPolicySubentry attribute to point to the given policy.

This example show the activation for a **slapd.conf** file for OpenLDAP 2.6
(see **slapd-config** and **slapo-ppolicy** for more information for
 **cn=config** configuration)

```
overlay ppolicy
ppolicy_default "cn=default,ou=policies,dc=my-domain,dc=com"
ppolicy_check_module /usr/local/openldap/libexec/openldap/ppm.so
#ppolicy_use_lockout   # for having more infos about the lockout
```

For OpenLDAP 2.5, you must remove **ppolicy_check_module** parameter as
it is managed in the password policy definition


# FEATURES

Here are the main features:

- 4 character classes are defined by default:
upper case, lower case, digits and special characters.

- more character classes can be defined, just write your own.

- passwords must match the amount of quality points.
A point is validated when at least m characters of the corresponding
character class are present in the password.

- passwords must have at least n of the corresponding character class
present, else they are rejected.

- passwords must have at the most x occurrences of characters from the
corresponding character class, else they are rejected.

- the three previous criteria are checked against any specific character class
defined.

- if a password contains any of the forbidden characters, then it is
rejected.

- if a password contains tokens from the RDN, then it is rejected.

- if a password contains tokens from defined attributes, then it is rejected.

- if a password does not pass cracklib check, then it is rejected.


# CONFIGURATION

Since OpenLDAP 2.5 version, ppm configuration is held in a binary
attribute of the password policy: **pwdCheckModuleArg**

The example file (**ETCDIR/ppm.example** by default) is to be
considered as an example configuration, to import in the **pwdCheckModuleArg**
attribute. It is also used for testing passwords with the test program
provided.

If for some reasons, any parameter is not found, it will be given its
default value.

Note: you can still compile ppm to use the configuration file, by enabling
**PPM_READ_FILE** in **ppm.h** (but this is deprecated now). If you decide to do so,
you can use the **PPM_CONFIG_FILE** environment variable for overloading the
configuration file path.

The syntax of a configuration line is:

```
parameter value [min] [minForPoint] [max]
```

with spaces being delimiters and Line Feed (LF) ending the line.

Parameter names **are** case sensitive.

Lines beginning by a **#** are considered as comments.

The default configuration is the following:

```
# minQuality parameter
# Format:
# minQuality [NUMBER]
# Description:
# One point is granted for each class for which MIN_FOR_POINT criteria is fulfilled.
# defines the minimum point numbers for the password to be accepted.
minQuality 3

# checkRDN parameter
# Format:
# checkRDN [0 | 1]
# Description:
# If set to 1, password must not contain a token from the RDN.
# Tokens are separated by the following delimiters : space tabulation _ - , ; £
checkRDN 0

# checkAttributes parameter
# Format:
# checkAttributes [ATTR1,ATTR2,...]
# Description:
# Password must not contain a token from the values in the given list of attributes
# Tokens are substrings of the values of the given attributes,
# delimited by: space tabulation _ - , ; @
# For example, if uid="the wonderful entry",
# password must not contain "the", nor "wonderful", nor "entry"
checkAttributes

# forbiddenChars parameter
# Format:
# forbiddenChars [CHARACTERS_FORBIDDEN]
# Description:
# Defines the forbidden characters list (no separator).
# If one of them is found in the password, then it is rejected.
forbiddenChars

# maxConsecutivePerClass parameter
# Format:
# maxConsecutivePerClass [NUMBER]
# Description:
# Defines the maximum number of consecutive character allowed for any class
maxConsecutivePerClass 0

# useCracklib parameter
# Format:
# useCracklib [0 | 1]
# Description:
# If set to 1, the password must pass the cracklib check
useCracklib 0

# cracklibDict parameter
# Format:
# cracklibDict [path_to_cracklib_dictionary]
# Description:
# directory+filename-prefix that your version of CrackLib will go hunting for
# For example, /var/pw_dict resolves as /var/pw_dict.pwd,
# /var/pw_dict.pwi and /var/pw_dict.hwm dictionary files
cracklibDict /var/cache/cracklib/cracklib_dict

# classes parameter
# Format:
# class-[CLASS_NAME] [CHARACTERS_DEFINING_CLASS] [MIN] [MIN_FOR_POINT] [MAX]
# Description:
# [CHARACTERS_DEFINING_CLASS]: characters defining the class (no separator)
# [MIN]: If at least [MIN] characters of this class is not found in the password, then it is rejected
# [MIN_FOR_POINT]: one point is granted if password contains at least [MIN_FOR_POINT] character numbers of this class
# [MAX]: if > [MAX] occurrences of characters from this class are found, then the password is rejected (0 means no maximum)
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1 0
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1 0
class-digit 0123456789 0 1 0
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1 0
```

# EXAMPLE

With this policy:
```
minQuality 4
forbiddenChars .?,
checkRDN 1
checkAttributes mail
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 5 0
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 12 0
class-digit 0123456789 0 1 0
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1 0
class-myClass :) 1 1 0
```

the password **ThereIsNoCowLevel)** is working, because:

- it has 4 character classes validated : upper, lower, special, and myClass
- it has no character among .?,
- it has at least one character among : or )

but it won't work for the user uid=John Cowlevel,ou=people,cn=example,cn=com,
because the token "Cowlevel" from his RDN exists in the password (case insensitive).

Also, it won't work for a mail attribute containing: "thereis@domain.com"
because the part "thereis" matches the password.


# LOGS

If a user password is rejected by **ppm**, the user will get this type of message:

Typical user message from ldappasswd(5):

```
  Result: Constraint violation (19)
  Additional info: Password for dn=\"%s\" does not pass required number of strength checks (2 of 3)
```

A more detailed message is written to the server log.

While evaluating a password change, you should observe something looking at this in the logs:

```
ppm: entry uid=jack.oneill,ou=people,dc=my-domain,dc=com
ppm: Reading pwdCheckModuleArg attribute
ppm: RAW configuration: minQuality 3#012checkRDN 0#012checkAttributes mail,uid#012forbiddenChars#012maxConsecutivePerClass 0#012useCracklib 0#012cracklibDict /var/cache/cracklib/cracklib_dict#012class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1 0#012class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1 0#012class-digit 0123456789 0 1 0#012class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1 0
ppm: Parsing pwdCheckModuleArg attribute
ppm: get line: minQuality 3
ppm: Param = minQuality, value = 3, min = (null), minForPoint= (null)
ppm:  Accepted replaced value: 3
ppm: get line: checkRDN 0
ppm: Param = checkRDN, value = 0, min = (null), minForPoint= (null)
ppm:  Accepted replaced value: 0
ppm: get line: checkAttributes mail,uid
ppm: Param = checkAttributes, value = mail,uid, min = (null), minForPoint= (null)
ppm:  Accepted replaced value: mail,uid
ppm: get line: forbiddenChars
ppm: No value, goto next parameter
ppm: get line: maxConsecutivePerClass 0
ppm: Param = maxConsecutivePerClass, value = 0, min = (null), minForPoint= (null)
ppm:  Accepted replaced value: 0
ppm: get line: useCracklib 0
ppm: Param = useCracklib, value = 0, min = (null), minForPoint= (null)
ppm:  Accepted replaced value: 0
ppm: get line: cracklibDict /var/cache/cracklib/cracklib_dict
ppm: Param = cracklibDict, value = /var/cache/cracklib/cracklib_dict, min = (null), minForPoint= (null)
ppm:  Accepted replaced value: /var/cache/cracklib/cracklib_dict
ppm: get line: class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1 0
ppm: Param = class-upperCase, value = ABCDEFGHIJKLMNOPQRSTUVWXYZ, min = 0, minForPoint = 1, max = 0
ppm:  Accepted replaced value: ABCDEFGHIJKLMNOPQRSTUVWXYZ
ppm: get line: class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1 0
ppm: Param = class-lowerCase, value = abcdefghijklmnopqrstuvwxyz, min = 0, minForPoint = 1, max = 0
ppm:  Accepted replaced value: abcdefghijklmnopqrstuvwxyz
ppm: get line: class-digit 0123456789 0 1 0
ppm: Param = class-digit, value = 0123456789, min = 0, minForPoint = 1, max = 0
ppm:  Accepted replaced value: 0123456789
ppm: get line: class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1 0
ppm: Param = class-special, value = <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+, min = 0, minForPoint = 1, max = 0
ppm:  Accepted replaced value: <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+
ppm: 1 point granted for class class-upperCase
ppm: 1 point granted for class class-lowerCase
ppm: Reallocating szErrStr from 64 to 179
check_password_quality: module error: (/usr/local/openldap/libexec/openldap/ppm.so) Password for dn="uid=jack.oneill,ou=people,dc=my-domain,dc=com" does not pass required number of strength checks (2 of 3).[1]
```


# TESTS

There is a unit test script: **unit_tests.sh** that illustrates checking some passwords.

It is possible to test one particular password using directly the test program:

```
cd /usr/local/lib
LD_LIBRARY_PATH=. ./ppm_test "uid=test,ou=users,dc=my-domain,dc=com" "my_password" "/usr/local/etc/openldap/ppm.example" && echo OK
```


# FILES

**ETCDIR/ppm.example**

> example of ppm configuration to be inserted in **pwdCheckModuleArg** attribute of given password policy

**ppm.so**

> ppm library, loaded by the **pwdCheckModule** attribute of given password policy (OpenLDAP 2.5)
> or by the **ppolicy_check_module** / **olcPPolicyCheckModule** parameters of the ppolicy overlay (OpenLDAP 2.6)

**ppm_test**

> small test program for checking password in a command-line


# SEE ALSO

**slapo-ppolicy**(5), **slapd-config**(5), **slapd.conf**(5)

# ACKNOWLEDGEMENTS

This module was developed in 2014-2022 by David Coutadeur.
