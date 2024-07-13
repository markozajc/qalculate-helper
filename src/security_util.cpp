//SPDX-License-Identifier: GPL-3.0

#include <libqalculate/Function.h>
#include <libqalculate/Number.h>
#include <libqalculate/QalculateDateTime.h>
#include <libqalculate/Variable.h>
#include <security_util.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#ifdef SETUID
#include <grp.h>
#include <cap-ng.h>
#endif

#ifdef SECCOMP
#include <seccomp.h>
#endif

#ifdef LIBQALCULATE_DEFANG
static void destroy_if_exists(ExpressionItem *item) {
	if (item)
		item->destroy();
}

void do_defang_calculator(Calculator &calc) {
	destroy_if_exists(calc.getActiveFunction("command")); // rce
	destroy_if_exists(calc.getActiveFunction("plot")); // wouldn't work, possible rce
	destroy_if_exists(calc.getActiveVariable("uptime")); // information leakage
	destroy_if_exists(calc.getActiveVariable("export")); // lfi
	destroy_if_exists(calc.getActiveVariable("load")); // lfi
}
#else
void do_defang_calculator(Calculator&) {}
#endif

void do_setuid() {
#ifdef SETUID
	if (setgroups(0, {})) {
		perror("couldn't remove groups");
		abort();
	}

	if (setresgid(SETUID_GID, SETUID_GID, SETUID_GID)) {
		perror("couldn't set gid");
		abort();
	}

	if (setresuid(SETUID_UID, SETUID_UID, SETUID_UID)) {
		perror("couldn't set uid");
		abort();
	}

	capng_clear(CAPNG_SELECT_BOTH);
	if (capng_update(CAPNG_DROP, static_cast<capng_type_t>(CAPNG_EFFECTIVE | CAPNG_PERMITTED), CAP_SETGID)) {
		perror("couldn't drop caps: can't select capabilities to drop\n");
		abort();
	}
	int err = capng_apply(CAPNG_SELECT_BOTH);
	if (err) {
		printf("couldn't drop caps: %d\n", err);
		abort();
	}
#endif
}

#ifdef SECCOMP
static int now_year;
static int now_month;
static int now_day;
static suseconds_t now_usec;
static std::time_t now_sec;

// "now", "today", "yesterday", etc. all depend on the openat syscall. I've opted to instead get the time before
// seccomping and then return it by replacing these two functions. This does seem dirty and requires an extra linker
// flag when doing a static link, but it does work.
void QalculateDateTime::setToCurrentDate() {
	parsed_string.clear();
	set(now_year, now_month, now_day);
}

void QalculateDateTime::setToCurrentTime() {
	parsed_string.clear();
	Number nr(now_usec, 0, -6);
	nr += now_sec;
	set(nr);
}

void do_seccomp() {
	struct std::tm tmdate;
	std::time_t rawtime;
	std::time(&rawtime);
	tmdate = *localtime(&rawtime);
	now_year = tmdate.tm_year + 1900;
	now_month = tmdate.tm_mon + 1;
	now_day = tmdate.tm_mday;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	now_usec = tv.tv_usec;
	now_sec = tv.tv_sec;

	setenv("QALCULATE_USER_DIR", "/", 1);
	// Despite having global definitions compiled in, libqalculate will still attempt to load local definitions in some
	// cirsumstances (e.g. when using a dataset function such as Planets). This results in a call to util.cc:buildPath(),
	// which will in turn result to a nasty getpwuid() and getuid() if QALCULATE_USER_DIR, XDG_DATA_HOME are unset. These
	// calls in turn require openat and getcwd syscalls respectively, which can be avoided by simply setting either of
	// these environment variables to a bogus value.

	scmp_filter_ctx ctx;
#ifdef ENABLE_DEBUG
	ctx = seccomp_init(SCMP_ACT_LOG);
#else
	ctx = seccomp_init(SCMP_ACT_KILL_PROCESS);
#endif
	/*   0 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
	/*   1 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
	/*   3 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
	/*   9 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
	/*  10 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
	/*  11 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
	/*  12 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
	/*  13 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction), 0);
	/*  14 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask), 0);
	/*  24 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sched_yield), 0);
	/*  28 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(madvise), 0);
	/*  56 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone), 0); // required by docker
	/*  60 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
	/* 202 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
	/* 230 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_nanosleep), 0);
	/* 231 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
	/* 262 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);
	/* 273 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_robust_list), 0);
	/* 334 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rseq), 0);
	/* 435 */seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone3), 0);

	int err = seccomp_load(ctx);
	if (err) {
		printf("couldn't seccomp: %d\n", err);
		abort();
	}
}
#else
void do_seccomp() {}
#endif
