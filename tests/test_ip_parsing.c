/*
 * Unit tests for IP address parsing functions
 */

#include "test_harness.h"
#include "../tunstdio_lib.h"
#include <arpa/inet.h>

/*
 * Test: parse_ip_prefix with valid IP only (no prefix)
 */
static int test_parse_ip_no_prefix(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_STR_EQ("192.168.1.1", ip_out, "IP should be extracted");
    TEST_ASSERT_EQ(-1, prefix, "Prefix should be -1 (not specified)");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with valid IP and prefix
 */
static int test_parse_ip_with_prefix(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("10.0.0.1/24", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_STR_EQ("10.0.0.1", ip_out, "IP should be extracted");
    TEST_ASSERT_EQ(24, prefix, "Prefix should be 24");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with /0 prefix
 */
static int test_parse_ip_prefix_zero(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("0.0.0.0/0", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_STR_EQ("0.0.0.0", ip_out, "IP should be 0.0.0.0");
    TEST_ASSERT_EQ(0, prefix, "Prefix should be 0");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with /32 prefix
 */
static int test_parse_ip_prefix_32(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("255.255.255.255/32", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_STR_EQ("255.255.255.255", ip_out, "IP should be extracted");
    TEST_ASSERT_EQ(32, prefix, "Prefix should be 32");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with invalid prefix (>32)
 */
static int test_parse_ip_prefix_too_large(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1/33", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for prefix > 32");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with negative prefix
 */
static int test_parse_ip_prefix_negative(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1/-1", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for negative prefix");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with non-numeric prefix
 */
static int test_parse_ip_prefix_non_numeric(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1/abc", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for non-numeric prefix");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with empty prefix
 */
static int test_parse_ip_prefix_empty(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1/", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for empty prefix");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with IP too long
 */
static int test_parse_ip_too_long(void) {
    char ip_out[8]; /* Too small for a full IP */
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1/24", ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error when buffer too small");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with NULL input
 */
static int test_parse_ip_null_input(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    int ret = parse_ip_prefix(NULL, ip_out, sizeof(ip_out), &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for NULL input");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with NULL output buffer
 */
static int test_parse_ip_null_output(void) {
    int prefix;
    
    int ret = parse_ip_prefix("192.168.1.1", NULL, 16, &prefix);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for NULL output");
    
    return 0;
}

/*
 * Test: parse_ip_prefix with NULL prefix pointer
 */
static int test_parse_ip_null_prefix(void) {
    char ip_out[INET_ADDRSTRLEN];
    
    int ret = parse_ip_prefix("192.168.1.1", ip_out, sizeof(ip_out), NULL);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error for NULL prefix pointer");
    
    return 0;
}

/*
 * Test: validate_ip_address with valid IPs
 */
static int test_validate_ip_valid(void) {
    TEST_ASSERT_EQ(1, validate_ip_address("0.0.0.0"), "0.0.0.0 should be valid");
    TEST_ASSERT_EQ(1, validate_ip_address("255.255.255.255"), "255.255.255.255 should be valid");
    TEST_ASSERT_EQ(1, validate_ip_address("192.168.1.1"), "192.168.1.1 should be valid");
    TEST_ASSERT_EQ(1, validate_ip_address("10.0.0.1"), "10.0.0.1 should be valid");
    TEST_ASSERT_EQ(1, validate_ip_address("127.0.0.1"), "127.0.0.1 should be valid");
    TEST_ASSERT_EQ(1, validate_ip_address("1.2.3.4"), "1.2.3.4 should be valid");
    
    return 0;
}

/*
 * Test: validate_ip_address with invalid IPs
 */
static int test_validate_ip_invalid(void) {
    TEST_ASSERT_EQ(0, validate_ip_address("256.0.0.0"), "256.0.0.0 should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address("999.999.999.999"), "999.999.999.999 should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address("not.an.ip.address"), "not.an.ip should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address("1.2.3"), "1.2.3 should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address("1.2.3.4.5"), "1.2.3.4.5 should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address(""), "empty string should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address("abc"), "abc should be invalid");
    TEST_ASSERT_EQ(0, validate_ip_address("192.168.1.1/24"), "CIDR notation should be invalid");
    
    return 0;
}

/*
 * Test: prefix_to_netmask with common values
 */
static int test_prefix_to_netmask_common(void) {
    /* /0 = 0.0.0.0 */
    TEST_ASSERT_EQ(htonl(0x00000000), prefix_to_netmask(0), "/0 -> 0.0.0.0");
    
    /* /8 = 255.0.0.0 */
    TEST_ASSERT_EQ(htonl(0xFF000000), prefix_to_netmask(8), "/8 -> 255.0.0.0");
    
    /* /16 = 255.255.0.0 */
    TEST_ASSERT_EQ(htonl(0xFFFF0000), prefix_to_netmask(16), "/16 -> 255.255.0.0");
    
    /* /24 = 255.255.255.0 */
    TEST_ASSERT_EQ(htonl(0xFFFFFF00), prefix_to_netmask(24), "/24 -> 255.255.255.0");
    
    /* /32 = 255.255.255.255 */
    TEST_ASSERT_EQ(htonl(0xFFFFFFFF), prefix_to_netmask(32), "/32 -> 255.255.255.255");
    
    return 0;
}

/*
 * Test: prefix_to_netmask with less common values
 */
static int test_prefix_to_netmask_other(void) {
    /* /1 = 128.0.0.0 */
    TEST_ASSERT_EQ(htonl(0x80000000), prefix_to_netmask(1), "/1 -> 128.0.0.0");
    
    /* /25 = 255.255.255.128 */
    TEST_ASSERT_EQ(htonl(0xFFFFFF80), prefix_to_netmask(25), "/25 -> 255.255.255.128");
    
    /* /30 = 255.255.255.252 (point-to-point) */
    TEST_ASSERT_EQ(htonl(0xFFFFFFFC), prefix_to_netmask(30), "/30 -> 255.255.255.252");
    
    /* /31 = 255.255.255.254 */
    TEST_ASSERT_EQ(htonl(0xFFFFFFFE), prefix_to_netmask(31), "/31 -> 255.255.255.254");
    
    return 0;
}

/*
 * Test: prefix_to_netmask edge cases
 */
static int test_prefix_to_netmask_edge(void) {
    /* Negative should be treated as 0 */
    TEST_ASSERT_EQ(htonl(0x00000000), prefix_to_netmask(-1), "negative -> 0.0.0.0");
    
    /* > 32 should be treated as 32 */
    TEST_ASSERT_EQ(htonl(0xFFFFFFFF), prefix_to_netmask(33), ">32 -> 255.255.255.255");
    TEST_ASSERT_EQ(htonl(0xFFFFFFFF), prefix_to_netmask(100), "100 -> 255.255.255.255");
    
    return 0;
}

/*
 * Test: Full parsing workflow - parse then validate
 */
static int test_parse_then_validate_workflow(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    /* Valid input */
    int ret = parse_ip_prefix("192.168.1.100/24", ip_out, sizeof(ip_out), &prefix);
    TEST_ASSERT_EQ(0, ret, "Parsing should succeed");
    TEST_ASSERT_EQ(1, validate_ip_address(ip_out), "Parsed IP should be valid");
    TEST_ASSERT_EQ(24, prefix, "Prefix should be 24");
    
    return 0;
}

/*
 * Test: Various valid IP formats
 */
static int test_various_valid_formats(void) {
    char ip_out[INET_ADDRSTRLEN];
    int prefix;
    
    /* Class A private */
    TEST_ASSERT_EQ(0, parse_ip_prefix("10.0.0.1/8", ip_out, sizeof(ip_out), &prefix), "Class A");
    TEST_ASSERT_STR_EQ("10.0.0.1", ip_out, "Class A IP");
    TEST_ASSERT_EQ(8, prefix, "Class A prefix");
    
    /* Class B private */
    TEST_ASSERT_EQ(0, parse_ip_prefix("172.16.0.1/12", ip_out, sizeof(ip_out), &prefix), "Class B");
    TEST_ASSERT_STR_EQ("172.16.0.1", ip_out, "Class B IP");
    TEST_ASSERT_EQ(12, prefix, "Class B prefix");
    
    /* Class C private */
    TEST_ASSERT_EQ(0, parse_ip_prefix("192.168.0.1/24", ip_out, sizeof(ip_out), &prefix), "Class C");
    TEST_ASSERT_STR_EQ("192.168.0.1", ip_out, "Class C IP");
    TEST_ASSERT_EQ(24, prefix, "Class C prefix");
    
    /* Loopback */
    TEST_ASSERT_EQ(0, parse_ip_prefix("127.0.0.1", ip_out, sizeof(ip_out), &prefix), "Loopback");
    TEST_ASSERT_STR_EQ("127.0.0.1", ip_out, "Loopback IP");
    TEST_ASSERT_EQ(-1, prefix, "Loopback no prefix");
    
    return 0;
}

/*
 * Main test runner
 */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    TEST_SUITE_BEGIN("IP Parsing Unit Tests");
    
    printf("\n[parse_ip_prefix - valid cases]\n");
    RUN_TEST(test_parse_ip_no_prefix);
    RUN_TEST(test_parse_ip_with_prefix);
    RUN_TEST(test_parse_ip_prefix_zero);
    RUN_TEST(test_parse_ip_prefix_32);
    
    printf("\n[parse_ip_prefix - invalid cases]\n");
    RUN_TEST(test_parse_ip_prefix_too_large);
    RUN_TEST(test_parse_ip_prefix_negative);
    RUN_TEST(test_parse_ip_prefix_non_numeric);
    RUN_TEST(test_parse_ip_prefix_empty);
    RUN_TEST(test_parse_ip_too_long);
    
    printf("\n[parse_ip_prefix - NULL handling]\n");
    RUN_TEST(test_parse_ip_null_input);
    RUN_TEST(test_parse_ip_null_output);
    RUN_TEST(test_parse_ip_null_prefix);
    
    printf("\n[validate_ip_address]\n");
    RUN_TEST(test_validate_ip_valid);
    RUN_TEST(test_validate_ip_invalid);
    
    printf("\n[prefix_to_netmask]\n");
    RUN_TEST(test_prefix_to_netmask_common);
    RUN_TEST(test_prefix_to_netmask_other);
    RUN_TEST(test_prefix_to_netmask_edge);
    
    printf("\n[Workflow tests]\n");
    RUN_TEST(test_parse_then_validate_workflow);
    RUN_TEST(test_various_valid_formats);
    
    TEST_SUITE_END();
}
