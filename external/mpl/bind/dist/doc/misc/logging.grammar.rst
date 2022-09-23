.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

::

  logging {
  	category <string> { <string>; ... };
  	channel <string> {
  		buffered <boolean>;
  		file <quoted_string> [ versions ( unlimited | <integer> ) ]
  		    [ size <size> ] [ suffix ( increment | timestamp ) ];
  		null;
  		print-category <boolean>;
  		print-severity <boolean>;
  		print-time ( iso8601 | iso8601-utc | local | <boolean> );
  		severity <log_severity>;
  		stderr;
  		syslog [ <syslog_facility> ];
  	};
  };
