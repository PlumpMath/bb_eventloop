AC_INIT([EventLoop], [0.1.0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile])
AM_INIT_AUTOMAKE([subdir-objects foreign])
AC_PROG_CXX
AC_LANG([C++])
AM_PROG_LIBTOOL
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_ARG_ENABLE([examples], AS_HELP_STRING([--disable-examples], [Disable examples]))
AS_IF([test "$enable_examples" != "no"], [examples=true])
AM_CONDITIONAL([EXAMPLES], [test "$examples" = "true"])
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])
AX_BOOST_BASE([1.53], [], [AC_MSG_ERROR(Unable to detect suitable boost library)])
AC_OUTPUT
