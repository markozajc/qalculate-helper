//SPDX-License-Identifier: GPL-3.0
#pragma once

#include <stdexcept>

class qalculate_exception : public std::runtime_error {
	public:
		int get_code() const;

	protected:
		int code;

		explicit qalculate_exception(int code);
};

