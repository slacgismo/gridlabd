# File: configure.ac -*- Autoconf -*-
#
# GridLAB-D 4.0
#   Copyright (C) 2008 Battelle Memorial Institute
#
# HiPAS GridLAB-D 4.2
#   Copyright (C) 2020 Regents of the Leland Stanford Junior University
#
# Authors:
#   DP Chassin (dchassin@slac.stanford.edu)
#
# Process this file with autoconf to produce a configure script.
# This file is distributed under the same terms as GridLAB-D.

###############################################################################
# Init autoconf
###############################################################################

dnl m4 magic from Eric Blake <e...@byu.net>
dnl autoconf 2.63 does not have m4_esyscmd_s
m4_define([m4_chomp_all],
[m4_format([[%.*s]], m4_bregexp(m4_translit([[$1]], [
/], [/ ]), [/*$]), [$1])])

m4_define([m4_esyscmd_s], [m4_chomp_all(m4_esyscmd([$1]))])


m4_define([sbuild_m4_chomp],
[m4_format([[%.*s]], m4_bregexp(m4_translit([$1], [
]]m4_dquote(_m4_defn([m4_cr_all]))[, [/]]m4_format([%255s], [])[),
  [/*$]), [$1])])
m4_define([sbuild_m4_esyscmd_s],
[sbuild_m4_chomp(m4_esyscmd([$1]))])

dnl GridLAB-D version information
m4_define([pkg], sbuild_m4_esyscmd_s([build-aux/version.sh --package]))
m4_define([pkgname], sbuild_m4_esyscmd_s([build-aux/version.sh --package-name]))
m4_define([pkgversion], sbuild_m4_esyscmd_s([build-aux/version.sh --version]))
m4_define([pkgbranch],  sbuild_m4_esyscmd_s([build-aux/version.sh --branch]))
m4_define([gitversion], sbuild_m4_esyscmd_s([build-aux/version.sh --git]))
m4_define([versionname], sbuild_m4_esyscmd_s([build-aux/version.sh --name]))
m4_define([pkginstall], sbuild_m4_esyscmd_s([build-aux/version.sh --install]))
m4_define([pkgorigin],sbuild_m4_esyscmd_s([git rev-parse --abbrev-ref HEAD]))

AC_PREREQ([2.63])
AC_INIT(pkgname, pkgversion, [gridlabd@gmail.com], pkg)
AC_CONFIG_SRCDIR([gldcore/gridlabd.h])
AC_CONFIG_HEADER([gldcore/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

###############################################################################
# Init automake
###############################################################################
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

###############################################################################
# Misc. information and package setup.
###############################################################################
GLD_TOP_BUILDDIR="`pwd`"
cd "$srcdir"
GLD_TOP_SRCDIR="`pwd`"
cd "$GLD_TOP_BUILDDIR"
AS_IF([test "$GLD_TOP_BUILDDIR" != "$GLD_TOP_SRCDIR"],
      [AC_MSG_NOTICE([Detected VPATH build])])

AC_CANONICAL_HOST

# Export version info as autoconf variables
AC_SUBST(PACKAGE_BASEVERSION, pkgversion)
AC_SUBST(PACKAGE_BRANCH, pkgbranch)
AC_SUBST(PACKAGE_ORIGIN, pkgorigin)
AC_SUBST(PACKAGE_REVISION, gitversion)

AC_PREFIX_DEFAULT(pkginstall)

AM_SILENT_RULES([yes])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [turn on debugging @<:@default=yes@:>@])],
    [AS_CASE([$enableval],
        [yes], [debug=true],
        [no],  [debug=false],
        [AC_MSG_ERROR([bad value $enableval for --enable-debug])])],
    [debug=true])

AC_ARG_ENABLE([optimization],
    [AS_HELP_STRING([--enable-optimization@<:@=LEVEL@:>@],
                    [turn on optimization @<:@default=0@:>@])],
    [AS_CASE([${enableval}],
        [0|1|2|3|s], [optimization=${enableval}],
        [yes],       [optimization=2],
        [no],        [optimization=0],
        [AC_MSG_ERROR([bad value ${enableval} for --enable-optimization])])],
    [optimization=0])

###############################################################################
# C compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C compiler])
AC_MSG_NOTICE

: ${CFLAGS="-Wall"}
: ${CXXFLAGS="-Wall"}
AC_PROG_CC
AC_GNU_SOURCE   # defines _GNU_SOURCE
AM_PROG_CC_C_O  # required for automake to support subdir-objects

AS_IF([test "x$debug" = "true"], [GLD_CFLAGS="$GLD_CFLAGS -g"])
GLD_CFLAGS="$GLD_CFLAGS"

#--------------------------------------
# Checks for C header files.
#--------------------------------------

#--------------------------------------
# Checks for C typedefs, structures, and compiler characteristics.
#--------------------------------------
AX_PTHREAD
# check whether -lpthread is ok when linking and add it if so
AC_MSG_CHECKING([whether to add -lpthread to PTHREAD_LIBS])
AS_CASE([$PTHREAD_LIBS],
[*-lpthread*], [result=no],
[SAVE_LIBS="$LIBS"
LIBS="$LIBS -lpthread"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM(
[[
#include <pthread.h>
static void routine(void *a) { a = 0; }
static void *start_routine(void *a) { return a; }
void function() {
    pthread_t th;
    pthread_attr_t attr;
    pthread_create(&th, 0, start_routine, 0);
    pthread_join(th, 0);
    pthread_attr_init(&attr);
    pthread_cleanup_push(routine, 0);
    pthread_cleanup_pop(0);
}
]],
[[function();]])],
    [PTHREAD_LIBS="$PTHREAD_LIBS -lpthread"
     result=yes],
    [result=no])
LIBS="$SAVE_LIBS"
])
AC_MSG_RESULT([$result])


#--------------------------------------
# Checks for C type sizes.
#--------------------------------------

#--------------------------------------
# Checks for C library functions.
#--------------------------------------

#--------------------------------------
# Checks for C libraries.
#--------------------------------------

# Check for curses
AX_WITH_CURSES
AS_IF([test "x$ax_cv_curses" = xyes],
      [HAVE_CURSES=yes
       GLD_CPPFLAGS="$GLD_CPPFLAGS -DHAVE_CURSES"],
      [HAVE_CURSES="no (some features are disabled)"])

AC_SUBST([GLD_CFLAGS])

###############################################################################
# C++ compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C++ compiler])
AC_MSG_NOTICE

AC_LANG_PUSH([C++])

AC_PROG_CXX

AS_IF([test "x$debug" = "true"], [GLD_CXXFLAGS="$GLD_CXXFLAGS -g"])
GLD_CXXFLAGS="$GLD_CXXFLAGS"

#--------------------------------------
# Checks for C++ header files.
#--------------------------------------
AC_PATH_XTRA
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([assert.h])
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([curl/curl.h])
AC_CHECK_HEADERS([direct.h])
AC_CHECK_HEADERS([dirent.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([float.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([io.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([list])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([math.h])
AC_CHECK_HEADERS([memory.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([process.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_HEADERS([sched.h])
AC_CHECK_HEADERS([setjmp.h])
AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([stdarg.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([sys/errno.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/signal.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/stat.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/timeb.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([sys/unistd.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([vector])
AC_CHECK_HEADERS([wchar.h])

#--------------------------------------
# Checks for C++ typedefs, structures, and compiler characteristics.
#--------------------------------------
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_STRUCT_TM
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

GLD_CHECK_BUILTIN(
    [__sync_bool_compare_and_swap],
    [],
    [volatile unsigned int value = 0;
     __sync_bool_compare_and_swap(&value, value, 1);],
    [HAVE_SYNC_BOOL_COMPARE_AND_SWAP=yes],
    [HAVE_SYNC_BOOL_COMPARE_AND_SWAP=no])

GLD_CHECK_BUILTIN(
    [__sync_add_and_fetch],
    [],
    [volatile unsigned int value = 0;
     __sync_add_and_fetch(&value, 1);],
    [HAVE_SYNC_ADD_AND_FETCH=yes],
    [HAVE_SYNC_ADD_AND_FETCH=no])

# AX_PTHREAD is a C-only test. We assume its flags work for CXX, as well. But
# we need to introduce a similar PTHREAD_CXX variable.
# More AIX lossage: compile with *_r variant
if test "x$GCC" != xyes; then
    case $host_os in
        aix*)
        AS_CASE(["x/$CXX"],
          [x*/c89|x*/c89_128|x*/c99|x*/c99_128|x*/cc|x*/cc128|x*/xlc|x*/xlc_v6|x*/xlc128|x*/xlc128_v6],
          [#handle absolute path differently from PATH based program lookup
           AS_CASE(["x$CXX"],
             [x/*],
             [AS_IF([AS_EXECUTABLE_P([${CXX}_r])],[PTHREAD_CC="${CXX}_r"])],
             [AC_CHECK_PROGS([PTHREAD_CC],[${CXX}_r],[$CXX])])])
        ;;
    esac
fi
test -n "$PTHREAD_CXX" || PTHREAD_CXX="$CXX"
AC_SUBST([PTHREAD_CXX])

#--------------------------------------
# Checks for C++ type sizes.
#--------------------------------------
AC_CHECK_TYPES([ptrdiff_t])

#--------------------------------------
# Checks for C++ library functions.
#--------------------------------------
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([atexit])
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([floor])
AC_CHECK_FUNCS([ftime])
AC_CHECK_FUNCS([getcwd])
AC_CHECK_FUNCS([gethostbyname])
AC_CHECK_FUNCS([gethrtime])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([inet_ntoa])
AC_CHECK_FUNCS([localeconv])
AC_CHECK_FUNCS([memchr])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([modf])
AC_CHECK_FUNCS([munmap])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([putenv])
AC_CHECK_FUNCS([rmdir])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_FUNCS([strcasecmp])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strcspn])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strndup])
AC_CHECK_FUNCS([strpbrk])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_FUNCS([strspn])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])
AC_CHECK_FUNCS([tzset])

#--------------------------------------
# Checks for C++ function-like macros.
#--------------------------------------

# isfinite
AC_CACHE_CHECK([for isfinite], [gld_cv_func_isfinite],
    [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <math.h>]],
            [[int result = isfinite(1.0);]])],
        [gld_cv_func_isfinite=yes],
        [gld_cv_func_isfinite=no])])
AS_IF([test "x$gld_cv_func_isfinite" = xyes],
    [AC_DEFINE([HAVE_ISFINITE], [1],
        [Define to 1 if you have the isfinite function.])])

# CPU_ALLOC
AC_CACHE_CHECK([for CPU_ALLOC], [gld_cv_func_CPU_ALLOC],
    [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <sched.h>]],
            [[cpu_set_t *cpu = CPU_ALLOC(2);]])],
        [gld_cv_func_CPU_ALLOC=yes],
        [gld_cv_func_CPU_ALLOC=no])])
AS_IF([test "x$gld_cv_func_CPU_ALLOC" = xyes],
    [AC_DEFINE([DYN_PROC_AFFINITY],[1],
        ["Check for existence of dynamic proc functions"])])
GLD_GET_NPROCS

# Check for the Linux functions for controlling processor affinity.
# LINUX: sched_setaffinity
# AIX:   bindprocessor
# OSX (Leopard): thread_policy_set
AC_CHECK_FUNCS([sched_setaffinity])
AC_CHECK_FUNCS([sched_getaffinity])
AC_CHECK_FUNCS([bindprocessor])
AC_CHECK_FUNCS([thread_policy_set])
AC_CACHE_CHECK([whether cpu_set_t available],
    [gld_cv_have_cpu_set_t],
    [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <sched.h>]],
            [[cpu_set_t t;]])],
        [gld_cv_have_cpu_set_t=yes],
        [gld_cv_have_cpu_set_t=no])])
AS_IF([test "$gld_cv_have_cpu_set_t" = "yes"],
    [AC_DEFINE([HAVE_CPU_SET_T], [1],
        [Define if cpu_set_t is defined in sched.h])])
AC_CACHE_CHECK([whether the CPU_SET and CPU_ZERO macros are defined],
    [gld_cv_cpu_set_defined],
    [AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <sched.h>]],
            [[cpu_set_t t; CPU_ZERO(&t); CPU_SET(1,&t);]])],
        [gld_cv_cpu_set_defined=yes],
        [gld_cv_cpu_set_defined=no])])
AS_IF([test "$gld_cv_cpu_set_defined" = "yes"],
    [AC_DEFINE([HAVE_CPU_SET_MACROS], [1],
        [Define if CPU_SET and CPU_ZERO defined])])
AC_CACHE_CHECK([whether thread affinity macros defined],
    [gld_cv_have_thread_affinity_policy],
    [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <mach/thread_policy.h>]],
[[
#if !defined(THREAD_AFFINITY_POLICY) || !defined(THREAD_AFFINITY_TAG_NULL)
    :'thread macros not defined
#endif
]],
        [gld_cv_have_thread_affinity_policy=yes],
        [gld_cv_have_thread_affinity_policy=no])])])
AS_IF([test "$gld_cv_have_thread_affinity_policy" = "yes"],
    [AC_DEFINE([HAVE_OSX_THREAD_AFFINITY], [1],
        [Define is the OSX thread affinity policy macros defined])])

#--------------------------------------
# Checks for C++ libraries.
#--------------------------------------

AC_LANG_POP([C++])

###############################################################################
# Libtool
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Libtool])
AC_MSG_NOTICE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_PREREQ([2.2])
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_RANLIB # (DPC 2019-09-15) don't remove this, doing so will cause a warning in autoscan instead of autoreconf
LT_INIT([dlopen win32-dll shared disable-static])
AC_SUBST([LIBTOOL_DEPS])

AS_CASE([$host_os],
[darwin*],
    [LIBEXT=.so],
[cygwin*],
    [LIBEXT=.a],
[mingw*],
    [LIBEXT=.dll],
[LIBEXT=.so
 GLD_LDFLAGS="$GLD_LDFLAGS -Wl,--no-undefined"])

AC_DEFINE_UNQUOTED([DLEXT], ["$LIBEXT"], [Dynamic library extension])

AC_SUBST([LIBEXT])
AC_SUBST([GLD_LDFLAGS])
AC_SUBST([GLD_LIBS])

###############################################################################
# Checks for additional programs.
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Checks for additional programs])
AC_MSG_NOTICE

AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

DX_INIT_DOXYGEN(pkg, [['$(top_srcdir)/doxygen/gridlabd.conf']])
DX_HTML_FEATURE([ON])
DX_CHM_FEATURE([OFF])
DX_CHI_FEATURE([OFF])
DX_MAN_FEATURE([OFF])
DX_RTF_FEATURE([OFF])
DX_XML_FEATURE([OFF])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([OFF])
AS_IF([test -n "$DX_DOXYGEN"],
      [HAVE_DOXYGEN="$DX_DOXYGEN ($($DX_DOXYGEN --version))"],
      [HAVE_DOXYGEN=no])
AM_CONDITIONAL([HAVE_DOXYGEN], [test ! "x$HAVE_DOXYGEN" = "xno"])

# Check for armadillo
AS_IF([test -f /usr/include/armadillo -o -f /usr/local/include/armadillo ],
    [HAVE_ARMA="yes"],
    [HAVE_ARMA="no (required)"])

# Check for mysql
AS_IF([test -f /usr/local/include/mysql/mysql.h -a /usr/local/lib/libmysqlclient.a ],
    [MYSQL="/usr/local"],
    [MYSQL="no"])
AS_IF([test "x$MYSQL" != "xno"],
	[MYSQL_CPPFLAGS="'-I$MYSQL/include/mysql'"
		AS_CASE([$host],
			[*mingw*], [MYSQL_LDFLAGS="'-L$MYSQL/lib'"
				MYSQL_LIBS="-lmysql"],
			[*linux*], [MYSQL_LDFLAGS="'-L$MYSQL/lib'"
				MYSQL_LIBS="-lmysqlclient"],
			[*darwin*], [MYSQL_LDFLAGS="'-L$MYSQL/lib'"
				MYSQL_LIBS="-lmysqlclient"])
		GLD_CPPFLAGS="$GLD_CPPFLAGS -DHAVE_MYSQL"
		HAVE_MYSQL=yes],
	[HAVE_MYSQL=no])
AC_SUBST([MYSQL_CPPFLAGS])
AC_SUBST([MYSQL_LDFLAGS])
AC_SUBST([MYSQL_LIBS])
AM_CONDITIONAL([HAVE_MYSQL], [test ! "x$HAVE_MYSQL" = "xno"])

# Verify python version
AM_PATH_PYTHON(3.7)
HAVE_PYTHON="$(python3 --version | cut -f2 -d' ')"
PYTHON_EXEC="$(python3 -c "import os; print(os.path.realpath('$(which python3)'))")"
PYTHON_VER=$(echo $HAVE_PYTHON | cut -f1-2 -d.)
AM_CONDITIONAL([HAVE_PYTHON], [test ! "x$HAVE_PYTHON" = "xno"])
AS_IF([test "x$HAVE_PYTHON" != "xno" -a "$(uname)" == "Darwin" ],
    [PYTHON_LIB="-lpython${PYTHON_VER/3.7/3.7m} -shared -Wl-undefined,dynamic_lookup"],
    [PYTHON_LIB="-lpython${PYTHON_VER/3.7/3.7m}"])

# check for mono
AS_IF([test -z "$(which mono)" ],
    [HAVE_MONO="no"],
    [HAVE_MONO="$(which mono) ($(mono --version | head -n 1 | cut -f5 -d' '))"])
AM_CONDITIONAL([HAVE_MONO], [test ! "x$HAVE_MONO" = "xno"])

# check for naturaldocs
AS_IF([test -z "$(which naturaldocs)" ],
    [HAVE_NATURALDOCS="no"],
    [HAVE_NATURALDOCS="$(which naturaldocs) ($(naturaldocs --version))"])
AM_CONDITIONAL([HAVE_NATURALDOCS], [test ! "x$HAVE_NATURALDOCS" = "xno"])

# check for AWS
AS_IF([test -z "$(which aws)" ],
    [HAVE_AWSCLI="no"],
    [HAVE_AWSCLI="$(which aws) ($(aws --version | cut -f1 -d' ' | cut -f2 -d/))"])
AM_CONDITIONAL([HAVE_AWSCLI], [test ! "x$HAVE_AWSCLI" = "xno"])

AS_IF([test -z "$(which gcloud)" ],
    [HAVE_GCPCLI="no"],
    [HAVE_GCPCLI="$(which gcloud) ($(gcloud --version | head -n 1 | cut -f4 -d' '))"])
AM_CONDITIONAL([HAVE_GCPCLI], [test ! "x$HAVE_GCPCLI" = "xno"])

AS_IF([test -z "$(which az)" ],
    [HAVE_AZCLI="no"],
    [HAVE_AZCLI="$(which az) ($(echo $(az --version 2>/dev/null | head -n 1) | cut -f2 -d' '))"])
AM_CONDITIONAL([HAVE_AZCLI], [test ! "x$HAVE_AZCLI" = "xno"])

AS_IF([test -z "$(which git)" ],
    [HAVE_GIT="no"],
    [HAVE_GIT="$(which git) ($(git --version | cut -f3 -d' '))"])
AM_CONDITIONAL([HAVE_GIT], [test ! "x$HAVE_GIT" = "xno"])
BRANCHNAME=$(git rev-parse --abbrev-ref HEAD)
AM_CONDITIONAL([IS_MASTER],[test "x$BRANCHNAME" = "xmaster"])
AM_CONDITIONAL([IS_DEVELOP],[test "x$BRANCHNAME" = "xdevelop"])
AS_CASE([$BRANCHNAME],
    [master], [SUFFIX=""],
    [develop*], [SUFFIX="-dev"],
    [*], [SUFFIX="no"])
AM_CONDITIONAL([SUFFIX],[test ! "x$SUFFIX" = "xno"] )

###############################################################################
# The End
###############################################################################

VERSION_NAME=$(build-aux/version.sh --name)
PREFIX=/usr/local/opt/gridlabd
DSTDIR=$PREFIX/$VERSION_NAME
BINDIR=$DSTDIR/bin
LIBDIR=$DSTDIR/lib/gridlabd
ETCDIR=$DSTDIR/share/gridlabd
DOCDIR=$DSTDIR/share/doc/gridlabd

AC_MSG_RESULT([
-----------------------------------------------------------------------

  HiPAS GridLAB-D Automatic configuration:

    Package: .................... $PACKAGE
    Version: .................... $VERSION
    Branch: ..................... $PACKAGE_ORIGIN
    Install prefix: ............. ${DSTDIR/^$PWD/.}

  Dependencies:

    ncurses: .................... $HAVE_CURSES
    armadillo: .................. $HAVE_ARMA
    python: ..................... $HAVE_PYTHON
    mysql-connector-c: .......... $HAVE_MYSQL
    Doxygen: .................... $HAVE_DOXYGEN
    Mono: ....................... $HAVE_MONO
    Natural docs: ............... $HAVE_NATURALDOCS

  Atomic Builtins:

    __sync_bool_compare_and_swap: $HAVE_SYNC_BOOL_COMPARE_AND_SWAP
    __sync_add_and_fetch: ....... $HAVE_SYNC_ADD_AND_FETCH

  Build flags:

    CXXFLAGS: ................... ${CXXFLAGS:-(none)}
    LDFLAGS: .................... ${LDFLAGS:-(none)}

  Installation paths:

    Scripts: .................... ${BINDIR/^$PWD/.}
    Libraries: .................. ${LIBDIR/^$PWD/.}
    Data: ....................... ${ETCDIR/^#PWD/.}
    Documentation: .............. ${DOCDIR/^$PWD/.}

  Building:

    type 'make' to compile $PACKAGE.
    type 'make install' to install $PACKAGE in ${DSTDIR/^$PWD/.}.
    type 'make system' to install $PACKAGE as set system version

  Cloud support:

    AWS CLI: .................... $HAVE_AWSCLI
    GCP CLI: .................... $HAVE_GCPCLI
    Azure CLI: .................. $HAVE_AZCLI

  Some useful commands:

    To add $PACKAGE to your path:

        export PATH=$DSTDIR/bin:\$PATH

    To check that your running the current version of $PACKAGE:

        $PACKAGE --version

    To set this $PACKAGE version $VERSION as the system version:

        $DSTDIR/bin/$PACKAGE version set

-----------------------------------------------------------------------
])

CXXFLAGS="$(python3-config --cflags) $CXXFLAGS -D PYTHON_EXEC='\"$PYTHON_EXEC\"'"
LDFLAGS="$(python3-config --ldflags) $PYTHON_LIB $LDFLAGS "

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([gridlabd.spec])
AC_CONFIG_FILES([gldcore/gridlabd], [chmod +x gldcore/gridlabd])
AC_OUTPUT
