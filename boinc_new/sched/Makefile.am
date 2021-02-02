## -*- mode: makefile; tab-width: 4 -*-
## $Id$

include $(top_srcdir)/Makefile.incl

AM_CPPFLAGS += $(MYSQL_CFLAGS) $(PTHREAD_CFLAGS)
AM_LDFLAGS += -static


if ENABLE_SERVER

schedcgidir = $(prefix)/lib/boinc-server-maker/sched
schedsharedir = $(prefix)/lib/boinc-server-maker/sched
schedbindir = $(prefix)/lib/boinc-server-maker/sched


noinst_HEADERS = \
    sched_main.h \
    sched_version.h \
    schedq_handle.h \
    sched_types.h

fcgi_file_upload_handler_SOURCES = \
    sched_util_basic.cpp \
    db_base.cpp \
    boinc_db.cpp \
    sched_limit.cpp \
    sched_config.cpp \
    file_upload_handler.cpp
fcgi_file_upload_handler_CPPFLAGS = -D_USING_FCGI_ $(AM_CPPFLAGS)
fcgi_file_upload_handler_LDADD = $(SERVERLIBS_FCGI)

schedcgi_PROGRAMS = schedq fcgi_file_upload_handler
schedq_SOURCES = \
    sched_util_basic.cpp \
    db_base.cpp \
    boinc_db.cpp \
    sched_config.cpp \
    sched_limit.cpp \
    schedq_types.cpp \
    schedq_auth.cpp \
    schedq_misc.cpp \
    schedq_result.cpp \
    schedq_handle.cpp \
    feeder_simple.cpp \
    feeder_array.cpp \
    schedq_main.cpp

schedq_CPPFLAGS = -D_USING_FCGI_ -D_WITHOUT_SSP_ $(AM_CPPFLAGS)
schedq_LDADD = $(SERVERLIBS_FCGI)

endif
# end of "if ENABLE_SERVER"

.PHONY: PHONY-start

PHONY-start:
	@test -f start || @LN_S@ $(srcdir)/start start && test -f start

status stop: PHONY-start
	@test -f $@ || @LN_S@ start $@ && test -f $@

CLEANFILES = status stop *.gcno *.gcda *.gcov
