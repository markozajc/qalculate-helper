include .build_env
export

NAME='qalculate-helper'
FLAGS+=-Iinclude/
FLAGS+=-I${LIBQALCULATE_PREFIX}/include
FLAGS+=-Wl,-z now
ifeq ($(LIBQALCULATE_STATIC_LINK), 1)
	FLAGS+=-Wl,-z muldefs
	FLAGS+=${LIBQALCULATE_PREFIX}/lib/libqalculate.a
else
	FLAGS+=-L${LIBQALCULATE_PREFIX}/lib
	FLAGS+=-lqalculate
endif
FLAGS+=-lmpfr
FLAGS+=-lgmp
FLAGS+=-licuuc
FLAGS+=-lxml2
FLAGS+=-lcurl
FLAGS+=-lpthread
ifneq ($(origin SETUID), undefined)
	FLAGS+=-lcap-ng
endif
ifeq ($(SECCOMP), 1)
	FLAGS+=-lseccomp
endif
FLAGS+=-Wall -Wextra -Wformat
FLAGS+=-ansi
FLAGS+=-O3
FLAGS+=-D_FORTIFY_SOURCE=2
FLAGS+=-fPIE -pie
FLAGS+=-fstack-protector-strong
FLAGS+=-std=c++17
FLAGS+=-o $(NAME)
FLAGS+=-march=native
ifneq ($(origin SETUID), undefined)
	FLAGS+=-DUID=${SETUID}
endif
ifeq ($(SECCOMP), 1)
	FLAGS+=-DSECCOMP
endif
ifeq ($(SECCOMP_ALLOW_CLONE), 1)
	FLAGS+=-DSECCOMP_ALLOW_CLONE
endif

all:
	g++ src/security_util.cpp src/exchange_update_exception.cpp src/qalculate_exception.cpp src/qalculate-helper.cpp src/timeout_exception.cpp $(FLAGS)
	strip $(NAME)
