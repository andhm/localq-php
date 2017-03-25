dnl $Id$
dnl config.m4 for extension localq

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(localq, for localq support,
dnl Make sure that the comment is aligned:
[  --with-localq             Include localq support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(localq, whether to enable localq support,
dnl Make sure that the comment is aligned:
[  --enable-localq           Enable localq support])

if test "$PHP_LOCALQ" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-localq -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/localq.h"  # you most likely want to change this
  dnl if test -r $PHP_LOCALQ/$SEARCH_FOR; then # path given as parameter
  dnl   LOCALQ_DIR=$PHP_LOCALQ
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for localq files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       LOCALQ_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$LOCALQ_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the localq distribution])
  dnl fi

  dnl # --with-localq -> add include path
  dnl PHP_ADD_INCLUDE($LOCALQ_DIR/include)

  dnl # --with-localq -> check for lib and symbol presence
  dnl LIBNAME=localq # you may want to change this
  dnl LIBSYMBOL=localq # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LOCALQ_DIR/lib, LOCALQ_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_LOCALQLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong localq lib version or lib not found])
  dnl ],[
  dnl   -L$LOCALQ_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(LOCALQ_SHARED_LIBADD)

  PHP_NEW_EXTENSION(localq, localq.c, $ext_shared)
fi
