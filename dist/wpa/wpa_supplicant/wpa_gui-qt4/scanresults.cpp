/*
 * wpa_gui - ScanResults class
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include <QTimer>

#include "scanresults.h"
#include "wpagui.h"
#include "networkconfig.h"


ScanResults::ScanResults(QWidget *parent, const char *, bool, Qt::WFlags)
	: QDialog(parent)
{
	setupUi(this);

	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(scanButton, SIGNAL(clicked()), this, SLOT(scanRequest()));
	connect(scanResultsWidget,
		SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
		SLOT(bssSelected(QTreeWidgetItem *)));

	wpagui = NULL;
	scanResultsWidget->setItemsExpandable(FALSE);
	scanResultsWidget->setRootIsDecorated(FALSE);
}


ScanResults::~ScanResults()
{
	delete timer;
}


void ScanResults::languageChange()
{
	retranslateUi(this);
}


void ScanResults::setWpaGui(WpaGui *_wpagui)
{
	wpagui = _wpagui;
	updateResults();
    
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(getResults()));
	timer->setSingleShot(FALSE);
	timer->start(10000);
}


void ScanResults::updateResults()
{
	char reply[8192];
	size_t reply_len;
    
	if (wpagui == NULL)
		return;

	reply_len = sizeof(reply) - 1;
	if (wpagui->ctrlRequest("SCAN_RESULTS", reply, &reply_len) < 0)
		return;
	reply[reply_len] = '\0';

	scanResultsWidget->clear();

	QString res(reply);
	QStringList lines = res.split(QChar('\n'));
	bool first = true;
	for (QStringList::Iterator it = lines.begin(); it != lines.end(); it++)
	{
		if (first) {
			first = false;
			continue;
		}

		QStringList cols = (*it).split(QChar('\t'));
		QString ssid, bssid, freq, signal, flags;
		bssid = cols.count() > 0 ? cols[0] : "";
		freq = cols.count() > 1 ? cols[1] : "";
		signal = cols.count() > 2 ? cols[2] : "";
		flags = cols.count() > 3 ? cols[3] : "";
		ssid = cols.count() > 4 ? cols[4] : "";

		QTreeWidgetItem *item = new QTreeWidgetItem(scanResultsWidget);
		if (item) {
			item->setText(0, ssid);
			item->setText(1, bssid);
			item->setText(2, freq);
			item->setText(3, signal);
			item->setText(4, flags);
		}
	}
}


void ScanResults::scanRequest()
{
	char reply[10];
	size_t reply_len = sizeof(reply);
    
	if (wpagui == NULL)
		return;
    
	wpagui->ctrlRequest("SCAN", reply, &reply_len);
}


void ScanResults::getResults()
{
	updateResults();
}


void ScanResults::bssSelected(QTreeWidgetItem *sel)
{
	NetworkConfig *nc = new NetworkConfig();
	if (nc == NULL)
		return;
	nc->setWpaGui(wpagui);
	nc->paramsFromScanResults(sel);
	nc->show();
	nc->exec();
}
