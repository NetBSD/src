<?xml version="1.0" encoding="UTF-8"?>
<!--
 - Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 -
 - SPDX-License-Identifier: MPL-2.0
 - This Source Code Form is subject to the terms of the Mozilla Public
 - License, v. 2.0. If a copy of the MPL was not distributed with this
 - file, you can obtain one at https://mozilla.org/MPL/2.0/.
 -
 - See the COPYRIGHT file distributed with this work for additional
 - information regarding copyright ownership.
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">
  <xsl:output method="html" indent="yes" version="4.0"/>
  <!-- the version number **below** must match version in bin/named/statschannel.c -->
  <!-- don't forget to update "/xml/v<STATS_XML_VERSION_MAJOR>" in the HTTP endpoints listed below -->
  <xsl:template match="statistics[@version=&quot;3.14&quot;]">
    <html>
      <head>
        <script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js"></script>
        <script type="text/javascript">
          $(function($) {
              var wid=0;
              $('table.zones').each(function(i) { if( $(this).width() > wid ) wid = $(this).width(); return true; });
              $('table.zones').css('min-width', wid );
              $("h2+table,h3+table,h4+table,h2+div,h3+div,h2+script,h3+script").prev().append(' <a class="tabletoggle" href="#" style="font-size:small">Show/Hide</a>');
              $(".tabletoggle").click(function(){
		 var n = $(this).closest("h2,h3,h4").next();
		 if (n.is("script")) { n = n.next(); }
	         if (n.is("div")) { n.toggleClass("hidden"); n = n.next(); }
		 if (n.is("table")) { n.toggleClass("hidden"); }
		 return false;
	      });
          });
        </script>

        <script type="text/javascript" src="https://www.google.com/jsapi"/>
        <script type="text/javascript">
          google.load("visualization", "1", {packages:["corechart"]});
          google.setOnLoadCallback(loadGraphs);

          var graphs=[];

          function drawChart(chart_title,target,style,data) {
            var data = google.visualization.arrayToDataTable(data);

            var options = {
              title: chart_title
            };

            var chart;
            if (style == "barchart") {
              chart = new google.visualization.BarChart(document.getElementById(target));
              chart.draw(data, options);
            } else if (style == "piechart") {
              chart = new google.visualization.PieChart(document.getElementById(target));
              chart.draw(data, options);
            }
          }

          function loadGraphs(){
            var g;

            while (g = graphs.shift()) {
              if (g.data.length > 1) {
                drawChart(g.title,g.target,g.style,g.data);
              }
            }
          }

          <xsl:if test="server/counters[@type=&quot;qtype&quot;]/counter[. &gt; 0]">
            // Server Incoming Query Types
            graphs.push({
              'title' : "Server Incoming Query Types",
              'target': 'chart_incoming_qtypes',
              'style': 'barchart',
              'data': [['Type','Counter'],<xsl:for-each select="server/counters[@type=&quot;qtype&quot;]/counter">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
            });
          </xsl:if>

          <xsl:if test="server/counters[@type=&quot;opcode&quot;]/counter[. &gt; 0]">
            // Server Incoming Requests by opcode
            graphs.push({
              'title' : "Server Incoming Requests by DNS Opcode",
              'target': 'chart_incoming_opcodes',
              'style': 'barchart',
              'data': [['Opcode','Counter'],<xsl:for-each select="server/counters[@type=&quot;opcode&quot;]/counter[. &gt; 0 or substring(@name,1,3) != 'RES']">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
            });
          </xsl:if>
        </script>

        <style type="text/css">
     body {
      font-family: sans-serif;
      background-color: #ffffff;
      color: #000000;
      font-size: 10pt;
     }

     .hidden{
      display: none;
     }

     .odd{
      background-color: #f0f0f0;
     }

     .even{
      background-color: #ffffff;
     }

     p.footer{
      font-style:italic;
      color: grey;
     }

     table {
      border-collapse: collapse;
      border: 1px solid grey;
     }

     table.counters{
      border: 1px solid grey;
      width: 500px;
     }
     table.counters th {
      text-align: right;
      border: 1px solid grey;
      width: 150px;
     }
     table.counters td {
      text-align: right;
      font-family: monospace;
     }
     table.counters tr:hover{
      background-color: #99ddff;
     }

     table.info {
      border: 1px solid grey;
      width: 500px;
     }
     table.info th {
      text-align: center;
      border: 1px solid grey;
      width: 150px;
     }
     table.info td {
      text-align: center;
     }
     table.info tr:hover{
      background-color: #99ddff;
     }

     table.netstat {
      border: 1px solid grey;
      width: 500px;
     }
     table.netstat th {
      text-align: center;
      border: 1px solid grey;
      width: 150px;
     }
     table.netstat td {
      text-align: center;
     }
     table.netstat td:nth-child(4) {
      text-align: right;
      font-family: monospace;
     }
     table.netstat td:nth-child(7) {
      text-align: left;
     }
     table.netstat tr:hover{
      background-color: #99ddff;
     }

     table.mctx  {
      border: 1px solid grey;
      width: 500px;
     }
     table.mctx th {
      text-align: center;
      border: 1px solid grey;
     }
     table.mctx td {
      text-align: right;
      font-family: monospace;
     }
     table.mctx td:nth-child(-n+2) {
      text-align: left;
      width: 100px;
     }
     table.mctx tr:hover{
      background-color: #99ddff;
     }

     table.zones {
      border: 1px solid grey;
      width: 500px;
     }
     table.zones th {
      text-align: center;
      border: 1px solid grey;
     }
     table.zones td {
      text-align: center;
      font-family: monospace;
     }
     table.zones td:nth-child(1) {
      text-align: right;
     }
     table.zones td:nth-child(4) {
      text-align: right;
     }

     .totals {
      background-color: rgb(1,169,206);
      color: #ffffff;
     }
     table.zones {
       border: 1px solid grey;
     }
     table.zones td {
       text-align: right;
       font-family: monospace;
     }
     table.zones td:nth-child(2) {
       text-align: center;
     }
     table.zones td:nth-child(3) {
       text-align: left;
     }
     table.zones tr:hover{
      background-color: #99ddff;
     }

     td, th {
      padding-right: 5px;
      padding-left: 5px;
      border: 1px solid grey;
     }

     .header h1 {
      color: rgb(1,169,206);
      padding: 0px;
     }

     .content {
      background-color: #ffffff;
      color: #000000;
      padding: 4px;
     }

     .item {
      padding: 4px;
      text-align: right;
     }

     .value {
      padding: 4px;
      font-weight: bold;
     }


     h2 {
       color: grey;
       font-size: 14pt;
       width:500px;
       text-align:center;
     }

     h3 {
       color: #444444;
       font-size: 12pt;
       width:500px;
       text-align:center;
     }
     h4 {
	color:  rgb(1,169,206);
	font-size: 10pt;
       width:500px;
       text-align:center;
     }

     .pie {
      width:500px;
      height: 500px;
     }

      </style>
        <title>ISC BIND 9 Statistics</title>
      </head>
      <body>
        <div class="header">
          <h1>ISC Bind 9 Configuration and Statistics</h1>
        </div>
        <p>Alternate statistics views: <a href="/">All</a>,
	<a href="/xml/v3/status">Status</a>,
	<a href="/xml/v3/server">Server</a>,
	<a href="/xml/v3/zones">Zones</a>,
	<a href="/xml/v3/xfrins">Incoming Zone Transfers</a>,
	<a href="/xml/v3/net">Network</a>,
	<a href="/xml/v3/mem">Memory</a> and
	<a href="/xml/v3/traffic">Traffic Size</a></p>
        <hr/>
        <h2>Server Status</h2>
        <table class="info">
          <tr class="odd">
            <th>Boot time:</th>
            <td>
              <xsl:value-of select="server/boot-time"/>
            </td>
          </tr>
          <tr class="even">
            <th>Last reconfigured:</th>
            <td>
              <xsl:value-of select="server/config-time"/>
            </td>
          </tr>
          <tr class="odd">
            <th>Current time:</th>
            <td>
              <xsl:value-of select="server/current-time"/>
            </td>
          </tr>
          <tr class="even">
            <th>Server version:</th>
            <td>
              <xsl:value-of select="server/version"/>
            </td>
          </tr>
        </table>
        <br/>
        <xsl:if test="server/counters[@type=&quot;opcode&quot;]/counter[. &gt; 0]">
          <h2>Incoming Requests by DNS Opcode</h2>
          <div class="pie" id="chart_incoming_opcodes">
	    [cannot display chart]
	  </div>
          <table class="counters">
            <xsl:for-each select="server/counters[@type=&quot;opcode&quot;]/counter[. &gt; 0 or substring(@name,1,3) != 'RES']">
              <xsl:sort select="." data-type="number" order="descending"/>
                <xsl:variable name="css-class0">
                  <xsl:choose>
                    <xsl:when test="position() mod 2 = 0">even</xsl:when>
                    <xsl:otherwise>odd</xsl:otherwise>
                  </xsl:choose>
                </xsl:variable>
                <tr class="{$css-class0}">
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
            <tr>
              <th class="totals">Total:</th>
              <td class="totals">
                <xsl:value-of select="sum(server/counters[@type=&quot;opcode&quot;]/counter)"/>
              </td>
            </tr>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="server/counters[@type=&quot;qtype&quot;]/counter[. &gt; 0]">
          <h3>Incoming Queries by Query Type</h3>
          <div class="pie" id="chart_incoming_qtypes">
	    [cannot display chart]
	  </div>
          <table class="counters">
            <xsl:for-each select="server/counters[@type=&quot;qtype&quot;]/counter">
              <xsl:sort select="." data-type="number" order="descending"/>
              <xsl:variable name="css-class">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class}">
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
            <tr>
              <th class="totals">Total:</th>
              <td class="totals">
                <xsl:value-of select="sum(server/counters[@type=&quot;qtype&quot;]/counter)"/>
              </td>
            </tr>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="views/view[count(counters[@type=&quot;resqtype&quot;]/counter) &gt; 0]">
          <h2>Outgoing Queries per view</h2>
          <xsl:for-each select="views/view[count(counters[@type=&quot;resqtype&quot;]/counter) &gt; 0]">
            <h3>View <xsl:value-of select="@name"/></h3>
            <script type="text/javascript">
	      graphs.push({
                'title': "Outgoing Queries for view: <xsl:value-of select="@name"/>",
                'target': 'chart_outgoing_queries_view_<xsl:value-of select="@name"/>',
                'style': 'barchart',
                'data': [['Type','Counter'],<xsl:for-each select="counters[@type=&quot;resqtype&quot;]/counter">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
	      });
            </script>
            <xsl:variable name="target">
              <xsl:value-of select="@name"/>
            </xsl:variable>
            <div class="pie" id="chart_outgoing_queries_view_{$target}">[no data to display]</div>
            <table class="counters">
              <xsl:for-each select="counters[@type=&quot;resqtype&quot;]/counter">
                <xsl:sort select="." data-type="number" order="descending"/>
                <xsl:variable name="css-class1">
                  <xsl:choose>
                    <xsl:when test="position() mod 2 = 0">even</xsl:when>
                    <xsl:otherwise>odd</xsl:otherwise>
                  </xsl:choose>
                </xsl:variable>
                <tr class="{$css-class1}">
                  <th>
                    <xsl:value-of select="@name"/>
                  </th>
                  <td>
                    <xsl:value-of select="."/>
                  </td>
                </tr>
              </xsl:for-each>
            </table>
            <br/>
          </xsl:for-each>
        </xsl:if>
        <xsl:if test="server/counters[@type=&quot;nsstat&quot;]/counter[.&gt;0]">
          <h2>Server Statistics</h2>
            <script type="text/javascript">
              graphs.push({
                'title' : "Server Counters",
                'target': 'chart_server_nsstat_restype',
                'style': 'barchart',
                'data': [['Type','Counter'],<xsl:for-each select="server/counters[@type=&quot;nsstat&quot;]/counter[.&gt;0]">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
              });
            </script>
          <div class="pie" id="chart_server_nsstat_restype">[no data to display]</div>
          <table class="counters">
            <xsl:for-each select="server/counters[@type=&quot;nsstat&quot;]/counter[.&gt;0]">
              <xsl:sort select="." data-type="number" order="descending"/>
              <xsl:variable name="css-class2">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class2}">
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="server/counters[@type=&quot;zonestat&quot;]/counter[.&gt;0]">
          <h2>Zone Maintenance Statistics</h2>
          <script type="text/javascript">
            graphs.push({
              'title' : "Zone Maintenance Stats",
              'target': 'chart_server_zone_maint',
              'style': 'barchart',
              'data': [['Type','Counter'],<xsl:for-each select="server/counters[@type=&quot;zonestat&quot;]/counter[.&gt;0]">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
            });
          </script>
          <div class="pie" id="chart_server_zone_maint">[no data to display]</div>
          <table class="counters">
            <xsl:for-each select="server/counters[@type=&quot;zonestat&quot;]/counter">
              <xsl:sort select="." data-type="number" order="descending"/>
              <xsl:variable name="css-class3">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class3}">
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
        </xsl:if>
        <xsl:if test="server/counters[@type=&quot;resstat&quot;]/counter[.&gt;0]">
          <h2>Resolver Statistics (Common)</h2>
          <table class="counters">
            <xsl:for-each select="server/counters[@type=&quot;resstat&quot;]/counter">
              <xsl:sort select="." data-type="number" order="descending"/>
              <xsl:variable name="css-class4">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class4}">
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
        </xsl:if>
        <xsl:for-each select="views/view">
          <xsl:if test="counters[@type=&quot;resstats&quot;]/counter[.&gt;0]">
            <h3>Resolver Statistics for View <xsl:value-of select="@name"/></h3>
            <table class="counters">
              <xsl:for-each select="counters[@type=&quot;resstats&quot;]/counter[.&gt;0]">
                <xsl:sort select="." data-type="number" order="descending"/>
                <xsl:variable name="css-class5">
                  <xsl:choose>
                    <xsl:when test="position() mod 2 = 0">even</xsl:when>
                    <xsl:otherwise>odd</xsl:otherwise>
                  </xsl:choose>
                </xsl:variable>
                <tr class="{$css-class5}">
                  <th>
                    <xsl:value-of select="@name"/>
                  </th>
                  <td>
                    <xsl:value-of select="."/>
                  </td>
                </tr>
              </xsl:for-each>
            </table>
          </xsl:if>
        </xsl:for-each>
        <xsl:for-each select="views/view">
          <xsl:if test="counters[@type=&quot;adbstat&quot;]/counter[.&gt;0]">
            <h3>ADB Statistics for View <xsl:value-of select="@name"/></h3>
            <table class="counters">
              <xsl:for-each select="counters[@type=&quot;adbstat&quot;]/counter[.&gt;0]">
                <xsl:sort select="." data-type="number" order="descending"/>
                <xsl:variable name="css-class5">
                  <xsl:choose>
                    <xsl:when test="position() mod 2 = 0">even</xsl:when>
                    <xsl:otherwise>odd</xsl:otherwise>
                  </xsl:choose>
                </xsl:variable>
                <tr class="{$css-class5}">
                  <th>
                    <xsl:value-of select="@name"/>
                  </th>
                  <td>
                    <xsl:value-of select="."/>
                  </td>
                </tr>
              </xsl:for-each>
            </table>
          </xsl:if>
        </xsl:for-each>
        <xsl:for-each select="views/view">
          <xsl:if test="counters[@type=&quot;cachestats&quot;]/counter[.&gt;0]">
            <h3>Cache Statistics for View <xsl:value-of select="@name"/></h3>
            <table class="counters">
              <xsl:for-each select="counters[@type=&quot;cachestats&quot;]/counter[.&gt;0]">
                <xsl:sort select="." data-type="number" order="descending"/>
                <xsl:variable name="css-class5">
                  <xsl:choose>
                    <xsl:when test="position() mod 2 = 0">even</xsl:when>
                    <xsl:otherwise>odd</xsl:otherwise>
                  </xsl:choose>
                </xsl:variable>
                <tr class="{$css-class5}">
                  <th>
                    <xsl:value-of select="@name"/>
                  </th>
                  <td>
                    <xsl:value-of select="."/>
                  </td>
                </tr>
              </xsl:for-each>
            </table>
          </xsl:if>
        </xsl:for-each>
        <xsl:for-each select="views/view">
          <xsl:if test="cache/rrset">
            <h3>Cache DB RRsets for View <xsl:value-of select="@name"/></h3>
            <table class="counters">
              <xsl:for-each select="cache/rrset">
                <xsl:variable name="css-class6">
                  <xsl:choose>
                    <xsl:when test="position() mod 2 = 0">even</xsl:when>
                    <xsl:otherwise>odd</xsl:otherwise>
                  </xsl:choose>
                </xsl:variable>
                <tr class="{$css-class6}">
                  <th>
                    <xsl:value-of select="name"/>
                  </th>
                  <td>
                    <xsl:value-of select="counter"/>
                  </td>
                </tr>
              </xsl:for-each>
            </table>
            <br/>
          </xsl:if>
        </xsl:for-each>
        <xsl:if test="traffic//udp/counters[@type=&quot;request-size&quot;]/counter[.&gt;0] or traffic//udp/counters[@type=&quot;response-size&quot;]/counter[.&gt;0] or traffic//tcp/counters[@type=&quot;request-size&quot;]/counter[.&gt;0] or traffic//tcp/counters[@type=&quot;response-size&quot;]/counter[.&gt;0]">
          <h2>Traffic Size Statistics</h2>
        </xsl:if>
        <xsl:if test="traffic//udp/counters[@type=&quot;request-size&quot;]/counter[.&gt;0]">
          <h4>UDP Requests Received</h4>
          <table class="counters">
            <xsl:for-each select="traffic//udp/counters[@type=&quot;request-size&quot;]/counter[.&gt;0]">
              <xsl:variable name="css-class7">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class7}">
                <th><xsl:value-of select="local-name(../../..)"/></th>
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="traffic//udp/counters[@type=&quot;response-size&quot;]/counter[.&gt;0]">
          <h4>UDP Responses Sent</h4>
          <table class="counters">
            <xsl:for-each select="traffic//udp/counters[@type=&quot;response-size&quot;]/counter[.&gt;0]">
              <xsl:variable name="css-class7">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class7}">
                <th><xsl:value-of select="local-name(../../..)"/></th>
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="traffic//tcp/counters[@type=&quot;request-size&quot;]/counter[.&gt;0]">
          <h4>TCP Requests Received</h4>
          <table class="counters">
            <xsl:for-each select="traffic//tcp/counters[@type=&quot;request-size&quot;]/counter[.&gt;0]">
              <xsl:variable name="css-class7">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class7}">
                <th><xsl:value-of select="local-name(../../..)"/></th>
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="traffic//tcp/counters[@type=&quot;response-size&quot;]/counter[.&gt;0]">
          <h4>TCP Responses Sent</h4>
          <table class="counters">
            <xsl:for-each select="traffic//tcp/counters[@type=&quot;response-size&quot;]/counter[.&gt;0]">
              <xsl:variable name="css-class7">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class7}">
                <th><xsl:value-of select="local-name(../../..)"/></th>
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="server/counters[@type=&quot;sockstat&quot;]/counter[.&gt;0]">
          <h2>Socket I/O Statistics</h2>
          <table class="counters">
            <xsl:for-each select="server/counters[@type=&quot;sockstat&quot;]/counter[.&gt;0]">
              <xsl:variable name="css-class7">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class7}">
                <th>
                  <xsl:value-of select="@name"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="views/view/zones/zone">
          <xsl:for-each select="views/view">
            <h3>Zones for View <xsl:value-of select="@name"/></h3>
            <table class="zones">
              <thead><tr><th>Name</th><th>Class</th><th>Type</th><th>Serial</th><th>Loaded</th><th>Expires</th><th>Refresh</th></tr></thead>
              <tbody>
                <xsl:for-each select="zones/zone">
                  <xsl:variable name="css-class15">
                    <xsl:choose>
                      <xsl:when test="position() mod 2 = 0">even</xsl:when>
                      <xsl:otherwise>odd</xsl:otherwise>
                    </xsl:choose>
                  </xsl:variable>
                  <tr class="{$css-class15}">
                      <td><xsl:value-of select="@name"/></td>
                      <td><xsl:value-of select="@rdataclass"/></td>
                      <td><xsl:value-of select="type"/></td>
                      <td><xsl:value-of select="serial"/></td>
                      <td><xsl:value-of select="loaded"/></td>
                      <td><xsl:value-of select="expires"/></td>
                      <td><xsl:value-of select="refresh"/></td></tr>
                </xsl:for-each>
              </tbody>
            </table>
          </xsl:for-each>
        </xsl:if>
        <xsl:if test="views/view[zones/zone/counters[@type=&quot;qtype&quot;]/counter &gt;0]">
          <h2>Received QTYPES per view/zone</h2>
          <xsl:for-each select="views/view[zones/zone/counters[@type=&quot;qtype&quot;]/counter &gt;0]">
            <h3>View <xsl:value-of select="@name"/></h3>
            <xsl:variable name="thisview">
              <xsl:value-of select="@name"/>
            </xsl:variable>
            <xsl:for-each select="zones/zone">
              <xsl:if test="counters[@type=&quot;qtype&quot;]/counter[count(.) &gt; 0]">
                <h4>Zone <xsl:value-of select="@name"/></h4>
                <script type="text/javascript">
                  graphs.push({
                    'title': "Query types for zone <xsl:value-of select="@name"/>",
                    'target': 'chart_qtype_<xsl:value-of select="../../@name"/>_<xsl:value-of select="@name"/>',
                    'style': 'barchart',
                    'data': [['Type','Counter'],<xsl:for-each select="counters[@type=&quot;qtype&quot;]/counter[.&gt;0 and @name != &quot;QryAuthAns&quot;]">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
                  });
                </script>
                <xsl:variable name="target">
                  <xsl:value-of select="@name"/>
                </xsl:variable>
                <div class="pie" id="chart_qtype_{$thisview}_{$target}">[no data to display]</div>
                <table class="counters">
                  <xsl:for-each select="counters[@type=&quot;qtype&quot;]/counter">
                    <xsl:sort select="."/>
                    <xsl:variable name="css-class10">
                      <xsl:choose>
                        <xsl:when test="position() mod 2 = 0">even</xsl:when>
                        <xsl:otherwise>odd</xsl:otherwise>
                      </xsl:choose>
                    </xsl:variable>
                    <tr class="{$css-class10}">
                      <th>
                        <xsl:value-of select="@name"/>
                      </th>
                      <td>
                        <xsl:value-of select="."/>
                      </td>
                    </tr>
                  </xsl:for-each>
                </table>
              </xsl:if>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:if>
        <xsl:if test="views/view[zones/zone/counters[@type=&quot;rcode&quot;]/counter &gt;0]">
          <h2>Response Codes per view/zone</h2>
          <xsl:for-each select="views/view[zones/zone/counters[@type=&quot;rcode&quot;]/counter &gt;0]">
            <h3>View <xsl:value-of select="@name"/></h3>
            <xsl:variable name="thisview2">
              <xsl:value-of select="@name"/>
            </xsl:variable>
            <xsl:for-each select="zones/zone">
              <xsl:if test="counters[@type=&quot;rcode&quot;]/counter[. &gt; 0]">
                <h4>Zone <xsl:value-of select="@name"/></h4>
                <script type="text/javascript">
                  graphs.push({
                    'title': "Response codes for zone <xsl:value-of select="@name"/>",
                    'target': 'chart_rescode_<xsl:value-of select="../../@name"/>_<xsl:value-of select="@name"/>',
                    'style': 'barchart',
                    'data': [['Type','Counter'],<xsl:for-each select="counters[@type=&quot;rcode&quot;]/counter[.&gt;0 and @name != &quot;QryAuthAns&quot;]">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
                  });
                </script>
                <xsl:variable name="target">
                  <xsl:value-of select="@name"/>
                </xsl:variable>
                <div class="pie" id="chart_rescode_{$thisview2}_{$target}">[no data to display]</div>
                <table class="counters">
                  <xsl:for-each select="counters[@type=&quot;rcode&quot;]/counter[.&gt;0 and @name != &quot;QryAuthAns&quot;]">
                    <xsl:sort select="."/>
                    <xsl:variable name="css-class11">
                      <xsl:choose>
                        <xsl:when test="position() mod 2 = 0">even</xsl:when>
                        <xsl:otherwise>odd</xsl:otherwise>
                      </xsl:choose>
                    </xsl:variable>
                    <tr class="{$css-class11}">
                      <th>
                        <xsl:value-of select="@name"/>
                      </th>
                      <td>
                        <xsl:value-of select="."/>
                      </td>
                    </tr>
                  </xsl:for-each>
                </table>
              </xsl:if>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:if>
        <xsl:if test="views/view[zones/zone/counters[@type=&quot;gluecache&quot;]/counter &gt;0]">
          <h2>Glue cache statistics</h2>
          <xsl:for-each select="views/view[zones/zone/counters[@type=&quot;gluecache&quot;]/counter &gt;0]">
            <h3>View <xsl:value-of select="@name"/></h3>
            <xsl:variable name="thisview2">
              <xsl:value-of select="@name"/>
            </xsl:variable>
            <xsl:for-each select="zones/zone">
              <xsl:if test="counters[@type=&quot;gluecache&quot;]/counter[. &gt; 0]">
                <h4>Zone <xsl:value-of select="@name"/></h4>
                <table class="counters">
                  <xsl:for-each select="counters[@type=&quot;gluecache&quot;]/counter[. &gt; 0]">
                    <xsl:sort select="."/>
                    <xsl:variable name="css-class11">
                      <xsl:choose>
                        <xsl:when test="position() mod 2 = 0">even</xsl:when>
                        <xsl:otherwise>odd</xsl:otherwise>
                      </xsl:choose>
                    </xsl:variable>
                    <tr class="{$css-class11}">
                      <th>
                        <xsl:value-of select="@name"/>
                      </th>
                      <td>
                        <xsl:value-of select="."/>
                      </td>
                    </tr>
                  </xsl:for-each>
                </table>
              </xsl:if>
            </xsl:for-each>
          </xsl:for-each>
        </xsl:if>
        <xsl:if test="views/view/xfrins/xfrin">
          <xsl:for-each select="views/view">
            <h3>Incoming Zone Transfers for View <xsl:value-of select="@name"/></h3>
            <table class="xfrins">
              <thead>
	        <tr>
                  <th>Zone Name</th>
                  <th>Zone Type</th>
                  <th>Local Serial</th>
                  <th>Remote Serial</th>
                  <th>IXFR</th>
                  <th>First Refresh</th>
                  <th>State</th>
                  <th>Additional Refresh Queued</th>
                  <th>Local Address</th>
                  <th>Remote Address</th>
                  <th>SOA Transport</th>
                  <th>Transport</th>
                  <th>TSIG Key Name</th>
                  <th>Duration (s)</th>
                  <th>Messages Received</th>
                  <th>Records Received</th>
                  <th>Bytes Received</th>
                  <th>Transfer Rate (B/s)</th>
                </tr>
              </thead>
              <tbody>
                <xsl:for-each select="xfrins/xfrin">
                  <xsl:variable name="css-class16">
                    <xsl:choose>
                      <xsl:when test="position() mod 2 = 0">even</xsl:when>
                      <xsl:otherwise>odd</xsl:otherwise>
                    </xsl:choose>
                  </xsl:variable>
                  <tr class="{$css-class16}">
                    <td><xsl:value-of select="@name"/></td>
                    <td><xsl:value-of select="type"/></td>
                    <td><xsl:value-of select="serial"/></td>
                    <td><xsl:value-of select="remoteserial"/></td>
                    <td><xsl:value-of select="ixfr"/></td>
                    <td><xsl:value-of select="firstrefresh"/></td>
                    <td><xsl:value-of select="state"/></td>
                    <td><xsl:value-of select="refreshqueued"/></td>
                    <td><xsl:value-of select="localaddr"/></td>
                    <td><xsl:value-of select="remoteaddr"/></td>
                    <td><xsl:value-of select="soatransport"/></td>
                    <td><xsl:value-of select="transport"/></td>
                    <td><xsl:value-of select="tsigkeyname"/></td>
                    <td><xsl:value-of select="duration"/></td>
                    <td><xsl:value-of select="nmsg"/></td>
                    <td><xsl:value-of select="nrecs"/></td>
                    <td><xsl:value-of select="nbytes"/></td>
                    <td><xsl:value-of select="rate"/></td>
                  </tr>
                </xsl:for-each>
              </tbody>
            </table>
          </xsl:for-each>
        </xsl:if>
        <xsl:if test="memory/summary">
          <h2>Memory Usage Summary</h2>
          <table class="counters">
            <xsl:for-each select="memory/summary/*">
              <xsl:variable name="css-class13">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class13}">
                <th>
                  <xsl:value-of select="name()"/>
                </th>
                <td>
                  <xsl:value-of select="."/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="memory/contexts/context">
          <h2>Memory Contexts</h2>
          <table class="mctx">
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>References</th>
              <th>InUse</th>
              <th>Pools</th>
              <th>HiWater</th>
              <th>LoWater</th>
            </tr>
            <xsl:for-each select="memory/contexts/context">
              <xsl:sort select="total" data-type="number" order="descending"/>
              <xsl:variable name="css-class14">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class14}">
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
                  <xsl:value-of select="inuse"/>
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
        </xsl:if>
        <hr/>
        <p class="footer">Internet Systems Consortium Inc.<br/><a href="http://www.isc.org">http://www.isc.org</a></p>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
