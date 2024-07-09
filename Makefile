include .build_env
export

NAME='qalculate-helper'
FLAGS+=-Iinclude/
FLAGS+=-I${LIBQALCULATE_PREFIX}/include
FLAGS+=-Wl,-z now
ifeq ($(LIBQALCULATE_STATIC_LINK), 1)
	FLAGS+=-Wl,-z muldefs
	ifneq ($(origin LIBQALCULATE_PREFIX), undefined)
		STATIC_LIBS+=${LIBQALCULATE_PREFIX}/lib/libqalculate.a
	else
		FLAGS+=-Wl,-Bstatic
		FLAGS+=-lqalculate
		FLAGS+=-Wl,-Bdynamic
	endif
else
	ifneq ($(origin LIBQALCULATE_PREFIX), undefined)
		FLAGS+=-L${LIBQALCULATE_PREFIX}/lib
	endif
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

ifeq ($(DEBUG), 1)
	FLAGS+=-O3
	FLAGS+=-D_FORTIFY_SOURCE=2
	FLAGS+=-fPIE -pie
	FLAGS+=-fstack-protector-strong
	FLAGS+=-march=native
else
	FLAGS+=-O0
	FLAGS+=-g3
endif

FLAGS+=-std=c++17
FLAGS+=-o $(NAME)
ifneq ($(origin SETUID), undefined)
	FLAGS+=-DUID=${SETUID}
endif
ifeq ($(SECCOMP), 1)
	FLAGS+=-DSECCOMP
endif
ifeq ($(SECCOMP_ALLOW_CLONE), 1)
	FLAGS+=-DSECCOMP_ALLOW_CLONE
endif
ifeq ($(LIBQALCULATE_PRELOAD_DATASETS), 1)
	FLAGS+=-DLIBQALCULATE_PRELOAD_DATASETS
endif
ifeq ($(LIBQALCULATE_DEFANG), 1)
	FLAGS+=-DLIBQALCULATE_DEFANG
endif
ifeq ($(DEBUG), 1)
	FLAGS+=-DENABLE_DEBUG
endif

FILES+=src/security_util.cpp
FILES+=src/exchange_update_exception.cpp
FILES+=src/qalculate_exception.cpp
FILES+=src/qalculate-helper.cpp
FILES+=src/timeout_exception.cpp

all:
	g++ $(FILES) $(STATIC_LIBS) $(FLAGS)
	@if [ $(DEBUG) != 1 ]; then\
		strip $(NAME);\
	fi
