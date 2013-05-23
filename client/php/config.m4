dnl $Id$
dnl config.m4 for extension ucmq

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(ucmq, for ucmq support,
dnl Make sure that the comment is aligned:
dnl [  --with-ucmq             Include ucmq support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(ucmq, whether to enable ucmq support,
Make sure that the comment is aligned:
[  --enable-ucmq           Enable ucmq support])

if test "$PHP_UCMQ" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-ucmq -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/ucmq.h"  # you most likely want to change this
  dnl if test -r $PHP_UCMQ/$SEARCH_FOR; then # path given as parameter
  dnl   UCMQ_DIR=$PHP_UCMQ
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for ucmq files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       UCMQ_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$UCMQ_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the ucmq distribution])
  dnl fi

  dnl # --with-ucmq -> add include path
  dnl PHP_ADD_INCLUDE($UCMQ_DIR/include)

  dnl # --with-ucmq -> check for lib and symbol presence
  dnl LIBNAME=ucmq # you may want to change this
  dnl LIBSYMBOL=ucmq # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $UCMQ_DIR/lib, UCMQ_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_UCMQLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong ucmq lib version or lib not found])
  dnl ],[
  dnl   -L$UCMQ_DIR/lib -lm
  dnl ])
  dnl
  PHP_SUBST(UCMQ_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ucmq, ucmq.c, $ext_shared)
fi
