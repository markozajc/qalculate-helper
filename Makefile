include .build_env
export

NAME='qalculate-helper'
FLAGS+=-Iinclude/
ifneq ($(origin QALCULATE_INCLUDE_PATH), undefined)
	FLAGS+=-I${QALCULATE_INCLUDE_PATH}
endif
ifneq ($(origin QALCULATE_LIBRARY_PATH), undefined)
	FLAGS+=-L${QALCULATE_LIBRARY_PATH}
endif
FLAGS+=-fuse-ld=mold
FLAGS+=-Wl,-z muldefs
FLAGS+=-Wl,-z now
FLAGS+=-Wl,-z relro
FLAGS+=-Wl,-Bstatic
FLAGS+=-lqalculate
FLAGS+=-Wl,-Bdynamic
FLAGS+=-lmpfr
FLAGS+=-lgmp
FLAGS+=-licuuc
FLAGS+=-lxml2
FLAGS+=-lcurl
FLAGS+=-lpthread
ifneq ($(origin SETUID), undefined)
	FLAGS+=-lcap-ng
endif
ifneq ($(origin SECCOMP), undefined)
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
ifneq ($(origin SECCOMP), undefined)
	ifeq ($(SECCOMP), 1)
		FLAGS+=-DSECCOMP
	endif
endif
ifneq ($(origin SECCOMP_ALLOW_CLONE), undefined)
	ifeq ($(SECCOMP_ALLOW_CLONE), 1)
		FLAGS+=-DSECCOMP_ALLOW_CLONE
	endif
endif

all:
	g++ src/security_util.cpp src/exchange_update_exception.cpp src/qalculate_exception.cpp src/qalculate-helper.cpp src/timeout_exception.cpp $(FLAGS)
	strip $(NAME)
