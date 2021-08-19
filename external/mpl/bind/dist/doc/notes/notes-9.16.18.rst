.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, you can obtain one at https://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

Notes for BIND 9.16.18
----------------------

Bug Fixes
~~~~~~~~~

- When preparing DNS responses, ``named`` could replace the letters
  ``W`` (uppercase) and ``w`` (lowercase) with ``\000``. This has been
  fixed. :gl:`#2779`

- The configuration-checking code failed to account for the inheritance
  rules of the ``key-directory`` option. As a side effect of this flaw,
  the code detecting ``key-directory`` conflicts for zones using KASP
  incorrectly reported unique key directories as being reused. This has
  been fixed. :gl:`#2778`
