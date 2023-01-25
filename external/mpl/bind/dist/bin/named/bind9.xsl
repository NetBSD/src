<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">
  <xsl:output method="html" indent="yes" version="4.0"/>
  <!-- the version number **below** must match version in bin/named/statschannel.c -->
  <!-- don't forget to update "/xml/v<STATS_XML_VERSION_MAJOR>" in the HTTP endpoints listed below -->
  <xsl:template match="statistics[@version=&quot;3.11.1&quot;]">
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

        <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
          <!-- Non Mozilla specific markup -->
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

	      while(g = graphs.shift()){
		// alert("going for: " + g.target);
		if(g.data.length > 1){
		  drawChart(g.title,g.target,g.style,g.data);
		}
	      }
	    }

	    <xsl:if test="server/counters[@type=&quot;qtype&quot;]/counter">
	      // Server Incoming Query Types
	      graphs.push({
			   'title' : "Server Incoming Query Types",
			   'target': 'chart_incoming_qtypes',
			   'style': 'barchart',
			   'data': [['Type','Counter'],<xsl:for-each select="server/counters[@type=&quot;qtype&quot;]/counter">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
			   });
	    </xsl:if>

	    <xsl:if test="server/counters[@type=&quot;opcode&quot;]/counter">
	      // Server Incoming Requests by opcode
	      graphs.push({
			   'title' : "Server Incoming Requests by DNS Opcode",
			   'target': 'chart_incoming_opcodes',
			   'style': 'barchart',
			   'data': [['Opcode','Counter'],<xsl:for-each select="server/counters[@type=&quot;opcode&quot;]/counter[. &gt; 0 or substring(@name,1,3) != 'RES']">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]});
	    </xsl:if>
	  </script>
        </xsl:if>
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

     table.tasks {
      border: 1px solid grey;
      width: 500px;
     }
     table.tasks th {
      text-align: center;
      border: 1px solid grey;
      width: 150px;
     }
     table.tasks td {
      text-align: right;
      font-family: monospace;
     }
     table.tasks td:nth-child(2) {
      text-align: center;
     }
     table.tasks td:nth-child(4) {
      text-align: center;
     }
     table.tasks tr:hover{
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
	<a href="/xml/v3/net">Network</a>,
	<a href="/xml/v3/tasks">Tasks</a>,
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
          <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
            <h2>Incoming Requests by DNS Opcode</h2>
            <!-- Non Mozilla specific markup -->
            <div class="pie" id="chart_incoming_opcodes">
	      [cannot display chart]
	    </div>
          </xsl:if>
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
        <xsl:if test="server/counters[@type=&quot;qtype&quot;]/counter">
          <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
            <!-- Non Mozilla specific markup -->
            <h3>Incoming Queries by Query Type</h3>
            <div class="pie" id="chart_incoming_qtypes">
	      [cannot display chart]
	    </div>
          </xsl:if>
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
            <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
              <!-- Non Mozilla specific markup -->
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
            </xsl:if>
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
          <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
            <!-- Non Mozilla specific markup -->
            <script type="text/javascript">
		  graphs.push({
				'title' : "Server Counters",
				'target': 'chart_server_nsstat_restype',
				'style': 'barchart',
				'data': [['Type','Counter'],<xsl:for-each select="server/counters[@type=&quot;nsstat&quot;]/counter[.&gt;0]">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
				});
	      </script>
            <div class="pie" id="chart_server_nsstat_restype">[no data to display]</div>
          </xsl:if>
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
          <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
            <h2>Zone Maintenance Statistics</h2>
            <script type="text/javascript">
		  graphs.push({
				'title' : "Zone Maintenance Stats",
				'target': 'chart_server_zone_maint',
				'style': 'barchart',
				'data': [['Type','Counter'],<xsl:for-each select="server/counters[@type=&quot;zonestat&quot;]/counter[.&gt;0]">['<xsl:value-of select="@name"/>',<xsl:value-of select="."/>],</xsl:for-each>]
				});
	    </script>
            <!-- Non Mozilla specific markup -->
            <div class="pie" id="chart_server_zone_maint">[no data to display]</div>
          </xsl:if>
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
                <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
                  <!-- Non Mozilla specific markup -->
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
                </xsl:if>
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
                <xsl:if test="system-property('xsl:vendor')!='Transformiix'">
                  <!-- Non Mozilla specific markup -->
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
                </xsl:if>
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
        <xsl:if test="socketmgr/sockets/socket">
          <h2>Network Status</h2>
          <table class="netstat">
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>Type</th>
              <th>References</th>
              <th>LocalAddress</th>
              <th>PeerAddress</th>
              <th>State</th>
            </tr>
            <xsl:for-each select="socketmgr/sockets/socket">
              <xsl:sort select="id"/>
              <xsl:variable name="css-class12">
                <xsl:choose>
                  <xsl:when test="position() mod 2 = 0">even</xsl:when>
                  <xsl:otherwise>odd</xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <tr class="{$css-class12}">
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
        </xsl:if>
        <xsl:if test="taskmgr/thread-model/type">
          <h2>Task Manager Configuration</h2>
          <table class="counters">
            <tr>
              <th class="even">Thread-Model</th>
              <td>
                <xsl:value-of select="taskmgr/thread-model/type"/>
              </td>
            </tr>
            <tr class="odd">
              <th>Worker Threads</th>
              <td>
                <xsl:value-of select="taskmgr/thread-model/worker-threads"/>
              </td>
            </tr>
            <tr class="even">
              <th>Default Quantum</th>
              <td>
                <xsl:value-of select="taskmgr/thread-model/default-quantum"/>
              </td>
            </tr>
            <tr class="odd">
              <th>Tasks Running</th>
              <td>
                <xsl:value-of select="taskmgr/thread-model/tasks-running"/>
              </td>
            </tr>
            <tr class="even">
              <th>Tasks Ready</th>
              <td>
                <xsl:value-of select="taskmgr/thread-model/tasks-ready"/>
              </td>
            </tr>
          </table>
          <br/>
        </xsl:if>
        <xsl:if test="taskmgr/tasks/task">
          <h2>Tasks</h2>
          <table class="tasks">
            <tr>
              <th>ID</th>
              <th>Name</th>
              <th>References</th>
              <th>State</th>
              <th>Quantum</th>
              <th>Events</th>
            </tr>
            <xsl:for-each select="taskmgr/tasks/task">
              <xsl:sort select="name"/>
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
                  <xsl:value-of select="state"/>
                </td>
                <td>
                  <xsl:value-of select="quantum"/>
                </td>
                <td>
                  <xsl:value-of select="events"/>
                </td>
              </tr>
            </xsl:for-each>
          </table>
          <br/>
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
              <th>TotalUse</th>
              <th>InUse</th>
              <th>MaxUse</th>
              <th>Malloced</th>
              <th>MaxMalloced</th>
              <th>BlockSize</th>
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
                  <xsl:value-of select="total"/>
                </td>
                <td>
                  <xsl:value-of select="inuse"/>
                </td>
                <td>
                  <xsl:value-of select="maxinuse"/>
                </td>
                <td>
                  <xsl:value-of select="malloced"/>
                </td>
                <td>
                  <xsl:value-of select="maxmalloced"/>
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
        </xsl:if>
        <hr/>
        <p class="footer">Internet Systems Consortium Inc.<br/><a href="http://www.isc.org">http://www.isc.org</a></p>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
