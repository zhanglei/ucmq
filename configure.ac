#           -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([ucmq], [2.0.1], [yuancy@ucweb.local])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([source/main.c])
AC_CONFIG_HEADER([source/config.h])

default_find_path=$HOME/local

AC_ARG_WITH([target], [AS_HELP_STRING([--with-target=TARGET],[build with server/client/all])],[],[withval="all"])
case $withval in
    "client"*)
        AM_CONDITIONAL([SERVER], [false])
        AM_CONDITIONAL([CLIENT], [true])
    ;;
    "server"*)
        AM_CONDITIONAL([SERVER], [true])
        AM_CONDITIONAL([CLIENT], [false])
        server="yes"
    ;;
    "all"*)
        AM_CONDITIONAL([SERVER], [true])
        AM_CONDITIONAL([CLIENT], [true])
        server="yes"
    ;;
    *)
        AC_MSG_ERROR([unknown target, must be server/client/all]) 
    ;;
esac

if test -n "$server"; then
    AC_ARG_WITH([libevent], [AS_HELP_STRING([--with-libevent=DIR],[Where to find libevent])],, [withval=$default_find_path])
    if test "$withval" != "no" ; then
        AC_MSG_CHECKING([for libevent installation]) 
        for dir in "." $withval/include ; do 
            if test -f "$dir/event.h" ; then 
                CFLAGS="$CFLAGS -I$dir" 
                ev_include="ok"
            fi
        done
        for dir in "." $withval/lib ; do 
            if test -f "$dir/libevent_core.a" ; then 
                EVENT_LIBS="$dir/libevent.a"
                ev_core="ok"
            fi
            if test -f "$dir/libevent_extra.a" ; then 
                EVENT_LIBS="$EVENT_LIBS $dir/libevent_extra.a"
                ev_extra="ok"
            fi
        done
        if test "$ev_include" == "ok" -a "$ev_core" == "ok"  -a "$ev_extra" == "ok" ; then 
            AC_MSG_RESULT([yes]) 
        else 
            AC_MSG_RESULT([no]) 
            AC_MSG_ERROR([libevent not found]) 
        fi 
    fi
    AC_SUBST(EVENT_LIBS)
fi

# if test "$LIBEVENT_EXIST" == "yes" ; then 
#     AC_CHECK_HEADER([event.h], , AC_MSG_ERROR([event.h not found])) 
#     AC_CHECK_HEADER([evhttp.h], , AC_MSG_ERROR([evhttp.h not found])) 
#     AC_CHECK_LIB(event_core, event_init, LIBEVENT_LIBS="-levent_core", AC_MSG_ERROR(libevent not found)) 
#     AC_CHECK_LIB(event_extra, evhttp_new, LIBEVENT_LIBS="$LIBEVENT_LIBS -levent_extra", AC_MSG_ERROR(libevent not found)) 
# fi

# AC_SUBST(LIBEVENT_LIBS)
AX_CHECK_COMPILER_FLAGS(["-pipe"], [CFLAGS="$CFLAGS -pipe"]) 
AC_ARG_WITH(gcc-arch, [AS_HELP_STRING([--with-gcc-arch=<arch>], [use architecture <arch> for gcc -march/-mtune, instead of guessing])], gcc_arch_list=$withval, gcc_arch_list=sys)
if test "$gcc_arch_list" = "sys" ; then
    AC_CANONICAL_HOST
    case $host_cpu in
        x86_64*) gcc_arch_list="native corei7 core2 nocona pentium4 pentiumpro" ;;
        *) gcc_arch_list="native prescott pentium4 pentium-m pentium3 pentium2 pentiumpro" ;;
    esac
fi
for arch in $gcc_arch_list; do
    AX_CHECK_COMPILER_FLAGS(["-march=$arch"], [CFLAGS="$CFLAGS -march=$arch"; break]) 
done

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug], [compile for debugging])], [CFLAGS="$CFLAGS -g -O0"], [CFLAGS="$CFLAGS -O2 -minline-all-stringops -fomit-frame-pointer"])

# store current user givern compiler flags to avoid default setup via AC_PROG_CXX
OLD_CXXFLAGS=$CXXFLAGS
OLD_CFLAGS=$CFLAGS

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# reset compiler flagsto initial flags
CXXFLAGS=$OLD_CXXFLAGS
CFLAGS=$OLD_CFLAGS

MIN_VERSION=4.1.2
AC_MSG_CHECKING([$CC version])
CC_VER=`$CC -dumpversion`
is_ok=`echo $MIN_VERSION.$CC_VER | awk 'BEGIN{FS="."}{print(($4>$1)||(($4==$1)&&(($5>$2)||(($5==$2)&&($6>=$3))))) ? "OK" : "FAILED";}'`
if test "$is_ok" = "OK"; then
	AC_MSG_RESULT([$CC_VER]) 
else
	AC_MSG_RESULT([$CC_VER]) 
	AC_MSG_ERROR([ !!! we need $CC version up to $MIN_VERSION !!! ]) 
fi

AC_MSG_CHECKING([$CXX version])
CXX_VER=`$CXX -dumpversion`
is_ok=`echo $MIN_VERSION.$CC_VER | awk 'BEGIN{FS="."}{print(($4>$1)||(($4==$1)&&(($5>$2)||(($5==$2)&&($6>=$3))))) ? "OK" : "FAILED";}'`
if test "$is_ok" = "OK"; then
	AC_MSG_RESULT([$CXX_VER]) 
else
	AC_MSG_RESULT([$CXX_VER]) 
	AC_MSG_ERROR([ !!! we need $CXX version up to $MIN_VERSION !!! ]) 
fi

AC_ARG_ENABLE(gcov, [AC_HELP_STRING([--enable-gcov], [compile for gcov])], enable_gcov=$enableval, enable_gcov=no)
if test "$enable_gcov" = "yes"; then
	if test "$enable_debug" = "yes"; then
		CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	else
		AC_MSG_ERROR([ !!! debug must be enabled when try to enable gconv !!! ]) 
	fi
fi

gcc_arg_list="-pipe -Wall -Wparentheses -Winline -Wuninitialized -Wunused -Wcomment -Wformat -Wimplicit -Wsequence-point -Wfloat-equal -Wshadow -fstack-protector-all "
for arg in $gcc_arg_list; do
    AX_CHECK_COMPILER_FLAGS(["$arg"], [CFLAGS="$CFLAGS $arg"]) 
done

CFLAGS="$CFLAGS -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"

CXXFLAGS="$CFLAGS -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS "
CFLAGS="$CFLAGS --std=gnu99"

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lbz2':
AC_CHECK_LIB([bz2], [main])
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lnsl':
AC_CHECK_LIB([nsl], [main])
# FIXME: Replace `main' with a function in `-lresolv':
AC_CHECK_LIB([resolv], [main])
# FIXME: Replace `main' with a function in `-lrt':
AC_CHECK_LIB([rt], [main])
# FIXME: Replace `main' with a function in `-lz':
AC_CHECK_LIB([z], [main])
# # FIXME: Replace `main' with a function in `-lstdc++':
# AC_CHECK_LIB([stdc++], [main])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_WAIT3
AC_CHECK_FUNCS([clock_gettime gethostbyname gettimeofday inet_ntoa localtime_r memset mkdir socket strcasecmp strchr strcspn strerror strncasecmp strstr])

AC_CONFIG_FILES([Makefile
                 client/Makefile
                 client/c/Makefile
                 source/Makefile])
AC_OUTPUT
