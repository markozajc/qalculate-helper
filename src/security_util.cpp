//SPDX-License-Identifier: GPL-3.0

#include <libqalculate/Function.h>
#include <libqalculate/Number.h>
#include <libqalculate/QalculateDateTime.h>
#include <libqalculate/Variable.h>
#include <linux/capability.h>
#include <security_util.h>
#include <sys/time.h>
#include <unistd.h>
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
	::destroy_if_exists(calc.getActiveFunction("command")); // rce
	::destroy_if_exists(calc.getActiveFunction("plot")); // doesn't work headless, possibly rce
	::destroy_if_exists(calc.getActiveVariable("uptime")); // information leakage
	::destroy_if_exists(calc.getActiveVariable("export")); // lfi
	::destroy_if_exists(calc.getActiveVariable("load")); // lfi
}
#else
void do_defang_calculator(Calculator&) {}
#endif

void do_setuid() {
#ifdef SETUID
	if (::setgroups(0, {})) {
		::perror("couldn't remove groups");
		::abort();
	}

	if (::setresgid(SETUID_GID, SETUID_GID, SETUID_GID)) {
		::perror("couldn't set gid");
		::abort();
	}

	if (::setresuid(SETUID_UID, SETUID_UID, SETUID_UID)) {
		::perror("couldn't set uid");
		::abort();
	}

	::capng_clear(CAPNG_SELECT_BOTH);
	if (::capng_update(CAPNG_DROP, static_cast<capng_type_t>(CAPNG_EFFECTIVE | CAPNG_PERMITTED), CAP_SETGID)) {
		::perror("couldn't drop caps: can't select capabilities to drop\n");
		::abort();
	}
	auto err = ::capng_apply(CAPNG_SELECT_BOTH);
	if (err) {
		::printf("couldn't drop caps: %d\n", err);
		::abort();
	}
#endif
}

#ifdef SECCOMP
static int now_year; // NOSONAR no way around this
static int now_month; // NOSONAR ditto
static int now_day; // NOSONAR ditto
static suseconds_t now_usec; // NOSONAR ditto
static std::time_t now_sec; // NOSONAR ditto

// "now", "today", "yesterday", etc. all depend on the openat syscall. I've opted to instead get the time before
// seccomping and then return it by replacing these two functions. This does seem dirty and requires an extra linker
// flag when doing a static link, but it does work.
void QalculateDateTime::setToCurrentDate() { // NOSONAR overriding an external function
	parsed_string.clear();
	set(::now_year, ::now_month, ::now_day);
}

void QalculateDateTime::setToCurrentTime() { // NOSONAR overriding an external function
	parsed_string.clear();
	Number nr(::now_usec, 0, -6);
	nr += ::now_sec;
	set(nr);
}

static void checked_seccomp_rule_add(scmp_filter_ctx ctx, unsigned int action, int syscall) {
	auto err = ::seccomp_rule_add(ctx, action, syscall, 0);
	if (err) {
		::printf("couldn't seccomp: %d\n", err);
		::abort();
	}
}

void do_seccomp() {
	std::time_t rawtime = 0;
	if (!std::time(&rawtime)) {
		::perror("couldn't get time");
		::abort();
	}
	std::tm local_tmdate;
	std::tm tmdate = *::localtime_r(&rawtime, &local_tmdate);
	::now_year = tmdate.tm_year + 1900;
	::now_month = tmdate.tm_mon + 1;
	::now_day = tmdate.tm_mday;

	timeval tv;
	if (::gettimeofday(&tv, nullptr)) {
		::perror("couldn't gettimeofday");
		::abort();
	}
	::now_usec = tv.tv_usec;
	::now_sec = tv.tv_sec;

	if (::setenv("QALCULATE_USER_DIR", "/", 1)) {
		::perror("couldn't setenv");
		::abort();
	}
	// Despite having global definitions compiled in, libqalculate will still attempt to load local definitions in some
	// circumstances (e.g. when using a dataset function such as Planets). This results in a call to util.cc:buildPath(),
	// which will in turn result to a nasty getpwuid() and getuid() if QALCULATE_USER_DIR, XDG_DATA_HOME are unset. These
	// calls in turn require openat and getcwd syscalls respectively, which can be avoided by simply setting either of
	// these environment variables to a bogus value.

#ifdef ENABLE_DEBUG
	scmp_filter_ctx ctx = ::seccomp_init(SCMP_ACT_LOG);
#else
	scmp_filter_ctx ctx = ::seccomp_init(SCMP_ACT_KILL_PROCESS);
#endif
	/*   0 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read));
	/*   1 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write));
	/*   3 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close));
	/*   5 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat));
	/*   9 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap));
	/*  10 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect));
	/*  11 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap));
	/*  12 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk));
	/*  13 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigaction));
	/*  14 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigprocmask));
	/*  24 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sched_yield));
	/*  28 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(madvise));
	/*  56 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone)); // required by docker
	/*  60 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit));
	/* 202 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex));
	/* 230 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clock_nanosleep));
	/* 231 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group));
	/* 262 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat));
	/* 273 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_robust_list));
	/* 334 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rseq));
	/* 435 */::checked_seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone3));

	auto err = ::seccomp_load(ctx);
	if (err) {
		::printf("couldn't seccomp: %d\n", err);
		::abort();
	}
}
#else
void do_seccomp() {}
#endif
