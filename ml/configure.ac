dnl Process this file with autoconf to produce a configure script.
AC_INIT([ML], [0.1], [kiraly@disi.unitn.it])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_RANLIB
AC_LANG_C

AC_ARG_WITH(libevent2, [  --with-libevent2=libevent2_dir use libevent2 in dir])
if test "$with_libevent2" == "yes"
then
	AC_MSG_ERROR([missing directory specification after --libevent2])
	exit -1
fi
if test -d "$with_libevent2"
then
	CPPFLAGS="$CPPFLAGS -I$with_libevent2/include"
	LDFLAGS="$LDFLAGS -L$with_libevent2/lib"
fi

AC_CHECK_HEADER(event2/util.h)
if test "$ac_cv_header_libevent_h" == no
then
	AC_MSG_NOTICE([libevent2 seems to be missing, unable to continue])
fi

AC_CHECK_LIB(event, event_init)
if test $ac_cv_lib_event_event_init == no
then
	AC_MSG_NOTICE([libevent2 seems to be missing, unable to continue])
fi

#case $host in
#  *mingw32) 
#		LIBML_A_SYSTEM='inet_pton.$(OBJEXT) inet_ntop.$(OBJEXT)' 
#		AC_MSG_NOTICE([winzoz mode activated]) ;;
#  *)       ;;
#esac
#AC_SUBST([LIBML_A_SYSTEM])

AM_CONDITIONAL([WINDOWS],  [test "$host" = i586-pc-mingw32 -o "$host" = i686-w64-mingw32] )


AC_OUTPUT( [ Makefile \
	tests/Makefile ] )

