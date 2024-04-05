//SPDX-License-Identifier: GPL-3.0
#pragma once

#include <stdexcept>

class qalculate_exception : public std::runtime_error {
	protected:
		int code;

		qalculate_exception(int code);

	public:
		int getCode() const;
};

