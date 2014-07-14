dnl Process this file with autoconf to produce a configure script
# $Id: configure.ac,v 1.15 2010/08/11 17:03:00 bcx Exp $
# 
# Acceptable arguments to configure are:
# 	The usual CC= etc.
#	--prefix=/path define where to install this library
# 	--with-wall forces CC=gcc -Wall
#	--with-static make a statically linked build
# 
# @start1


AC_INIT(onepixd, [1.0.4], [onepixd@bcx.com])
AC_COPYRIGHT([Copyright (c) 2010 Bryan Costales])
AC_REVISION([1.0.4])

CPPFLAGS="$CPPFLAGS -I."

if test -d /usr/local/ssl
then
	SSLBASE="/usr/local/ssl"
else
	SSLBASE="/usr"
fi
CPPFLAGS="$CPPFLAGS -I${SSLBASE}/include"
LDFLAGS="$LDFLAGS -L${SSLBASE}/lib -lssl -lcrypto"

AC_CHECK_HEADERS(openssl/err.h)
AC_CHECK_HEADERS(openssl/sha.h)
AC_CHECK_HEADERS(openssl/evp.h)
AC_CHECK_HEADERS(openssl/ssl.h)
AC_CHECK_HEADERS(openssl/bio.h)
AC_CHECK_HEADERS(openssl/x509v3.h)
AC_CHECK_HEADERS(openssl/asn1.h)
AC_CHECK_HEADERS(openssl/rand.h)
AC_CHECK_HEADERS(openssl/opensslv.h)
AC_CHECK_HEADERS(openssl/rsa.h)
AC_CHECK_HEADERS(openssl/pem.h)
AC_CHECK_HEADERS(openssl/md5.h)

AC_PREREQ(2.52)
AC_CONFIG_SRCDIR(.)
AC_CONFIG_AUX_DIR(build)
AC_CANONICAL_SYSTEM
AC_PROG_MAKE_SET
AM_INIT_AUTOMAKE(onepixd, [1.0.4])

AC_PROG_CC(gcc cc)
AM_CONFIG_HEADER(config.h)

AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS(assert.h)
AC_CHECK_HEADERS(string.h fcntl.h memory.h malloc.h unistd.h ctype.h)
AC_CHECK_HEADERS(strings.h stdio.h)
AC_CHECK_HEADERS(sys/stat.h sys/param.h sys/file.h stdlib.h)
AC_CHECK_HEADERS(sys/socket.h netinet/in.h netdb.h sys/ioctl.h)
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(arpa/inet.h time.h signal.h errno.h)
AC_CHECK_HEADERS(stdio.h)
AC_CHECK_HEADERS(sys/socket.h netinet/in.h netdb.h sys/ioctl.h arpa/nameser.h resolv.h)
AC_CHECK_HEADERS(idna.h)
AC_CHECK_HEADERS(stdarg.h)
AC_CHECK_HEADERS(malloc.h)
AC_CHECK_HEADERS(errno.h)
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(ctype.h)
AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(search.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(syslog.h)
AC_CHECK_HEADERS(sys/timeb.h)
AC_CHECK_HEADERS(pthread.h)
AC_CHECK_HEADERS(windows.h)
AC_CHECK_HEADERS(pwd.h)
AC_CHECK_HEADERS(grp.h)
AC_CHECK_HEADERS(dirent.h)
AC_CHECK_HEADERS(alloca.h)
AC_CHECK_HEADERS(resource.h)
AC_CHECK_HEADERS(sys/resource.h)
AC_CHECK_HEADERS(stdarg.h)
AC_CHECK_HEADERS(varargs.h)
AC_CHECK_HEADERS(sys/varargs.h)
AC_CHECK_HEADERS(poll.h)
AC_CHECK_HEADERS(alloca.h)
AC_CHECK_HEADERS(wait.h)
AC_CHECK_HEADERS(sys/wait.h)
AC_CHECK_HEADERS(sys/int_limts.h)
AC_CHECK_HEADERS(curl.h)
AC_CHECK_HEADERS(libgen.h)
AC_CHECK_HEADERS(netinet/tcp.h)
AC_CHECK_HEADERS(db.h)
AC_CHECK_HEADERS(utime.h)
CFLAGS="$CFLAGS -Iinclude"

AC_CHECK_FUNCS(strlcpy)
AC_CHECK_FUNCS(strlcat)
AC_CHECK_FUNCS(closesocket)

AC_C_BIGENDIAN
AC_STRUCT_TM
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(void *)

#
# We need this because IDs are 32 bit.
#
if test "$ac_cv_sizeof_int" = "4"; then
AC_CHECK_TYPE(int32_t, int)
AC_CHECK_TYPE(uint32_t, unsigned int)
elif test "$ac_cv_sizeof_short" = "4"; then
AC_CHECK_TYPE(int32_t, short)
AC_CHECK_TYPE(uint32_t, unsigned short)
elif test "$ac_cv_sizeof_long" = "4"; then
AC_CHECK_TYPE(int32_t, long)
AC_CHECK_TYPE(uint32_t, unsigned long)
else
AC_CHECK_TYPE(int32_t, 'cannot determine 32 bit size')
fi

AC_CHECK_LIB(malloc, bsdmalloc)
AC_FUNC_MALLOC

AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(fopen)
AC_CHECK_FUNCS(fgets)
AC_CHECK_FUNCS(strrchr)
AC_CHECK_FUNCS(strdup)
AC_CHECK_FUNCS(strcasecmp)
AC_CHECK_FUNCS(strncasecmp)
AC_CHECK_LIB(socket, connect)
AC_CHECK_LIB(resolv, res_query)
AC_CHECK_LIB(resolv, res_init)
AC_CHECK_LIB(resolv, dn_expand)
AC_CHECK_LIB(idn, idn_free)
AC_CHECK_LIB(rt, nanosleep)

AC_PROG_LN_S
AC_PROG_RANLIB
AC_SUBST(ac_aux_dir)

AC_MSG_CHECKING([for system type])
case "$host_os" in
*linux*)
	AC_DEFINE([OS_LINUX],1,[Linux])
	AC_MSG_RESULT("Linux")
	#
	# These should be in the C-library
	AC_CHECK_FUNCS(getaddrinfo getnameinfo)
	LDFLAGS="$LDFLAGS -D_REENTRANT"
	CFLAGS="$CFLAGS -D_REENTRANT"
	LIBS="$LIBS -ldl"
	;;
*solaris*)
	AC_DEFINE([OS_SOLARIS],1,[Solaris])
	AC_MSG_RESULT("Solaris")
	if test "$GCC" != yes; then
		AC_DEFINE([__STDC__],1,[define on solaris if using gcc])
	fi
	AC_CHECK_LIB(nsl, getnameinfo)
	AC_CHECK_LIB(rt, nanosleep)
	;;
*darwin*)
	AC_DEFINE([OS_DARWIN],1,[Mac OS X])
	AC_MSG_RESULT("Darwin")
	#
	# These should be in the C-library
	AC_CHECK_FUNCS(getaddrinfo getnameinfo)
	;;
*freebsd*)
	AC_DEFINE([OS_FREEBSD],1,[FreeBSD])
	AC_MSG_RESULT("FreeBSD")
	AC_CHECK_FUNCS(getaddrinfo getnameinfo)
	;;
*ultrix*)
	AC_DEFINE([OS_ULTRIX],1,[Ultrix])
	AC_DEFINE(NOPROTO,,1)
	AC_DEFINE(NOCONST,,1)
	AC_MSG_RESULT("ultrix")
	AC_CHECK_FUNCS(getaddrinfo getnameinfo)
	;;
esac

AC_ARG_WITH([wall],
	AC_HELP_STRING([--with-wall], [(no default)]),
	CCADD="-Wall",)
CC="$CC $CCADD"
AC_ARG_WITH([static],
	AC_HELP_STRING([--with-static], [(no default)]),
	CCADD="-static -static-libgcc",)
CC="$CC $CCADD"


# ALWAYS_DEFINES="-D_REENTRANT"
SAVE_LIBS="$LIBS"  
SAVE_CFLAGS="$CFLAGS"
SAVE_CC="$CC"
ACX_PTHREAD()
LIBS="$PTHREAD_LIBS $SAVE_LIBS"
CFLAGS="$SAVE_CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"
if test "$host_os" = "solaris"
then
	CFLAGS="`echo $CFLAGS | sed -e 's/-pthread//'`"
fi

AC_OUTPUT([ Makefile libtpool/Makefile ],
[test -z "$CONFIG_HEADERS" || echo timestamp > stamp-h.in])
# @end1
