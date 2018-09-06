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

<!--
  - Whack &mdash; into something that won't choke LaTeX.
  - There's probably a better way to do this, but this will work for now.
  -->

<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:db="http://docbook.org/ns/docbook">

  <xsl:variable name="mdash" select="'&#8212;'"/>

  <xsl:template name="fix-mdash" match="text()[contains(., '&#8212;')]">
    <xsl:param name="s" select="."/>
    <xsl:choose>
      <xsl:when test="contains($s, $mdash)">
        <xsl:value-of select="substring-before($s, $mdash)"/>
	<xsl:text>---</xsl:text>
        <xsl:call-template name="fix-mdash">
	  <xsl:with-param name="s" select="substring-after($s, $mdash)"/>
	</xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$s"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/">
    <xsl:apply-templates/>
  </xsl:template>

</xsl:stylesheet>
