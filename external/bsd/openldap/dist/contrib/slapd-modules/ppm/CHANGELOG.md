# CHANGELOG

* 2022-05-17 David Coutadeur <david.coutadeur@gmail.com>
  implement a maximum number of characters for each class #18
  upgrade documentation for new olcPPolicyCheckModule in OpenLDAP 2.6 #30
  Make one unique code of development for 2.5 and 2.6 OpenLDAP versions #35
  fix segmentation fault in ppm_test #36
  various minor fixes and optimizations
  Version 2.2
* 2022-03-22 David Coutadeur <david.coutadeur@gmail.com>
  Reject password if it contains tokens from an attribute of the LDAP entry #17 
  Version 2.1
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
