<?xml version="1.0" encoding="UTF-8"?>
<!--
 - Copyright (C) 2006-2008  Internet Systems Consortium, Inc. ("ISC")
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

<!-- Id: bind9.xsl,v 1.13.130.4 2008/04/09 22:49:37 jinmei Exp -->

<xsl:stylesheet version="1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
   xmlns="http://www.w3.org/1999/xhtml">
  <xsl:template match="isc/bind/statistics">
    <html>
      <head>
        <style type="text/css">
body {
	font-family: sans-serif;
	background-color: #ffffff;
	color: #000000;
}

table {
	border-collapse: collapse;
}

tr.rowh {
	text-align: center;
	border: 1px solid #000000;
	background-color: #8080ff;
	color: #ffffff;
}

tr.row {
	text-align: right;
	border: 1px solid #000000;
	background-color: teal;
	color: #ffffff;
}

tr.lrow {
	text-align: left;
	border: 1px solid #000000;
	background-color: teal;
	color: #ffffff;
}

td, th {
	padding-right: 5px;
	padding-left: 5px;
}

.header {
	background-color: teal;
	color: #ffffff;
	padding: 4px;
}

.content {
	background-color: #ffffff;
	color: #000000;
	padding: 4px;
}

.item {
	padding: 4px;
	align: right;
}

.value {
	padding: 4px;
	font-weight: bold;
}
        </style>
        <title>BIND 9 Statistics</title>
      </head>
      <body>
        <div class="header">Bind 9 Configuration and Statistics</div>

	<br/>

	<table>
	  <tr class="rowh"><th colspan="2">Times</th></tr>
	  <tr class="lrow">
	    <td>boot-time</td>
	    <td><xsl:value-of select="server/boot-time"/></td>
	  </tr>
	  <tr class="lrow">
	    <td>current-time</td>
	    <td><xsl:value-of select="server/current-time"/></td>
	  </tr>
	</table>

	<br/>

	<table>
	  <tr class="rowh"><th colspan="2">Incoming Requests</th></tr>
	  <xsl:for-each select="server/requests/opcode">
	    <tr class="lrow">
	      <td><xsl:value-of select="name"/></td>
	      <td><xsl:value-of select="counter"/></td>
	    </tr>
	  </xsl:for-each>
	</table>

	<br/>

	<table>
	  <tr class="rowh"><th colspan="2">Incoming Queries</th></tr>
	  <xsl:for-each select="server/queries-in/rdtype">
	    <tr class="lrow">
	      <td><xsl:value-of select="name"/></td>
	      <td><xsl:value-of select="counter"/></td>
	    </tr>
	  </xsl:for-each>
	</table>

	<br/>

	<xsl:for-each select="views/view">
	  <table>
	    <tr class="rowh">
	      <th colspan="2">Outgoing Queries from View <xsl:value-of select="name"/></th>
	    </tr>
	    <xsl:for-each select="rdtype">
	      <tr class="lrow">
		<td><xsl:value-of select="name"/></td>
		<td><xsl:value-of select="counter"/></td>
	      </tr>
	    </xsl:for-each>
	  </table>
	  <br/>
	</xsl:for-each>

	<br/>

	<table>
	  <tr class="rowh"><th colspan="10">Server Statistics</th></tr>
          <tr class="rowh">
	    <!-- The ordering of the following items must be consistent
	    with dns_nsstatscounter_xxx -->
	    <th>Requestv4</th>
	    <th>Requestv6</th>
	    <th>ReqEdns0</th>
	    <th>ReqBadEDNSVer</th>
	    <th>ReqTSIG</th>
	    <th>ReqSIG0</th>
	    <th>ReqBadSIG</th>
	    <th>ReqTCP</th>
	    <th>AuthQryRej</th>
	    <th>RecQryRej</th>
	  </tr>
	  <tr class="lrow">
	    <td><xsl:value-of select="server/nsstats/Requestv4"/></td>
	    <td><xsl:value-of select="server/nsstats/Requestv6"/></td>
	    <td><xsl:value-of select="server/nsstats/ReqEdns0"/></td>
	    <td><xsl:value-of select="server/nsstats/ReqBadEDNSVer"/></td>
	    <td><xsl:value-of select="server/nsstats/ReqTSIG"/></td>
	    <td><xsl:value-of select="server/nsstats/ReqSIG0"/></td>
	    <td><xsl:value-of select="server/nsstats/ReqBadSIG"/></td>
	    <td><xsl:value-of select="server/nsstats/ReqTCP"/></td>
	    <td><xsl:value-of select="server/nsstats/AuthQryRej"/></td>
	    <td><xsl:value-of select="server/nsstats/RecQryRej"/></td>
	  </tr>
          <tr class="rowh">
	    <th>XfrRej</th>
	    <th>UpdateRej</th>
	    <th>Response</th>
	    <th>RespTruncated</th>
	    <th>RespEDNS0</th>
	    <th>RespTSIG</th>
	    <th>RespSIG0</th>
	    <th>QrySuccess</th>
	    <th>QryAuthAns</th>
	    <th>QryNoauthAns</th>
	  </tr>
	  <tr class="lrow">
	    <td><xsl:value-of select="server/nsstats/XfrRej"/></td>
	    <td><xsl:value-of select="server/nsstats/UpdateRej"/></td>
	    <td><xsl:value-of select="server/nsstats/Response"/></td>
	    <td><xsl:value-of select="server/nsstats/TruncatedResp"/></td>
	    <td><xsl:value-of select="server/nsstats/RespEDNS0"/></td>
	    <td><xsl:value-of select="server/nsstats/RespTSIG"/></td>
	    <td><xsl:value-of select="server/nsstats/RespSIG0"/></td>
	    <td><xsl:value-of select="server/nsstats/QrySuccess"/></td>
	    <td><xsl:value-of select="server/nsstats/QryAuthAns"/></td>
	    <td><xsl:value-of select="server/nsstats/QryNoauthAns"/></td>
	  </tr>
          <tr class="rowh">
	    <th>QryReferral</th>
	    <th>QryNxrrset</th>
	    <th>QrySERVFAIL</th>
	    <th>QryFORMERR</th>
	    <th>QryNXDOMAIN</th>
	    <th>QryRecursion</th>
	    <th>QryDuplicate</th>
	    <th>QryDropped</th>
	    <th>QryFailure</th>
	    <th>XfrReqDone</th>
	  </tr>
	  <tr class="lrow">
	    <td><xsl:value-of select="server/nsstats/QryReferral"/></td>
	    <td><xsl:value-of select="server/nsstats/QryNxrrset"/></td>
	    <td><xsl:value-of select="server/nsstats/QrySERVFAIL"/></td>
	    <td><xsl:value-of select="server/nsstats/QryFORMERR"/></td>
	    <td><xsl:value-of select="server/nsstats/QryNXDOMAIN"/></td>
	    <td><xsl:value-of select="server/nsstats/QryRecursion"/></td>
	    <td><xsl:value-of select="server/nsstats/QryDuplicate"/></td>
	    <td><xsl:value-of select="server/nsstats/QryDropped"/></td>
	    <td><xsl:value-of select="server/nsstats/QryFailure"/></td>
	    <td><xsl:value-of select="server/nsstats/XfrReqDone"/></td>
	  </tr>
          <tr class="rowh">
	    <th>UpdateReqFwd</th>
	    <th>UpdateRespFwd</th>
	    <th>UpdateFwdFail</th>
	    <th>UpdateDone</th>
	    <th>UpdateFail</th>
	    <th>UpdateBadPrereq</th>
	    <th>RespMismatch</th>
	    <th />
	    <th />
	    <th />
	  </tr>
	  <tr class="lrow">
	    <td><xsl:value-of select="server/nsstats/UpdateReqFwd"/></td>
	    <td><xsl:value-of select="server/nsstats/UpdateRespFwd"/></td>
	    <td><xsl:value-of select="server/nsstats/UpdateFwdFail"/></td>
	    <td><xsl:value-of select="server/nsstats/UpdateDone"/></td>
	    <td><xsl:value-of select="server/nsstats/UpdateFail"/></td>
	    <td><xsl:value-of select="server/nsstats/UpdateBadPrereq"/></td>
	    <td><xsl:value-of select="server/resstats/Mismatch"/></td>
	    <td />
	    <td />
	    <td />
	  </tr>
	</table>

	<br/>

	<table>
	  <tr class="rowh"><th colspan="10">Zone Maintenance Statistics</th></tr>
          <tr class="rowh">
	    <!-- The ordering of the following items must be consistent
	    with dns_zonestatscounter_xxx -->
	    <th>NotifyOutv4</th>
	    <th>NotifyOutv6</th>
	    <th>NotifyInv4</th>
	    <th>NotifyInv6</th>
	    <th>NotifyRej</th>
	    <th>SOAOutv4</th>
	    <th>SOAOutv6</th>
	    <th>AXFRReqv4</th>
	    <th>AXFRReqv6</th>
	    <th>IXFRReqv4</th>
	  </tr>
	  <tr class="lrow">
	    <td><xsl:value-of select="server/zonestats/NotifyOutv4"/></td>
	    <td><xsl:value-of select="server/zonestats/NotifyOutv6"/></td>
	    <td><xsl:value-of select="server/zonestats/NotifyInv4"/></td>
	    <td><xsl:value-of select="server/zonestats/NotifyInv6"/></td>
	    <td><xsl:value-of select="server/zonestats/NotifyRej"/></td>
	    <td><xsl:value-of select="server/zonestats/SOAOutv4"/></td>
	    <td><xsl:value-of select="server/zonestats/SOAOutv6"/></td>
	    <td><xsl:value-of select="server/zonestats/AXFRReqv4"/></td>
	    <td><xsl:value-of select="server/zonestats/AXFRReqv6"/></td>
	    <td><xsl:value-of select="server/zonestats/IXFRReqv4"/></td>
	  </tr>
	  <tr class="rowh">
	    <th>IXFRReqv6</th>
	    <th>XfrSuccess</th>
	    <th>XfrFail</th>
	    <th/>
	    <th/>
	    <th/>
	    <th/>
	    <th/>
	    <th/>
	    <th/>
	  </tr>
	  <tr class="lrow">
	    <td><xsl:value-of select="server/zonestats/IXFRReqv6"/></td>
	    <td><xsl:value-of select="server/zonestats/XfrSuccess"/></td>
	    <td><xsl:value-of select="server/zonestats/XfrFail"/></td>
	    <td/>
	    <td/>
	    <td/>
	    <td/>
	    <td/>
	    <td/>
	    <td/>
	  </tr>
	</table>

	<br/>

	<xsl:for-each select="views/view">
	  <table>
	    <tr class="rowh">
	      <th colspan="10">Resolver Statistics for View <xsl:value-of select="name"/></th>
	    </tr>
	    <tr class="rowh">
	    <!-- The ordering of the following items must be consistent
	    with dns_resstatscounter_xxx -->
	      <th>Queryv4</th>
	      <th>Queryv6</th>
	      <th>Responsev4</th>
	      <th>Responsev6</th>
	      <th>NXDOMAIN</th>
	      <th>SERVFAIL</th>
	      <th>FORMERR</th>
	      <th>OtherError</th>
	      <th>EDNS0Fail</th>
	      <!-- this counter is not applicable to per-view stat,
	      but keep it for generating the description table used in
	      the statschannel.c.
		  <th>Mismatch</th>  -->
	      <th>Truncated</th>
	    </tr>
	    <tr class="lrow">
	      <td><xsl:value-of select="resstats/Queryv4"/></td>
	      <td><xsl:value-of select="resstats/Queryv6"/></td>
	      <td><xsl:value-of select="resstats/Responsev4"/></td>
	      <td><xsl:value-of select="resstats/Responsev6"/></td>
	      <td><xsl:value-of select="resstats/NXDOMAIN"/></td>
	      <td><xsl:value-of select="resstats/SERVFAIL"/></td>
	      <td><xsl:value-of select="resstats/FORMERR"/></td>
	      <td><xsl:value-of select="resstats/OtherError"/></td>
	      <td><xsl:value-of select="resstats/EDNS0Fail"/></td>
	      <!--  <td><xsl:value-of select="resstats/Mismatch"/></td>  -->
	      <td><xsl:value-of select="resstats/Truncated"/></td>
	    </tr>
	    <tr class="rowh">
	      <th>Lame</th>
	      <th>Retry</th>
	      <th>GlueFetchv4</th>
	      <th>GlueFetchv6</th>
	      <th>GlueFetchv4Fail</th>
	      <th>GlueFetchv6Fail</th>
	      <th>ValAttempt</th>
	      <th>ValOk</th>
	      <th>ValNegOk</th>
	      <th>ValFail</th>
	    </tr>
	    <tr class="lrow">
	      <td><xsl:value-of select="resstats/Lame"/></td>
	      <td><xsl:value-of select="resstats/Retry"/></td>
	      <td><xsl:value-of select="resstats/GlueFetchv4"/></td>
	      <td><xsl:value-of select="resstats/GlueFetchv6"/></td>
	      <td><xsl:value-of select="resstats/GlueFetchv4Fail"/></td>
	      <td><xsl:value-of select="resstats/GlueFetchv6Fail"/></td>
	      <td><xsl:value-of select="resstats/ValAttempt"/></td>
	      <td><xsl:value-of select="resstats/ValOk"/></td>
	      <td><xsl:value-of select="resstats/ValNegOk"/></td>
	      <td><xsl:value-of select="resstats/ValFail"/></td>
	    </tr>
          </table>
	  <br/>
        </xsl:for-each>

	<br/>

	<xsl:for-each select="views/view">
	  <table>
	    <tr class="rowh">
	      <th colspan="2">Cache DB RRsets for View <xsl:value-of select="name"/></th>
	    </tr>
	    <xsl:for-each select="cache/rrset">
	      <tr class="lrow">
		<td><xsl:value-of select="name"/></td>
		<td><xsl:value-of select="counter"/></td>
	      </tr>
	    </xsl:for-each>
	  </table>
	  <br/>
	</xsl:for-each>

	<br/>

        <xsl:for-each select="views/view">
          <table>
            <tr class="rowh">
              <th colspan="10">Zones for View <xsl:value-of select="name"/></th>
            </tr>
            <tr class="rowh">
              <th>Name</th>
              <th>Class</th>
              <th>Serial</th>
              <th>Success</th>
              <th>Referral</th>
              <th>NXRRSET</th>
              <th>NXDOMAIN</th>
              <th>Failure</th>
	      <th>XfrReqDone</th>
	      <th>XfrRej</th>
            </tr>
            <xsl:for-each select="zones/zone">
              <tr class="lrow">
                <td>
                  <xsl:value-of select="name"/>
                </td>
                <td>
                  <xsl:value-of select="rdataclass"/>
                </td>
                <td>
                  <xsl:value-of select="serial"/>
                </td>
                <td>
                  <xsl:value-of select="counters/QrySuccess"/>
                </td>
                <td>
                  <xsl:value-of select="counters/QryReferral"/>
                </td>
                <td>
                  <xsl:value-of select="counters/QryNxrrset"/>
                </td>
                <td>
                  <xsl:value-of select="counters/QryNXDOMAIN"/>
                </td>
                <td>
                  <xsl:value-of select="counters/QryFailure"/>
                </td>
                <td>
                  <xsl:value-of select="counters/XfrReqDone"/>
                </td>
                <td>
                  <xsl:value-of select="counters/XfrRej"/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:for-each>

        <br/>

        <table>
          <tr class="rowh">
            <th colspan="7">Network Status</th>
          </tr>
          <tr class="rowh">
            <th>ID</th>
	    <th>Name</th>
            <th>Type</th>
            <th>References</th>
            <th>LocalAddress</th>
            <th>PeerAddress</th>
            <th>State</th>
          </tr>
          <xsl:for-each select="socketmgr/sockets/socket">
            <tr class="lrow">
              <td>
                <xsl:value-of select="id"/>
              </td>
              <td>
                <xsl:value-of select="name"/>
              </td>
              <td>
                <xsl:value-of select="type"/>
              </td>
              <td>
                <xsl:value-of select="references"/>
              </td>
              <td>
                <xsl:value-of select="local-address"/>
              </td>
              <td>
                <xsl:value-of select="peer-address"/>
              </td>
              <td>
                <xsl:for-each select="states">
                  <xsl:value-of select="."/>
                </xsl:for-each>
              </td>
            </tr>
          </xsl:for-each>
        </table>
        <br/>
        <table>
          <tr class="rowh">
            <th colspan="2">Task Manager Configuration</th>
          </tr>
          <tr class="lrow">
            <td>Thread-Model</td>
            <td>
              <xsl:value-of select="taskmgr/thread-model/type"/>
            </td>
          </tr>
          <tr class="lrow">
            <td>Worker Threads</td>
            <td>
              <xsl:value-of select="taskmgr/thread-model/worker-threads"/>
            </td>
          </tr>
          <tr class="lrow">
            <td>Default Quantum</td>
            <td>
              <xsl:value-of select="taskmgr/thread-model/default-quantum"/>
            </td>
          </tr>
          <tr class="lrow">
            <td>Tasks Running</td>
            <td>
              <xsl:value-of select="taskmgr/thread-model/tasks-running"/>
            </td>
          </tr>
        </table>
        <br/>
        <table>
          <tr class="rowh">
            <th colspan="5">Tasks</th>
          </tr>
          <tr class="rowh">
            <th>ID</th>
            <th>Name</th>
            <th>References</th>
            <th>State</th>
            <th>Quantum</th>
          </tr>
          <xsl:for-each select="taskmgr/tasks/task">
            <tr class="lrow">
              <td>
                <xsl:value-of select="id"/>
              </td>
              <td>
                <xsl:value-of select="name"/>
              </td>
              <td>
                <xsl:value-of select="references"/>
              </td>
              <td>
                <xsl:value-of select="state"/>
              </td>
              <td>
                <xsl:value-of select="quantum"/>
              </td>
            </tr>
          </xsl:for-each>
        </table>
	<br />
	<table>
          <tr class="rowh">
            <th colspan="4">Memory Usage Summary</th>
          </tr>
	  <xsl:for-each select="memory/summary/*">
	    <tr class="lrow">
	      <td><xsl:value-of select="name()"/></td>
	      <td><xsl:value-of select="."/></td>
	    </tr>
	  </xsl:for-each>
	</table>
	<br />
	<table>
          <tr class="rowh">
            <th colspan="10">Memory Contexts</th>
          </tr>
	  <tr class="rowh">
	    <th>ID</th>
	    <th>Name</th>
	    <th>References</th>
	    <th>TotalUse</th>
	    <th>InUse</th>
	    <th>MaxUse</th>
	    <th>BlockSize</th>
	    <th>Pools</th>
	    <th>HiWater</th>
	    <th>LoWater</th>
	  </tr>
	  <xsl:for-each select="memory/contexts/context">
	    <tr class="lrow">
	      <td>
		<xsl:value-of select="id"/>
	      </td>
              <td>
                <xsl:value-of select="name"/>
              </td>
	      <td>
		<xsl:value-of select="references"/>
	      </td>
	      <td>
		<xsl:value-of select="total"/>
	      </td>
	      <td>
		<xsl:value-of select="inuse"/>
	      </td>
	      <td>
		<xsl:value-of select="maxinuse"/>
	      </td>
	      <td>
		<xsl:value-of select="blocksize"/>
	      </td>
	      <td>
		<xsl:value-of select="pools"/>
	      </td>
	      <td>
		<xsl:value-of select="hiwater"/>
	      </td>
	      <td>
		<xsl:value-of select="lowater"/>
	      </td>
	    </tr>
	  </xsl:for-each>
	</table>

      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
