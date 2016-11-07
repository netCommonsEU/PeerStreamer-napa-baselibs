/***************************************************************************
 *   Copyright (C) 2009 by Robert Birke
 *   robert.birke@polito.it
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 ***********************************************************************/

#ifndef _HOPCOUNT_MEASURE_H_
#define _HOPCOUNT_MEASURE_H_

#include <string>
#include "measure_plugin.h"
#include "mon_measure.h"

class HopcountMeasure : public MonMeasure {

public:
	HopcountMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md);
	virtual ~HopcountMeasure();
	virtual result RxPkt(result *r,ExecutionList *el);
	virtual void stop();
};

class HopcountMeasurePlugin : public MeasurePlugin {
	virtual ~HopcountMeasurePlugin() {};

	virtual MeasurementCapabilities getCaps() {
		return IN_BAND | PACKET | TXRXUNI;
	};

	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new HopcountMeasure(this, mc, md);
	};
public:
	HopcountMeasurePlugin();
};

#endif /* _EXAMPLE_MEASURE_H_ */

