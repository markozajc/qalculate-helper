#include <sstream>
#include <libqalculate/qalculate.h>
#ifdef UID
	#include <grp.h>
	#include <cap-ng.h>
#else
	#warning "Not doing setuid/setgid, do not use in production!"
#endif

#ifdef SECCOMP
	#include <seccomp.h>
#else
	#warning "Not doing seccomp, do not use in production!"
#endif

#define PRECISION_DEFAULT 20
#define PRECISION_HIGH 900

#define MIN_DECIMALS 20
#define TIMEOUT_CALC 2000
#define TIMEOUT_PRINT 2000
#define TIMEOUT_UPDATE 30

#define TYPE_MESSAGE putchar(1)
#define TYPE_RESULT putchar(2)

#define RESULT_APPROXIMATION_NO putchar(1);
#define RESULT_APPROXIMATION_YES putchar(2);

#define LEVEL_INFO putchar(1)
#define LEVEL_WARNING putchar(2)
#define LEVEL_ERROR putchar(3)
#define LEVEL_UNKNOWN putchar(4)
#define SEPARATOR putchar(0)

#define COMMAND_UPDATE "update"

#define MODE_HIGH_PRECISION 2
#define MODE_EXACT 3

#define ENOARG 101;
#define ETIMEOUT 102;
#define ECANTFETCH 103;

using std::string;
using std::stringstream;
using std::getline;

int evaluate_single(EvaluationOptions *eo, MathStructure *result, int line_number, string expression) {
	if (!CALCULATOR->calculate(result, CALCULATOR->unlocalizeExpression(expression), TIMEOUT_CALC, *eo))
		return ETIMEOUT;

	CalculatorMessage* message;
	while ((message = CALCULATOR->message())) {
		TYPE_MESSAGE;
		switch (message->type()) {
			case MESSAGE_INFORMATION:
				LEVEL_INFO;
				break;
			case MESSAGE_WARNING:
				LEVEL_WARNING;
				break;
			case MESSAGE_ERROR:
				LEVEL_ERROR;
				break;
			default:
				LEVEL_UNKNOWN;
				break;
		}
		printf("line %d: ", line_number);
		fputs(message->c_message(), stdout); SEPARATOR;
		CALCULATOR->nextMessage();
	}
	return 0;
}

int evaluate(char *expression, char *mode, char *base) {
	CALCULATOR->setExchangeRatesWarningEnabled(false);
	CALCULATOR->loadExchangeRates();
	CALCULATOR->loadGlobalDefinitions();

	CALCULATOR->getActiveFunction("command")->destroy(); // rce
#ifdef HAS_PLOT
	CALCULATOR->getActiveFunction("plot")->destroy(); // no use
#endif
	CALCULATOR->getActiveVariable("uptime")->destroy(); // why would you

	PrintOptions po;

	if (strlen(base) == 1)
		po.base = *base;

	po.number_fraction_format = FRACTION_DECIMAL;
	po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	po.use_unicode_signs = true;
	//po.min_decimals = MIN_DECIMALS;
	po.time_zone = TIME_ZONE_UTC;
	po.abbreviate_names = true;
	po.show_ending_zeroes = false;
	//po.preserve_precision = true;
	//po.restrict_to_parent_precision = false;

	bool approximate = false;
	po.is_approximate = &approximate;
	CALCULATOR->setMessagePrintOptions(po);

	EvaluationOptions eo;

	eo.approximation = APPROXIMATION_TRY_EXACT;
	eo.parse_options.unknowns_enabled = false;
	eo.sync_units = false;

#ifdef SECCOMP
	scmp_filter_ctx ctx;
	ctx = seccomp_init(SCMP_ACT_KILL);
    /*   0 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    /*   1 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    /*   9 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
    /*  10 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
    /*  11 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
    /*  13 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0);
    /*  14 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);
    /*  24 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sched_yield), 0);
    /* 230 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_nanosleep), 0);
    /* 231 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
    /* 262 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);
    /* 273 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_robust_list), 0);
    /* 334 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rseq), 0);
    /* 435 */ seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone3), 0);
	int err = seccomp_load(ctx);
	if (err) {
		printf("couldn't seccomp: %d\n", err);
		abort();
	}
#endif

	int precision = PRECISION_DEFAULT;
	if (strlen(mode) == 1) {
		if (*mode == MODE_EXACT) {
			eo.approximation = APPROXIMATION_EXACT;
			po.number_fraction_format = FRACTION_DECIMAL_EXACT;

		} else if (*mode == MODE_HIGH_PRECISION) {
			precision = PRECISION_HIGH;
			po.indicate_infinite_series = false;
		}
	}

	CALCULATOR->setPrecision(precision);

	MathStructure result;
    stringstream expressions(expression);
    string single_expression;
    int line_number = 1;
    int return_value;
    while (getline(expressions, single_expression, '\n')) {
        if ((return_value = evaluate_single(&eo, &result, line_number, single_expression)))
        	return return_value;
        line_number++;
    }

	string string_result = CALCULATOR->print(result, TIMEOUT_PRINT, po, false, 1, TAG_TYPE_TERMINAL);

	if (string_result.ends_with(CALCULATOR->timedOutString())) {
		return ETIMEOUT;

	} else {
		TYPE_RESULT;
		if (approximate) {
			RESULT_APPROXIMATION_YES;
		} else {
			RESULT_APPROXIMATION_NO;
		}

		fputs(string_result.c_str(), stdout);
	}
	SEPARATOR;
	return 0;
}

int update() {
	if (!CALCULATOR->canFetch())
		return ECANTFETCH;
	CALCULATOR->fetchExchangeRates(TIMEOUT_UPDATE);
	// for some reason this returns false even when it's successful so I'm not going to check it
	return 0;
}

int main(int argc, char** argv) {
#ifdef UID
	if (setgroups(0, {})) {
		perror("couldn't remove groups");
		abort();
	}

	if (setresgid(UID, UID, UID)) {
		perror("couldn't set gid");
		abort();
	}

	if (setresuid(UID, UID, UID)) {
		perror("couldn't set uid");
		abort();
	}

	capng_clear(CAPNG_SELECT_BOTH);
	if (capng_update(CAPNG_DROP, (capng_type_t)(CAPNG_EFFECTIVE | CAPNG_PERMITTED), CAP_SETGID)) {
		printf("couldn't drop caps: can't select\n");
		abort();
	}
	int err = capng_apply(CAPNG_SELECT_BOTH);
	if (err) {
		printf("couldn't drop caps: %d\n", err);
		abort();
	}
#endif

	if (argc < 2)
		return ENOARG;

	new Calculator(true);
	if (argc == 2) {
		if (strcmp(argv[1], COMMAND_UPDATE) == 0)
			return update();
		else
			return 1;
	} else {
		return evaluate(argv[1], argv[2], argv[3]);
	}
}

