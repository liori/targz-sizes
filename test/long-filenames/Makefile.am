@VALGRIND_CHECK_RULES@

TEST_EXTENSIONS = .sh

SH_LOG_COMPILE=${top_srcdir}/sh-log-compile.sh

VALGRIND_memcheck_FLAGS = --leak-check=full --show-reachable=no --track-origins=yes

check_SCRIPTS = \
	basic.sh \
	one-full-block.sh \
	two-blocks.sh \
	two-full-blocks.sh \
	three-blocks.sh

TESTS = $(check_SCRIPTS)

