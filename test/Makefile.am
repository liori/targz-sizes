@VALGRIND_CHECK_RULES@

TEST_EXTENSIONS = .sh

SH_LOG_COMPILE=${top_srcdir}/sh-log-compile.sh

VALGRIND_memcheck_FLAGS = --leak-check=full --show-reachable=no --track-origins=yes

check_SCRIPTS = \
	basic.sh

TESTS = $(check_SCRIPTS)
