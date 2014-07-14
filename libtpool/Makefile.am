## Makefile.am -- Process this file with automake to produce Makefile.in
# $Id: Makefile.am,v 1.1.1.1 2010/06/22 23:57:46 bcx Exp $


INCLUDES		= -I.
nobase_pkglib_LIBRARIES	= libtpool.a
libtpool_a_SOURCES	= tpool_delta.c tpool_init.c \
			  tpool_shutdown.c tpool_tasks.c \
			  tpool_worker.c tpool_debug.c

auxdir		= @ac_aux_dir@
AUX_DIST	= $(auxdir)/install-sh $(auxdir)/missing \
		  $(auxdir)/mkinstalldirs \
		  $(auxdir)/config.guess $(auxdir)/build \
		  $(auxdir)/dh.m4


AUTOMAKE_OPTIONS 	= foreign
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure \
			  $(AUX_DIST)

ACLOCAL		= aclocal -I $(auxdir)

dist-hook:	

install-exec-local:


