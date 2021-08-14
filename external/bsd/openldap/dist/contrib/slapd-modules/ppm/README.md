
ppm.c - OpenLDAP password policy module

version 2.0

ppm.c is an OpenLDAP module for checking password quality when they are modified.
Passwords are checked against the presence or absence of certain character classes.

This module is used as an extension of the OpenLDAP password policy controls,
see slapo-ppolicy(5) section pwdCheckModule.

contributions
-------------

* 2014 - 2021 - David Coutadeur <david.coutadeur@gmail.com> - maintainer
* 2015 - Daly Chikhaoui - Janua <dchikhaoui@janua.fr> - contribution on RDN checks
* 2017 - tdb - Tim Bishop - contribution on some compilation improvements


INSTALLATION
------------

See INSTALL file


USAGE
-----

Create a password policy entry and indicate the path of the ppm.so library
and the content of the desired policy.
Use a base64 tool to code / decode the content of the policy stored into
pwdCheckModuleArg. Here is an example:

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
pwdCheckModule: /usr/local/lib/ppm.so
pwdCheckModuleArg:: bWluUXVhbGl0eSAzCmNoZWNrUkROIDAKZm9yYmlkZGVuQ2hhcnMKbWF4Q29uc2VjdXRpdmVQZXJDbGFzcyAwCnVzZUNyYWNrbGliIDAKY3JhY2tsaWJEaWN0IC92YXIvY2FjaGUvY3JhY2tsaWIvY3JhY2tsaWJfZGljdApjbGFzcy11cHBlckNhc2UgQUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVogMCAxCmNsYXNzLWxvd2VyQ2FzZSBhYmNkZWZnaGlqa2xtbm9wcXJzdHV2d3h5eiAwIDEKY2xhc3MtZGlnaXQgMDEyMzQ1Njc4OSAwIDEKY2xhc3Mtc3BlY2lhbCA8Piw/Oy46LyHCp8O5JSrCtV7CqCTCo8KyJsOpfiIjJ3soWy18w6hgX1zDp17DoEApXcKwPX0rIDAgMQ==
```


See slapo-ppolicy for more information, but to sum up:
- enable ppolicy overlay in your database.
This example show the activation for a slapd.conf file
(see slapd-config and slapo-ppolicy for more information for
 cn=config configuration)

```
overlay ppolicy
ppolicy_default "cn=default,ou=policies,dc=my-domain,dc=com"
#ppolicy_use_lockout   # for having more infos about the lockout
```

- define a default password policy in OpenLDAP configuration or
use pwdPolicySubentry attribute to point to the given policy.




Password checks
---------------

- 4 character classes are defined by default:
upper case, lower case, digits and special characters.

- more character classes can be defined, just write your own.

- passwords must match the amount of quality points.
A point is validated when at least m characters of the corresponding
character class are present in the password.

- passwords must have at least n of the corresponding character class
present, else they are rejected.

- the two previous criteria are checked against any specific character class
defined.

- if a password contains any of the forbidden characters, then it is
rejected.

- if a password contains tokens from the RDN, then it is rejected.

- if a password is too long, it can be rejected.

- if a password does not pass cracklib check, it can be rejected.


Configuration
-------------

Since OpenLDAP 2.5 version, ppm configuration is held in a binary
attribute of the password policy: pwdCheckModuleArg
The example file (/etc/openldap/ppm.example by default) is to be
considered as an example configuration, to import in the pwdCheckModuleArg
attribute. It is also used for testing passwords with the test program
provided.
If for some reasons, any parameter is not found, it will be given its
default value.

Note: you can still compile ppm to use the configuration file, by enabling
PPM_READ_FILE in ppm.h (but this is deprecated now). If you decide to do so,
you can use the PPM_CONFIG_FILE environment variable for overloading the
configuration file path.

The syntax of a configuration line is:
parameter value [min] [minForPoint]

with spaces being delimiters and Line Feed (LF) ending the line.
Parameter names ARE case sensitive.

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
# class-[CLASS_NAME] [CHARACTERS_DEFINING_CLASS] [MIN] [MIN_FOR_POINT]
# Description:
# [CHARACTERS_DEFINING_CLASS]: characters defining the class (no separator)
# [MIN]: If at least [MIN] characters of this class is not found in the password, then it is rejected
# [MIN_FOR_POINT]: one point is granted if password contains at least [MIN_FOR_POINT] character numbers of this class
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1
class-digit 0123456789 0 1
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1
```

Example
-------

With this policy:
```
minQuality 4
forbiddenChars .?,
checkRDN 1
class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 5
class-lowerCase abcdefghijklmnopqrstuvwxyz 0 12
class-digit 0123456789 0 1
class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1
class-myClass :) 1 1``
```

the password

ThereIsNoCowLevel)

is working, because,
- it has 4 character classes validated : upper, lower, special, and myClass
- it has no character among .?,
- it has at least one character among : or )

but it won't work for the user uid=John Cowlevel,ou=people,cn=example,cn=com,
because the token "Cowlevel" from his RDN exists in the password (case insensitive).


Logs
----
If a user password is rejected by ppm, the user will get this type of message:

Typical user message from ldappasswd(5):
  Result: Constraint violation (19)
  Additional info: Password for dn=\"%s\" does not pass required number of strength checks (2 of 3)

A more detailed message is written to the server log.

Server log:

```
Feb 26 14:46:10 debian-10-64 slapd[1981]: conn=1000 op=16 MOD dn="uid=user,ou=persons,dc=my-domain,dc=com"
Feb 26 14:46:10 debian-10-64 slapd[1981]: conn=1000 op=16 MOD attr=userPassword
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: entry uid=user,ou=persons,dc=my-domain,dc=com
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Reading pwdCheckModuleArg attribute
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: RAW configuration: # minQuality parameter#012# Format:#012# minQuality [NUMBER]#012# Description:#012# One point is granted for each class for which MIN_FOR_POINT criteria is fulfilled.#012# defines the minimum point numbers for the password to be accepted.#012minQuality 3#012#012# checkRDN parameter#012# Format:#012# checkRDN [0 | 1]#012# Description:#012# If set to 1, password must not contain a token from the RDN.#012# Tokens are separated by the following delimiters : space tabulation _ - , ; £#012checkRDN 0#012#012# forbiddenChars parameter#012# Format:#012# forbiddenChars [CHARACTERS_FORBIDDEN]#012# Description:#012# Defines the forbidden characters list (no separator).#012# If one of them is found in the password, then it is rejected.#012forbiddenChars#012#012# maxConsecutivePerClass parameter#012# Format:#012# maxConsecutivePerClass [NUMBER]#012# Description:#012# Defines the maximum number of consecutive character allowed for any class#012maxConsecutivePerClass 0#012#012# useCracklib parameter#012# Format:#012# useCracklib [0 | 1]#012# Description:#012# If set to 1, the password must pass the cracklib check#012useCracklib 0#012#012# cracklibDict parameter#012# Format:#012# cracklibDict [path_to_cracklib_dictionary]#012# Description:#012# directory+filename-prefix that your version of CrackLib will go hunting for#012# For example, /var/pw_dict resolves as /var/pw_dict.pwd,#012# /var/pw_dict.pwi and /var/pw_dict.hwm dictionary files#012cracklibDict /var/cache/cracklib/cracklib_dict#012#012# classes parameter#012# Format:#012# class-[CLASS_NAME] [CHARACTERS_DEFINING_CLASS] [MIN] [MIN_FOR_POINT]#012# Description:#012# [CHARACTERS_DEFINING_CLASS]: characters defining the class (no separator)#012# [MIN]: If at least [MIN] characters of this class is not found in the password, then it is rejected#012# [MIN_FOR_POINT]: one point is granted if password contains at least [MIN_FOR_POINT] character numbers of this class#012class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1#012class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1#012class-digit 0123456789 0 1#012class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Parsing pwdCheckModuleArg attribute
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # minQuality parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # minQuality [NUMBER]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # One point is granted for each class for which MIN_FOR_POINT criteria is fulfilled.
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # defines the minimum point numbers for the password to be accepted.
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: minQuality 3
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = minQuality, value = 3, min = (null), minForPoint= (null)
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: 3
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # checkRDN parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # checkRDN [0 | 1]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # If set to 1, password must not contain a token from the RDN.
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Tokens are separated by the following delimiters : space tabulation _ - , ; £
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: checkRDN 0
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = checkRDN, value = 0, min = (null), minForPoint= (null)
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: 0
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # forbiddenChars parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # forbiddenChars [CHARACTERS_FORBIDDEN]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Defines the forbidden characters list (no separator).
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # If one of them is found in the password, then it is rejected.
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: forbiddenChars
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: No value, goto next parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # maxConsecutivePerClass parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # maxConsecutivePerClass [NUMBER]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Defines the maximum number of consecutive character allowed for any class
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: maxConsecutivePerClass 0
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = maxConsecutivePerClass, value = 0, min = (null), minForPoint= (null)
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: 0
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # useCracklib parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # useCracklib [0 | 1]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # If set to 1, the password must pass the cracklib check
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: useCracklib 0
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = useCracklib, value = 0, min = (null), minForPoint= (null)
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: 0
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # cracklibDict parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # cracklibDict [path_to_cracklib_dictionary]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # directory+filename-prefix that your version of CrackLib will go hunting for
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # For example, /var/pw_dict resolves as /var/pw_dict.pwd,
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # /var/pw_dict.pwi and /var/pw_dict.hwm dictionary files
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: cracklibDict /var/cache/cracklib/cracklib_dict
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = cracklibDict, value = /var/cache/cracklib/cracklib_dict, min = (null), minForPoint= (null)
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: /var/cache/cracklib/cracklib_dict
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # classes parameter
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Format:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # class-[CLASS_NAME] [CHARACTERS_DEFINING_CLASS] [MIN] [MIN_FOR_POINT]
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # Description:
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # [CHARACTERS_DEFINING_CLASS]: characters defining the class (no separator)
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # [MIN]: If at least [MIN] characters of this class is not found in the password, then it is rejected
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: # [MIN_FOR_POINT]: one point is granted if password contains at least [MIN_FOR_POINT] character numbers of this class
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: class-upperCase ABCDEFGHIJKLMNOPQRSTUVWXYZ 0 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = class-upperCase, value = ABCDEFGHIJKLMNOPQRSTUVWXYZ, min = 0, minForPoint= 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: ABCDEFGHIJKLMNOPQRSTUVWXYZ
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: class-lowerCase abcdefghijklmnopqrstuvwxyz 0 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = class-lowerCase, value = abcdefghijklmnopqrstuvwxyz, min = 0, minForPoint= 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: abcdefghijklmnopqrstuvwxyz
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: class-digit 0123456789 0 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = class-digit, value = 0123456789, min = 0, minForPoint= 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: 0123456789
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: get line: class-special <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+ 0 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Param = class-special, value = <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+, min = 0, minForPoint= 1
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm:  Accepted replaced value: <>,?;.:/!§ù%*µ^¨$£²&é~"#'{([-|è`_\ç^à@)]°=}+
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: 1 point granted for class class-lowerCase
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: 1 point granted for class class-digit
Feb 26 14:46:10 debian-10-64 slapd[1981]: ppm: Reallocating szErrStr from 64 to 173
Feb 26 14:46:10 debian-10-64 slapd[1981]: check_password_quality: module error: (/usr/local/lib/ppm.so) Password for dn="uid=user,ou=persons,dc=my-domain,dc=com" does not pass required number of strength checks (2 of 3).[1]
Feb 26 14:46:10 debian-10-64 slapd[1981]: conn=1000 op=16 RESULT tag=103 err=19 qtime=0.000020 etime=0.001496 text=Password for dn="uid=user,ou=persons,dc=my-domain,dc=com" does not pass required number of strength checks (2 of 3)
```


Tests
-----

There is a unit test script: "unit_tests.sh" that illustrates checking some passwords.
It is possible to test one particular password using directly the test program:

```
cd /usr/local/lib
LD_LIBRARY_PATH=. ./ppm_test "uid=test,ou=users,dc=my-domain,dc=com" "my_password" "/usr/local/etc/openldap/ppm.example" && echo OK
```



HISTORY
-------

* 2021-02-23 David Coutadeur <david.coutadeur@gmail.com>
  remove maxLength attribute (#21)
  adapt the readme and documentation of ppm (#22)
  prepare ppolicy10 in OpenLDAP 2.5 (#20, #23 and #24)
  add pwdCheckModuleArg feature
  Version 2.0
* 2019-08-20 David Coutadeur <david.coutadeur@gmail.com>
  adding debug symbols for ppm_test,
  improve tests with the possibility to add username,
  fix openldap crash when checkRDN=1 and username contains too short parts
  Version 1.8
* 2018-03-30 David Coutadeur <david.coutadeur@gmail.com>
  various minor improvements provided by Tim Bishop (tdb) (compilation, test program,
  imprvts in Makefile: new OLDAP_SOURCES variable pointing to OLDAP install. directory
  Version 1.7
* 2017-05-19 David Coutadeur <david.coutadeur@gmail.com>
  Adds cracklib support
  Readme adaptations and cleaning
  Version 1.6
* 2017-02-07 David Coutadeur <david.coutadeur@gmail.com>
  Adds maxConsecutivePerClass (idea from Trevor Vaughan / tvaughan@onyxpoint.com)
  Version 1.5
* 2016-08-22 David Coutadeur <david.coutadeur@gmail.com>
  Get config file from environment variable
  Version 1.4
* 2014-12-20 Daly Chikhaoui <dchikhaoui@janua.fr>
  Adding checkRDN parameter
  Version 1.3
* 2014-10-28 David Coutadeur <david.coutadeur@gmail.com>
  Adding maxLength parameter
  Version 1.2
* 2014-07-27 David Coutadeur <david.coutadeur@gmail.com>
  Changing the configuration file and the configuration data structure
  Version 1.1
* 2014-04-04 David Coutadeur <david.coutadeur@gmail.com>
  Version 1.0

