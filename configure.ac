dnl Process this file with autoconf to produce a configure script.

AC_DEFUN([AX_CHECK_COMPILE_FLAG],
[AC_PREREQ(2.59)dnl for _AC_LANG_PREFIX
AS_VAR_PUSHDEF([CACHEVAR],[ax_cv_check_[]_AC_LANG_ABBREV[]flags_$4_$1])dnl
AC_CACHE_CHECK([whether _AC_LANG compiler accepts $1], CACHEVAR, [
  ax_check_save_flags=$[]_AC_LANG_PREFIX[]FLAGS
  _AC_LANG_PREFIX[]FLAGS="$[]_AC_LANG_PREFIX[]FLAGS $4 $1"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
    [AS_VAR_SET(CACHEVAR,[yes])],
    [AS_VAR_SET(CACHEVAR,[no])])
  _AC_LANG_PREFIX[]FLAGS=$ax_check_save_flags])
AS_IF([test x"AS_VAR_GET(CACHEVAR)" = xyes],
  [m4_default([$2], :)],
  [m4_default([$3], :)])
AS_VAR_POPDEF([CACHEVAR])dnl
])dnl AX_CHECK_COMPILE_FLAGS

AC_PREREQ(2.59)
AC_INIT(googleproxy, 2.0)
LT_INIT()

AC_CONFIG_HEADERS(config.h)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CXX
PKG_PROG_PKG_CONFIG


AC_LANG(C++)

AX_CHECK_COMPILE_FLAG([-std=gnu++11],[CXXFLAGS="$CXXFLAGS -std=gnu++11"],[AC_MSG_FAILURE([please use c++11 compiler],[-1])])
AX_CHECK_COMPILE_FLAG([-fpermissive],[CXXFLAGS="$CXXFLAGS -fpermissive"],[AC_MSG_FAILURE([please update c++ compiler],[-1])])

AC_ARG_WITH(googleaddress,[helpstring],[googleaddress=$withval],[googleaddress="74.125.71.94"])

BOOST_REQUIRE(1.48)

AC_ARG_WITH([systemdsystemunitdir],
        AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
        [with_systemdsystemunitdir=$withval],
        [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)]
    )

AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])

if [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ]; then
		 AC_CONFIG_FILES(googleproxy.service.in)
		 AC_DEFINE([HAVE_SYSTEMD],[1],[use systemd])
fi

AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir" -a "x$with_systemdsystemunitdir" != xno ])

AC_ARG_ENABLE([threads],
		AS_HELP_STRING([--enable-threads], [enable multithread support]),
		[ case "${enableval}" in
			yes) enable_threads=yes ;;
			no)  enable_threads=no ;;
			*)   AC_MSG_ERROR(bad value ${enableval} for --enable-threads) ;;
		esac],
		[enable_threads="no"]
	)

if [test -n "${enable_threads}" -a "x${enable_threads}" != xno ]; then
	AC_DEFINE([ENABLE_THREAD],[1],[use thread])

	BOOST_THREADS([s])
fi

AC_DEFINE_UNQUOTED(GOOGLE_ADDRESS,"${googleaddress}",[the address of www.google.com])

AC_CONFIG_FILES(Makefile src/Makefile asio-1.5.3/Makefile asio-1.5.3/include/Makefile)
AC_OUTPUT

