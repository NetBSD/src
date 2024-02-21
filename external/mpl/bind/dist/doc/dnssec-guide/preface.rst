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

Preface
-------

.. _preface_organization:

Organization
~~~~~~~~~~~~

This document provides introductory information on how DNSSEC works, how
to configure BIND 9 to support some common DNSSEC features, and
some basic troubleshooting tips. The chapters are organized as follows:

:ref:`dnssec_guide_introduction` covers the intended audience for this
document, assumed background knowledge, and a basic introduction to the
topic of DNSSEC.

:ref:`getting_started` covers various requirements
before implementing DNSSEC, such as software versions, hardware
capacity, network requirements, and security changes.

:ref:`dnssec_validation` walks through setting up a validating
resolver, and gives both more information on the validation process and
some examples of tools to verify that the resolver is properly validating
answers.

:ref:`dnssec_signing` explains how to set up a basic signed
authoritative zone, details the relationship between a child and a parent zone,
and discusses ongoing maintenance tasks.

:ref:`dnssec_troubleshooting` provides some tips on how to analyze
and diagnose DNSSEC-related problems.

:ref:`dnssec_advanced_discussions` covers several topics, including key
generation, key storage, key management, NSEC and NSEC3, and some
disadvantages of DNSSEC.

:ref:`dnssec_recipes` provides several working examples of common DNSSEC
solutions, with step-by-step details.

:ref:`dnssec_commonly_asked_questions` lists some commonly asked
questions and answers about DNSSEC.

.. _preface_acknowledgement:

Acknowledgements
~~~~~~~~~~~~~~~~

This document was originally authored by Josh Kuo of `DeepDive
Networking <https://www.deepdivenetworking.com/>`__. He can be reached
at josh.kuo@gmail.com.

Thanks to the following individuals (in no particular order) who have
helped in completing this document: Jeremy C. Reed, Heidi Schempf,
Stephen Morris, Jeff Osborn, Vicky Risk, Jim Martin, Evan Hunt, Mark
Andrews, Michael McNally, Kelli Blucher, Chuck Aurora, Francis Dupont,
Rob Nagy, Ray Bellis, Matthijs Mekking, and Suzanne Goldlust.

Special thanks goes to Cricket Liu and Matt Larson for their
selflessness in knowledge sharing.

Thanks to all the reviewers and contributors, including John Allen, Jim
Young, Tony Finch, Timothe Litt, and Dr. Jeffry A. Spain.

The sections on key rollover and key timing metadata borrowed heavily
from the Internet Engineering Task Force draft titled "DNSSEC Key Timing
Considerations" by S. Morris, J. Ihren, J. Dickinson, and W. Mekking,
subsequently published as :rfc:`7583`.

Icons made by `Freepik <https://www.freepik.com/>`__ and
`SimpleIcon <https://www.simpleicon.com/>`__ from
`Flaticon <https://www.flaticon.com/>`__, licensed under `Creative Commons BY
3.0 <https://creativecommons.org/licenses/by/3.0/>`__.
