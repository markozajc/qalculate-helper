//SPDX-License-Identifier: GPL-3.0

#include <config.h>
#include <exchange_update_exception.h>
#include <libqalculate/Calculator.h>
#include <security_util.h>

int main() {
	do_setuid();

	Calculator calc(true);
	if (!calc.canFetch())
		throw exchange_update_exception();
	calc.fetchExchangeRates(TIMEOUT_UPDATE);
	// for some reason this returns false even when it's successful so I'm not going to check it
}
