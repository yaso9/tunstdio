#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

/* Colors for terminal output */
#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[0;33m"
#define COLOR_RESET   "\033[0m"

/* Test counters - defined in each test file */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_skipped = 0;

/* Current test name for error reporting */
static const char *current_test_name = NULL;

/*
 * Core assertion macros
 */

#define TEST_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        fprintf(stderr, COLOR_RED "  FAIL: %s\n" COLOR_RESET, msg); \
        fprintf(stderr, "        at %s:%d\n", __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_EQ(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        fprintf(stderr, COLOR_RED "  FAIL: %s\n" COLOR_RESET, msg); \
        fprintf(stderr, "        expected: %ld, got: %ld\n", (long)(expected), (long)(actual)); \
        fprintf(stderr, "        at %s:%d\n", __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(expected, actual, msg) do { \
    if (strcmp((expected), (actual)) != 0) { \
        fprintf(stderr, COLOR_RED "  FAIL: %s\n" COLOR_RESET, msg); \
        fprintf(stderr, "        expected: \"%s\", got: \"%s\"\n", (expected), (actual)); \
        fprintf(stderr, "        at %s:%d\n", __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_MEM_EQ(expected, actual, len, msg) do { \
    if (memcmp((expected), (actual), (len)) != 0) { \
        fprintf(stderr, COLOR_RED "  FAIL: %s\n" COLOR_RESET, msg); \
        fprintf(stderr, "        memory mismatch at %s:%d\n", __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr, msg) do { \
    if ((ptr) != NULL) { \
        fprintf(stderr, COLOR_RED "  FAIL: %s\n" COLOR_RESET, msg); \
        fprintf(stderr, "        expected NULL, got non-NULL\n"); \
        fprintf(stderr, "        at %s:%d\n", __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr, msg) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, COLOR_RED "  FAIL: %s\n" COLOR_RESET, msg); \
        fprintf(stderr, "        expected non-NULL, got NULL\n"); \
        fprintf(stderr, "        at %s:%d\n", __FILE__, __LINE__); \
        return 1; \
    } \
} while(0)

/*
 * Test running infrastructure
 */

#define RUN_TEST(test_func) do { \
    current_test_name = #test_func; \
    tests_run++; \
    printf("  Running %s... ", #test_func); \
    fflush(stdout); \
    int result = test_func(); \
    if (result == 0) { \
        printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
        tests_passed++; \
    } else if (result == 77) { \
        printf(COLOR_YELLOW "SKIP" COLOR_RESET "\n"); \
        tests_skipped++; \
    } else { \
        tests_failed++; \
    } \
} while(0)

#define SKIP_TEST(reason) do { \
    fprintf(stderr, COLOR_YELLOW "  SKIP: %s" COLOR_RESET "\n", reason); \
    return 77; \
} while(0)

#define TEST_SUITE_BEGIN(name) do { \
    printf("\n" COLOR_YELLOW "=== %s ===" COLOR_RESET "\n", name); \
} while(0)

#define TEST_SUITE_END() do { \
    printf("\n--- Results ---\n"); \
    printf("Total: %d, ", tests_run); \
    printf(COLOR_GREEN "Passed: %d" COLOR_RESET ", ", tests_passed); \
    printf(COLOR_RED "Failed: %d" COLOR_RESET ", ", tests_failed); \
    printf(COLOR_YELLOW "Skipped: %d" COLOR_RESET "\n\n", tests_skipped); \
    return tests_failed > 0 ? 1 : 0; \
} while(0)

/*
 * Utility functions for integration tests
 */

/* Check if running as root */
static inline int is_root(void) {
    return geteuid() == 0;
}

/* Check if TUN device is available */
static inline int tun_available(void) {
    return access("/dev/net/tun", R_OK | W_OK) == 0;
}

/* Skip test if not root */
#define REQUIRE_ROOT() do { \
    if (!is_root()) { \
        SKIP_TEST("Requires root privileges"); \
    } \
} while(0)

/* Skip test if TUN not available */
#define REQUIRE_TUN() do { \
    if (!tun_available()) { \
        SKIP_TEST("TUN device not available (requires /dev/net/tun)"); \
    } \
} while(0)

/*
 * Subprocess helpers for integration testing
 */

typedef struct {
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
} subprocess_t;

/* 
 * Spawn a subprocess with piped stdin/stdout/stderr.
 * Returns 0 on success, -1 on failure.
 */
static inline int subprocess_spawn(subprocess_t *proc, const char *path, char *const argv[]) {
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
    
    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
        perror("pipe");
        return -1;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        close(stdin_pipe[1]);  /* Close write end of stdin pipe */
        close(stdout_pipe[0]); /* Close read end of stdout pipe */
        close(stderr_pipe[0]); /* Close read end of stderr pipe */
        
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        
        execv(path, argv);
        perror("execv");
        _exit(127);
    }
    
    /* Parent process */
    close(stdin_pipe[0]);  /* Close read end of stdin pipe */
    close(stdout_pipe[1]); /* Close write end of stdout pipe */
    close(stderr_pipe[1]); /* Close write end of stderr pipe */
    
    proc->pid = pid;
    proc->stdin_fd = stdin_pipe[1];
    proc->stdout_fd = stdout_pipe[0];
    proc->stderr_fd = stderr_pipe[0];
    
    return 0;
}

/*
 * Wait for subprocess with timeout (in seconds).
 * Returns exit status, or -1 on error/timeout.
 */
static inline int subprocess_wait(subprocess_t *proc, int timeout_sec) {
    int status;
    int elapsed = 0;
    
    while (elapsed < timeout_sec) {
        pid_t result = waitpid(proc->pid, &status, WNOHANG);
        if (result < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (result > 0) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                return -WTERMSIG(status);
            }
            return -1;
        }
        usleep(100000); /* 100ms */
        elapsed++;
    }
    
    /* Timeout - kill the process */
    kill(proc->pid, SIGKILL);
    waitpid(proc->pid, &status, 0);
    return -1;
}

/*
 * Close subprocess file descriptors
 */
static inline void subprocess_close(subprocess_t *proc) {
    if (proc->stdin_fd >= 0) close(proc->stdin_fd);
    if (proc->stdout_fd >= 0) close(proc->stdout_fd);
    if (proc->stderr_fd >= 0) close(proc->stderr_fd);
    proc->stdin_fd = proc->stdout_fd = proc->stderr_fd = -1;
}

/*
 * Kill subprocess
 */
static inline void subprocess_kill(subprocess_t *proc) {
    if (proc->pid > 0) {
        kill(proc->pid, SIGTERM);
        usleep(100000);
        kill(proc->pid, SIGKILL);
        waitpid(proc->pid, NULL, 0);
        proc->pid = -1;
    }
    subprocess_close(proc);
}

/*
 * Read from fd with timeout (in milliseconds).
 * Returns bytes read, 0 on timeout, -1 on error.
 */
static inline ssize_t read_timeout(int fd, void *buf, size_t count, int timeout_ms) {
    fd_set rfds;
    struct timeval tv;
    
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (ret < 0) return -1;
    if (ret == 0) return 0; /* Timeout */
    
    return read(fd, buf, count);
}

/*
 * Hex dump utility for debugging
 */
static inline void hexdump(const char *prefix, const unsigned char *data, size_t len) {
    fprintf(stderr, "%s: ", prefix);
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "%02x ", data[i]);
    }
    fprintf(stderr, "\n");
}

#endif /* TEST_HARNESS_H */
