//SPDX-License-Identifier: GPL-3.0
/*
 * qalculate-helper.cpp
 * Copyright (C) 2024 Marko Zajc
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <exchange_update_exception.h>
#include <libqalculate/Calculator.h>
#include <libqalculate/includes.h>
#include <libqalculate/MathStructure.h>
#include <security_util.h>
#include <timeout_exception.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::stringstream;
using std::getline;
using std::vector;
using std::string_view;
using std::size_t;

#if __cplusplus >= 201703L
#include <string_view>

static bool ends_with(string_view str, string_view suffix) {
	return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

#endif

const char TYPE_MESSAGE = 1;
const char TYPE_RESULT = 2;

const char RESULT_APPROXIMATION_NO = 1;
const char RESULT_APPROXIMATION_YES = 2;

const char LEVEL_INFO = 1;
const char LEVEL_WARNING = 2;
const char LEVEL_ERROR = 3;
const char LEVEL_UNKNOWN = 4;
const char SEPARATOR = 0;

#define COMMAND_UPDATE "update"

const unsigned long MODE_PRECISION = 1 << 0;
const unsigned long MODE_EXACT = 1 << 1;
const unsigned long MODE_NOCOLOR = 1 << 2;

const int ETIMEOUT = 102;
const int ECANTFETCH = 103;

static MathStructure evaluate_single(Calculator *calc, const EvaluationOptions &eo, unsigned long line_number,
									 const string &expression) {
	MathStructure result;
	if (!calc->calculate(&result, calc->unlocalizeExpression(expression), TIMEOUT_CALC, eo))
		throw timeout_exception();

	const CalculatorMessage *message;
	while ((message = CALCULATOR->message())) {
		putchar(TYPE_MESSAGE);
		switch (message->type()) {
			case MESSAGE_INFORMATION:
				putchar(LEVEL_INFO);
				break;
			case MESSAGE_WARNING:
				putchar(LEVEL_WARNING);
				break;
			case MESSAGE_ERROR:
				putchar(LEVEL_ERROR);
				break;
			default:
				putchar(LEVEL_UNKNOWN);
				break;
		}
		printf("line %lu: ", line_number);
		fputs(message->c_message(), stdout);
		putchar(SEPARATOR);
		calc->nextMessage();
	}
	return result;
}

static bool mode_set(unsigned long mode, unsigned long test) {
	return mode & test;
}

static void set_precision(Calculator *calc, unsigned long mode, EvaluationOptions &eo, PrintOptions &po) {
	int precision = PRECISION_DEFAULT;

	if (mode_set(mode, MODE_EXACT)) {
		eo.approximation = APPROXIMATION_EXACT;
		po.number_fraction_format = FRACTION_DECIMAL_EXACT;

	} else if (mode_set(mode, MODE_PRECISION)) {
		precision = PRECISION_HIGH;
		po.indicate_infinite_series = false;
	}

	calc->setPrecision(precision);
}

MathStructure evaluate_all(const vector<string> &expressions, const EvaluationOptions &eo, Calculator *calc) {
	for (size_t i = 0; i < expressions.size() - 1; ++i)
		evaluate_single(calc, eo, i + 1, expressions[i]);
	return evaluate_single(calc, eo, expressions.size(), expressions.back());
}

void print_result(Calculator *calc, const MathStructure &result_struct, const PrintOptions &po, int mode,
				  bool &approximate) {
	string result = calc->print(result_struct, TIMEOUT_PRINT, po, false, mode_set(mode, MODE_NOCOLOR) ? 0 : 1,
								TAG_TYPE_TERMINAL);

	if (ends_with(result, calc->timedOutString())) {
		throw timeout_exception();

	} else {
		putchar(TYPE_RESULT);
		putchar(approximate ? RESULT_APPROXIMATION_YES : RESULT_APPROXIMATION_NO);
		fputs(result.c_str(), stdout);
	}
	putchar(SEPARATOR);
}

static EvaluationOptions get_evaluationoptions() {
	EvaluationOptions eo;
	eo.approximation = APPROXIMATION_TRY_EXACT;
	eo.parse_options.unknowns_enabled = false;
	// eo.sync_units = false; // causes issues with monetary conversion, eg x usd > 1 eur. no idea why I
	                          // enabled this in the first place
	return eo;
}

static PrintOptions get_printoptions(int base, bool *approximate) {
	PrintOptions po;
	po.base = base;
	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	po.use_unicode_signs = true;
	//po.min_decimals = MIN_DECIMALS;
	po.time_zone = TIME_ZONE_UTC;
	po.abbreviate_names = true;
	po.spell_out_logical_operators = true;
	po.allow_non_usable = true;
	po.show_ending_zeroes = false;
	//po.preserve_precision = true;
	//po.restrict_to_parent_precision = false;
	po.is_approximate = approximate;
	return po;
}

static void evaluate(Calculator *calc, const vector<string> &expressions, unsigned int mode, int base) {
	calc->setExchangeRatesWarningEnabled(false);
	calc->loadExchangeRates();
	calc->loadGlobalDefinitions();

	bool approximate = false;

	PrintOptions po = get_printoptions(base, &approximate);
	EvaluationOptions eo = get_evaluationoptions();
	set_precision(calc, mode, eo, po);

	calc->setMessagePrintOptions(po);

	do_seccomp();

	auto result_struct = evaluate_all(expressions, eo, calc);

	print_result(calc, result_struct, po, mode, approximate);
}

static void update() {
	if (!CALCULATOR->canFetch())
		throw exchange_update_exception();

	CALCULATOR->fetchExchangeRates(TIMEOUT_UPDATE);
	// for some reason this returns false even when it's successful so I'm not going to check it
}

static vector<string> parseExpressions(stringstream input) {
	vector<string> result;
	string expression;
	while (std::getline(input, expression, '\n'))
		result.push_back(expression);

	return result;
}

int main(int argc, char **argv) {
	do_setuid();

	if (argc < 2)
		return 1;

	auto *calc = new Calculator(true);
	do_defang_calculator(calc);
	try {
		if (argc == 2) {
			if (strcmp(argv[1], COMMAND_UPDATE) == 0)
				update();
			else
				return 1;
		} else {
			evaluate(calc, parseExpressions(stringstream(argv[1])), std::strtoul(argv[2], nullptr, 10),
					 std::strtol(argv[3], nullptr, 10));
		}

	} catch (const qalculate_exception &e) {
		return e.getCode();
	}
}

