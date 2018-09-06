<!--
 - Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 -
 - This Source Code Form is subject to the terms of the Mozilla Public
 - License, v. 2.0. If a copy of the MPL was not distributed with this
 - file, You can obtain one at http://mozilla.org/MPL/2.0/.
 -
 - See the COPYRIGHT file distributed with this work for additional
 - information regarding copyright ownership.
-->

<!-- Tweaks to Docbook-XSL HTML for producing flat ASCII text. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
		xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0">

  <!-- Import our Docbook HTML stuff -->
  <xsl:import href="isc-docbook-html.xsl"/>

  <!-- Disable tables of contents (for now - tweak as needed) -->
  <xsl:param name="generate.toc"/>

  <!-- Voodoo to read i18n/l10n overrides directly from this stylesheet -->
  <xsl:param name="local.l10n.xml" select="document('')"/>

  <!-- Customize Docbook-XSL i18n/l10n mappings. -->
  <l:i18n>
    <l:l10n language="en" english-language-name="English">

      <!-- Please use plain old ASCII quotes -->
      <l:dingbat key="startquote" text='&quot;'/>
      <l:dingbat key="endquote"   text='&quot;'/>

    </l:l10n>
  </l:i18n>

</xsl:stylesheet>

<!--
  - Local variables:
  - mode: sgml
  - End:
 -->
