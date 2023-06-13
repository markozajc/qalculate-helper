include .build_env
export

NAME='qalculate-helper'
FLAGS+=-I${QALCULATE_INCLUDE_PATH}
FLAGS+=-L${QALCULATE_LIBRARY_PATH}
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
FLAGS+=-Wall
FLAGS+=-Wextra
FLAGS+=-ansi
FLAGS+=-O3
FLAGS+=-D_FORTIFY_SOURCE=2
FLAGS+=-fPIE
FLAGS+=-std=c++2a
FLAGS+=-o $(NAME)
FLAGS+=-march=native
ifneq ($(origin SETUID), undefined)
	FLAGS+=-DUID=${SETUID}
endif
ifneq ($(origin SECCOMP), undefined)
	FLAGS+=-DSECCOMP
endif

all:
	g++ qalculate-helper.cpp $(FLAGS)
	strip $(NAME)
