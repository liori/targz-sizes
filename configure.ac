dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(targz_sizes, 1.0)
AC_CONFIG_MACRO_DIR([m4])

dnl Autoconf this.
CFLAGS="$CFLAGS -std=gnu99"

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CC
AX_CHECK_ZLIB

AX_VALGRIND_CHECK

AM_EXTRA_RECURSIVE_TARGETS( \
	check-valgrind \
	check-valgrind-helgrind \
	check-valgrind-drd \
	check-valgrind-sgcheck \
	check-valgrind-memcheck)

AC_CONFIG_FILES( \
	Makefile \
	src/Makefile \
	test/Makefile )
AC_OUTPUT

