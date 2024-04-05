//SPDX-License-Identifier: GPL-3.0

#include <libqalculate/Function.h>
#include <libqalculate/Variable.h>
#include <security_util.h>

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

void do_setuid() {
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
}

void do_defang_calculator(Calculator *calc) {
	calc->getActiveFunction("command")->destroy(); // rce
#ifdef HAS_PLOT
	calc->getActiveFunction("plot")->destroy(); // wouldn't work
#endif
	calc->getActiveVariable("uptime")->destroy(); // information leakage
}

void do_seccomp() {
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
}
