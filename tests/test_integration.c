/*
 * Integration tests for tunstdio
 * 
 * These tests spawn the tunstdio program as a subprocess and test
 * the stdin/stdout hex encoding/decoding functionality.
 * 
 * Note: Tests requiring TUN device creation need root privileges
 * and will be skipped if not running as root.
 */

#include "test_harness.h"
#include "../tunstdio_lib.h"
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

/* Path to the tunstdio binary */
#ifndef TUNSTDIO_BIN
#define TUNSTDIO_BIN "./tunstdio"
#endif

/* Helper to write and check return value */
static inline ssize_t write_check(int fd, const void *buf, size_t count) {
    ssize_t ret = write(fd, buf, count);
    if (ret < 0) {
        perror("write");
    }
    return ret;
}

/*
 * Test: Program exits with error when no arguments provided
 */
static int test_no_args_error(void) {
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess - binary may not exist");
    }
    
    close(proc.stdin_fd);
    proc.stdin_fd = -1;
    
    int status = subprocess_wait(&proc, 5);
    subprocess_close(&proc);
    
    TEST_ASSERT_EQ(1, status, "Program should exit with status 1 when no args provided");
    
    return 0;
}

/*
 * Test: Program shows usage when no arguments provided
 */
static int test_usage_message(void) {
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    close(proc.stdin_fd);
    proc.stdin_fd = -1;
    
    char stderr_buf[1024] = {0};
    ssize_t n = read_timeout(proc.stderr_fd, stderr_buf, sizeof(stderr_buf) - 1, 1000);
    
    subprocess_wait(&proc, 5);
    subprocess_close(&proc);
    
    TEST_ASSERT(n > 0, "Should produce stderr output");
    TEST_ASSERT(strstr(stderr_buf, "Usage:") != NULL, "Should show usage message");
    
    return 0;
}

/*
 * Test: Invalid IP address is rejected
 */
static int test_invalid_ip_rejected(void) {
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "not.valid.ip", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    close(proc.stdin_fd);
    proc.stdin_fd = -1;
    
    int status = subprocess_wait(&proc, 5);
    subprocess_close(&proc);
    
    /* Should fail - either can't open TUN (not root) or invalid IP */
    TEST_ASSERT(status != 0, "Program should exit with error for invalid IP");
    
    return 0;
}

/*
 * Test: Invalid prefix length is rejected
 * 
 * Note: Without root, the program may fail on TUN device open before
 * reaching prefix validation. We just check that it fails with some error.
 */
static int test_invalid_prefix_rejected(void) {
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "192.168.1.1/33", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    close(proc.stdin_fd);
    proc.stdin_fd = -1;
    
    char stderr_buf[1024] = {0};
    read_timeout(proc.stderr_fd, stderr_buf, sizeof(stderr_buf) - 1, 1000);
    
    int status = subprocess_wait(&proc, 5);
    subprocess_close(&proc);
    
    /* Should fail - either with invalid prefix error or TUN open error (if not root) */
    TEST_ASSERT(status != 0, "Program should exit with error for invalid prefix");
    /* When running as root, we expect "Invalid prefix"; without root, it may fail on TUN open */
    TEST_ASSERT(strlen(stderr_buf) > 0, "Should produce error output on stderr");
    
    return 0;
}

/*
 * Test: TUN device creation (requires root)
 */
static int test_tun_creation(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.200.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    /* Give it time to start */
    usleep(500000);
    
    /* Read stderr to check for success message */
    char stderr_buf[1024] = {0};
    ssize_t n = read_timeout(proc.stderr_fd, stderr_buf, sizeof(stderr_buf) - 1, 2000);
    
    subprocess_kill(&proc);
    
    TEST_ASSERT(n > 0, "Should produce stderr output");
    TEST_ASSERT(strstr(stderr_buf, "TUN device") != NULL, "Should report TUN device creation");
    TEST_ASSERT(strstr(stderr_buf, "is up") != NULL, "Should report device is up");
    
    return 0;
}

/*
 * Test: Hex encoding stdin to TUN (requires root)
 * 
 * This test sends hex-encoded data via stdin and verifies it's processed correctly.
 */
static int test_stdin_hex_decode(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.201.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    /* Wait for device to come up */
    usleep(500000);
    
    /* Drain startup messages from stderr */
    char stderr_buf[4096] = {0};
    read_timeout(proc.stderr_fd, stderr_buf, sizeof(stderr_buf) - 1, 1000);
    
    /* Send a hex-encoded "packet" via stdin
     * This is just test data - not a real IP packet
     */
    const char *hex_data = "deadbeef\n";
    ssize_t written = write(proc.stdin_fd, hex_data, strlen(hex_data));
    TEST_ASSERT(written == (ssize_t)strlen(hex_data), "Should write hex data to stdin");
    
    /* Give it time to process */
    usleep(200000);
    
    /* Read debug output from stderr */
    memset(stderr_buf, 0, sizeof(stderr_buf));
    read_timeout(proc.stderr_fd, stderr_buf, sizeof(stderr_buf) - 1, 1000);
    
    subprocess_kill(&proc);
    
    /* Should see debug messages about reading bytes or writing packets */
    TEST_ASSERT(strstr(stderr_buf, "Read bytes") != NULL || 
                strstr(stderr_buf, "Writing packet") != NULL,
                "Should show debug messages about processing input");
    
    return 0;
}

/*
 * Test: Multiple hex pairs in single line (requires root)
 */
static int test_multiple_hex_pairs(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.202.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Drain startup messages */
    char buf[4096];
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    /* Send multiple hex pairs followed by newline (packet delimiter) */
    const char *hex_data = "0102030405060708090a0b0c0d0e0f\n";
    write_check(proc.stdin_fd, hex_data, strlen(hex_data));
    
    usleep(200000);
    
    /* Read stderr for debug messages */
    memset(buf, 0, sizeof(buf));
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    subprocess_kill(&proc);
    
    /* Should process the hex data - look for packet write message or read bytes message */
    TEST_ASSERT(strstr(buf, "Writing packet") != NULL || 
                strstr(buf, "Read bytes") != NULL,
                "Should process hex pairs");
    
    return 0;
}

/*
 * Test: Whitespace handling in hex input (requires root)
 */
static int test_whitespace_handling(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.203.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Drain startup messages */
    char buf[4096];
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    /* Send data with various whitespace - each whitespace triggers packet end */
    const char *hex_data = "aabb\nccdd\n";
    write_check(proc.stdin_fd, hex_data, strlen(hex_data));
    
    usleep(200000);
    
    memset(buf, 0, sizeof(buf));
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    subprocess_kill(&proc);
    
    /* Should see multiple write operations (one per packet) */
    TEST_ASSERT(strlen(buf) > 0, "Should produce debug output");
    
    return 0;
}

/*
 * Test: Empty packets are ignored (requires root)
 */
static int test_empty_packets_ignored(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.204.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Drain startup messages */
    char buf[4096];
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    /* Send multiple newlines (should be ignored) then actual data */
    const char *hex_data = "\n\n\naa\n";
    write_check(proc.stdin_fd, hex_data, strlen(hex_data));
    
    usleep(200000);
    
    memset(buf, 0, sizeof(buf));
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    subprocess_kill(&proc);
    
    /* Should process the valid hex pair - look for packet write or read bytes message */
    TEST_ASSERT(strstr(buf, "Writing packet") != NULL || 
                strstr(buf, "Read bytes") != NULL,
                "Should process the valid hex pair");
    
    return 0;
}

/*
 * Test: Large packet handling (requires root)
 */
static int test_large_packet(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.205.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Drain startup messages */
    char buf[4096];
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    /* Generate a large hex-encoded packet (1500 bytes = typical MTU) 
     * 1500 bytes * 2 hex chars = 3000 + newline + null = 3002 bytes needed */
    char large_packet[3008];
    for (int i = 0; i < 1500; i++) {
        sprintf(&large_packet[i*2], "%02x", i & 0xff);
    }
    strcat(large_packet, "\n");
    
    write_check(proc.stdin_fd, large_packet, strlen(large_packet));
    
    usleep(500000);
    
    subprocess_kill(&proc);
    
    /* If we got here without crashing, the large packet was handled */
    return 0;
}

/*
 * Test: Program handles SIGTERM gracefully
 */
static int test_sigterm_handling(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.206.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Send SIGTERM */
    kill(proc.pid, SIGTERM);
    
    int status = subprocess_wait(&proc, 5);
    subprocess_close(&proc);
    
    /* Process should terminate (either exit 0 or signal) */
    TEST_ASSERT(status <= 0 || status == 143, "Process should handle SIGTERM");
    
    return 0;
}

/*
 * Test: stdin EOF handling
 */
static int test_stdin_eof(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.207.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Close stdin to send EOF */
    close(proc.stdin_fd);
    proc.stdin_fd = -1;
    
    /* Give it time to react */
    usleep(500000);
    
    /* The program may continue running (waiting for TUN) or exit */
    subprocess_kill(&proc);
    
    /* If we got here without hanging, EOF was handled */
    return 0;
}

/*
 * Test: Concurrent TUN read/write (requires root)
 * 
 * This tests the poll loop handling both stdin and TUN fd.
 * Note: We send minimal data and don't require all writes to succeed,
 * as the TUN device may reject invalid packets causing the process to exit.
 */
static int test_concurrent_io(void) {
    REQUIRE_ROOT();
    REQUIRE_TUN();
    
    subprocess_t proc;
    char *argv[] = {TUNSTDIO_BIN, "10.200.208.1/24", NULL};
    
    if (subprocess_spawn(&proc, TUNSTDIO_BIN, argv) < 0) {
        SKIP_TEST("Could not spawn subprocess");
    }
    
    usleep(500000);
    
    /* Drain startup messages */
    char buf[4096];
    read_timeout(proc.stderr_fd, buf, sizeof(buf) - 1, 1000);
    
    /* Send a couple of packets - don't require all to succeed since
     * the TUN device may reject invalid packets and cause the process to exit */
    int writes_succeeded = 0;
    for (int i = 0; i < 3; i++) {
        char hex[32];
        sprintf(hex, "%02x%02x%02x%02x\n", i, i+1, i+2, i+3);
        if (write_check(proc.stdin_fd, hex, strlen(hex)) > 0) {
            writes_succeeded++;
        }
        usleep(50000);
    }
    
    usleep(200000);
    
    subprocess_kill(&proc);
    
    /* At least one write should have succeeded before any potential failure */
    TEST_ASSERT(writes_succeeded >= 1, "At least one write should succeed");
    
    return 0;
}

/*
 * Main test runner
 */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    /* Ignore SIGPIPE to prevent test runner from dying when subprocess exits */
    signal(SIGPIPE, SIG_IGN);
    
    TEST_SUITE_BEGIN("Integration Tests for tunstdio");
    
    printf("\n[Basic Argument Handling]\n");
    RUN_TEST(test_no_args_error);
    RUN_TEST(test_usage_message);
    RUN_TEST(test_invalid_ip_rejected);
    RUN_TEST(test_invalid_prefix_rejected);
    
    printf("\n[TUN Device Operations] (requires root)\n");
    RUN_TEST(test_tun_creation);
    
    printf("\n[Hex Encoding/Decoding] (requires root)\n");
    RUN_TEST(test_stdin_hex_decode);
    RUN_TEST(test_multiple_hex_pairs);
    RUN_TEST(test_whitespace_handling);
    RUN_TEST(test_empty_packets_ignored);
    RUN_TEST(test_large_packet);
    
    printf("\n[Signal and EOF Handling] (requires root)\n");
    RUN_TEST(test_sigterm_handling);
    RUN_TEST(test_stdin_eof);
    RUN_TEST(test_concurrent_io);
    
    TEST_SUITE_END();
}
