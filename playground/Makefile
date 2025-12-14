CC?=cc
DEBUG_FLAGS?=
DEFINES?=

DEBUG_FLAGS:=-g3
DEFINES:=$(DEFINES) DEBUG
DEFINES:=$(DEFINES) DEBUGPRINT

DEFINES:=$(addprefix -D,$(DEFINES))
CFLAGS=$(DEBUG_FLAGS)

multiwait: multiwait.c common.c common.h
	$(CC) $(CFLAGS) $(DEFINES) $^ -o $@ 
