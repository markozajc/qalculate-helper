//SPDX-License-Identifier: GPL-3.0

#include <qalculate_exception.h>
#include <string>

qalculate_exception::qalculate_exception(int code) :
		std::runtime_error(std::to_string(code).c_str()), code(code) {
}

[[nodiscard]] int qalculate_exception::get_code() const {
	return this->code;
}
