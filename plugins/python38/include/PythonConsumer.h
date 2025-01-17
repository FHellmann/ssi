// PythonConsumer.h
// author: Johannes Wagner <wagner@hcm-lab.de>
// created: 2016/03/02
// Copyright (C) University of Augsburg, Lab for Human Centered Multimedia
//
// *************************************************************************************************
//
// This file is part of Social Signal Interpretation (SSI) developed at the 
// Lab for Human Centered Multimedia of the University of Augsburg
//
// This library is free software; you can redistribute itand/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or any laterversion.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FORA PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along withthis library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//*************************************************************************************************

#pragma once

#ifndef SSI_PYTHON_CONSUMER_H
#define SSI_PYTHON_CONSUMER_H

#include "base/IConsumer.h"
#include "PythonOptions.h"

namespace ssi {

class PythonHelper;

class PythonConsumer : public IConsumer {

public:

	class Options : public PythonOptions {
	};

public:

	static const ssi_char_t *GetCreateName () { return "PythonConsumer"; };
	static IObject *Create (const ssi_char_t *file) { return new PythonConsumer (file); };
	~PythonConsumer ();

	PythonConsumer::Options *getOptions () { return &_options; };
	const ssi_char_t *getName () { return GetCreateName (); };
	const ssi_char_t *getInfo () { return "Python consumer wrapper."; };

	void consume_enter(ssi_size_t stream_in_num,
		ssi_stream_t stream_in[]);
	void consume(IConsumer::info consume_info,
		ssi_size_t stream_in_num,
		ssi_stream_t stream_in[]);
	void consume_flush(ssi_size_t stream_in_num,
		ssi_stream_t stream_in[]);

	bool setEventListener(IEventListener *listener);
	const ssi_char_t *getEventAddress();
	void send_enter();
	void send_flush();

	void listen_enter();
	bool update(ssi_event_t &e);
	bool update(IEvents &events, ssi_size_t n_new_events, ssi_size_t time_ms);
	void listen_flush();

protected:

	PythonConsumer (const ssi_char_t *file = 0);
	PythonConsumer::Options _options;
	ssi_char_t *_file;

	static ssi_char_t *ssi_log_name;

	void initHelper();
	PythonHelper *_helper;
};

}

#endif
