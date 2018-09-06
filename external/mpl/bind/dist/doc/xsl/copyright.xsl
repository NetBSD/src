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

<!-- Generate ISC copyright comments from Docbook copyright metadata. -->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xi="http://www.w3.org/2001/XInclude"
  xmlns:db="http://docbook.org/ns/docbook">

  <xsl:template name="isc.copyright.format">
    <xsl:param name="text"/>
    <xsl:value-of select="$isc.copyright.leader"/>
    <xsl:value-of select="normalize-space(substring-before($text, '&#10;'))"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:variable name="rest" select="substring-after($text, '&#10;')"/>
    <xsl:if test="translate($rest, '&#9;&#32;', '')">
      <xsl:call-template name="isc.copyright.format">
        <xsl:with-param name="text" select="$rest"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:variable name="isc.copyright.text">
    <xsl:text>
       This Source Code Form is subject to the terms of the Mozilla Public
       License, v. 2.0. If a copy of the MPL was not distributed with this
       file, You can obtain one at http://mozilla.org/MPL/2.0/.
    </xsl:text>
  </xsl:variable>

  <xsl:variable name="isc.copyright">
    <xsl:call-template name="isc.copyright.format">
      <xsl:with-param name="text">
        <xsl:for-each select="book/info/copyright | refentry/docinfo/copyright">
	  <xsl:text>Copyright (C) </xsl:text>
	  <xsl:call-template name="copyright.years">
	    <xsl:with-param name="years" select="year"/>
	  </xsl:call-template>
	  <xsl:text> </xsl:text>
	  <xsl:value-of select="holder"/>
          <xsl:value-of select="$isc.copyright.breakline"/>
	  <xsl:text>&#10;</xsl:text>
	</xsl:for-each>
	<xsl:value-of select="$isc.copyright.text"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:variable>
</xsl:stylesheet>

<!--
  - Local variables:
  - mode: sgml
  - End:
 -->
