## Makefile.am -- Process this file with automake to produce Makefile.in
# $Id: Makefile.am,v 1.12 2010/08/11 17:02:59 bcx Exp $


INCLUDES		= -I. -Iinclude -Iinclude/include -Iinclude/curl
bin_PROGRAMS		= onepixd
onepixd_SOURCES		= onepixd.c \
			  threads.c \
			  files.c \
			  html.c xml.c \
			  str.c log.c \
			  base64.c verify.c io.c utils.c \
			  gif.c conf.c readconf.c\
			  debug.c hash.c
SUBDIRS			= libtpool
onepixd_LDADD		= libtpool/libtpool.a 


auxdir		= @ac_aux_dir@
AUX_DIST	= $(auxdir)/install-sh $(auxdir)/missing \
		  $(auxdir)/mkinstalldirs \
		  $(auxdir)/config.guess $(auxdir)/build \
		  $(auxdir)/dh.m4

EXTRA_DIST	= tests/test_gif_requests.html tests/test_join_post_upload.html \
		  favicon.ico libtpool/tpool.h onepixd.h onepixd.conf makekey.sh \
		  README tools/Makefile tools/runtest.sh tools/sign_get_function.c \
		  tools/sign_get_function.php run.sh docs/Onepixd_Manual.pdf

AUTOMAKE_OPTIONS 	= foreign
MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure \
			  $(AUX_DIST)

ACLOCAL		= aclocal -I $(auxdir)

dist-hook:	

install-exec-local:


