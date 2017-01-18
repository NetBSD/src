<!--
 - Copyright (C) 2005, 2007, 2009, 2015  Internet Systems Consortium, Inc. ("ISC")
 -
 - Permission to use, copy, modify, and/or distribute this software for any
 - purpose with or without fee is hereby granted, provided that the above
 - copyright notice and this permission notice appear in all copies.
 -
 - THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 - REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 - AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 - INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 - LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 - OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 - PERFORMANCE OF THIS SOFTWARE.
-->

<!-- Id -->

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
      Permission to use, copy, modify, and/or distribute this software for any
      purpose with or without fee is hereby granted, provided that the above
      copyright notice and this permission notice appear in all copies.

      THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
      REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
      AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
      INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
      LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
      OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
      PERFORMANCE OF THIS SOFTWARE.
    </xsl:text>
  </xsl:variable>

  <xsl:variable name="isc.copyright">
    <xsl:call-template name="isc.copyright.format">
      <xsl:with-param name="text">
        <xsl:for-each select="db:book/db:info/db:copyright | db:refentry/db:docinfo/db:copyright">
	  <xsl:text>Copyright (C) </xsl:text>
	  <xsl:call-template name="copyright.years">
	    <xsl:with-param name="years" select="db:year"/>
	  </xsl:call-template>
	  <xsl:text> </xsl:text>
	  <xsl:value-of select="db:holder"/>
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
