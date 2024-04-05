//SPDX-License-Identifier: GPL-3.0
#pragma once

#include <qalculate_exception.h>

class timeout_exception : public qalculate_exception {
	public:
		timeout_exception();
};

