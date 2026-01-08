/*
 * Unit tests for hex encoding/decoding functions
 */

#include "test_harness.h"
#include "../tunstdio_lib.h"

/*
 * Test: is_hex_digit with valid digits
 */
static int test_is_hex_digit_valid(void) {
    /* Test 0-9 */
    for (char c = '0'; c <= '9'; c++) {
        TEST_ASSERT(is_hex_digit(c), "0-9 should be valid hex digits");
    }
    
    /* Test a-f */
    for (char c = 'a'; c <= 'f'; c++) {
        TEST_ASSERT(is_hex_digit(c), "a-f should be valid hex digits");
    }
    
    /* Test A-F */
    for (char c = 'A'; c <= 'F'; c++) {
        TEST_ASSERT(is_hex_digit(c), "A-F should be valid hex digits");
    }
    
    return 0;
}

/*
 * Test: is_hex_digit with invalid characters
 */
static int test_is_hex_digit_invalid(void) {
    TEST_ASSERT(!is_hex_digit('g'), "'g' should not be valid hex");
    TEST_ASSERT(!is_hex_digit('G'), "'G' should not be valid hex");
    TEST_ASSERT(!is_hex_digit('z'), "'z' should not be valid hex");
    TEST_ASSERT(!is_hex_digit(' '), "space should not be valid hex");
    TEST_ASSERT(!is_hex_digit('\n'), "newline should not be valid hex");
    TEST_ASSERT(!is_hex_digit('\0'), "null should not be valid hex");
    TEST_ASSERT(!is_hex_digit('@'), "'@' should not be valid hex");
    TEST_ASSERT(!is_hex_digit('/'), "'/' should not be valid hex");
    TEST_ASSERT(!is_hex_digit(':'), "':' should not be valid hex");
    
    return 0;
}

/*
 * Test: hex_pair_to_byte with valid pairs
 */
static int test_hex_pair_to_byte_valid(void) {
    TEST_ASSERT_EQ(0x00, hex_pair_to_byte("00"), "00 -> 0x00");
    TEST_ASSERT_EQ(0xFF, hex_pair_to_byte("ff"), "ff -> 0xFF");
    TEST_ASSERT_EQ(0xFF, hex_pair_to_byte("FF"), "FF -> 0xFF");
    TEST_ASSERT_EQ(0xAB, hex_pair_to_byte("ab"), "ab -> 0xAB");
    TEST_ASSERT_EQ(0xAB, hex_pair_to_byte("AB"), "AB -> 0xAB");
    TEST_ASSERT_EQ(0xAB, hex_pair_to_byte("Ab"), "Ab -> 0xAB (mixed case)");
    TEST_ASSERT_EQ(0x12, hex_pair_to_byte("12"), "12 -> 0x12");
    TEST_ASSERT_EQ(0xDE, hex_pair_to_byte("de"), "de -> 0xDE");
    TEST_ASSERT_EQ(0xAD, hex_pair_to_byte("ad"), "ad -> 0xAD");
    TEST_ASSERT_EQ(0xBE, hex_pair_to_byte("be"), "be -> 0xBE");
    TEST_ASSERT_EQ(0xEF, hex_pair_to_byte("ef"), "ef -> 0xEF");
    
    return 0;
}

/*
 * Test: hex_pair_to_byte with invalid input
 */
static int test_hex_pair_to_byte_invalid(void) {
    TEST_ASSERT_EQ(-1, hex_pair_to_byte(NULL), "NULL should return -1");
    TEST_ASSERT_EQ(-1, hex_pair_to_byte("gg"), "gg should return -1");
    TEST_ASSERT_EQ(-1, hex_pair_to_byte("g0"), "g0 should return -1");
    TEST_ASSERT_EQ(-1, hex_pair_to_byte("0g"), "0g should return -1");
    TEST_ASSERT_EQ(-1, hex_pair_to_byte("  "), "spaces should return -1");
    
    return 0;
}

/*
 * Test: hex_encode basic functionality
 */
static int test_hex_encode_basic(void) {
    char output[64];
    unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    
    hex_encode(data, 4, output);
    TEST_ASSERT_STR_EQ("deadbeef", output, "Should encode to lowercase hex");
    
    return 0;
}

/*
 * Test: hex_encode single byte
 */
static int test_hex_encode_single(void) {
    char output[4];
    unsigned char data[] = {0x42};
    
    hex_encode(data, 1, output);
    TEST_ASSERT_STR_EQ("42", output, "Single byte encode");
    
    return 0;
}

/*
 * Test: hex_encode all byte values
 */
static int test_hex_encode_all_values(void) {
    char output[4];
    
    for (int i = 0; i <= 255; i++) {
        unsigned char byte = (unsigned char)i;
        char expected[4];
        sprintf(expected, "%02x", i);
        
        hex_encode(&byte, 1, output);
        TEST_ASSERT_STR_EQ(expected, output, "Should encode all byte values");
    }
    
    return 0;
}

/*
 * Test: hex_encode empty input
 */
static int test_hex_encode_empty(void) {
    char output[4] = "xxx";
    unsigned char data[] = {};
    
    hex_encode(data, 0, output);
    TEST_ASSERT_STR_EQ("", output, "Empty input should produce empty output");
    
    return 0;
}

/*
 * Test: hex_decode_streaming basic decoding
 */
static int test_hex_decode_streaming_basic(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "deadbeef\n";
    int ret = hex_decode_streaming(input, strlen(input), 
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(4, pkt_end, "Should decode 4 bytes");
    TEST_ASSERT_EQ(1, packet_complete, "Packet should be complete (newline found)");
    TEST_ASSERT_EQ(0xDE, pkt_buf[0], "First byte should be 0xDE");
    TEST_ASSERT_EQ(0xAD, pkt_buf[1], "Second byte should be 0xAD");
    TEST_ASSERT_EQ(0xBE, pkt_buf[2], "Third byte should be 0xBE");
    TEST_ASSERT_EQ(0xEF, pkt_buf[3], "Fourth byte should be 0xEF");
    
    return 0;
}

/*
 * Test: hex_decode_streaming without delimiter
 */
static int test_hex_decode_streaming_no_delimiter(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "aabbccdd";
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(4, pkt_end, "Should decode 4 bytes");
    TEST_ASSERT_EQ(0, packet_complete, "Packet should not be complete (no delimiter)");
    TEST_ASSERT_EQ(8, consumed, "Should consume all 8 chars");
    
    return 0;
}

/*
 * Test: hex_decode_streaming with incomplete pair
 */
static int test_hex_decode_streaming_incomplete_pair(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    /* Input has an incomplete hex pair at the end */
    const char *input = "aabbc";
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(2, pkt_end, "Should decode 2 complete bytes");
    TEST_ASSERT_EQ(4, consumed, "Should consume 4 chars, leaving 1");
    TEST_ASSERT_EQ(0, packet_complete, "Packet should not be complete");
    
    return 0;
}

/*
 * Test: hex_decode_streaming with leading whitespace
 */
static int test_hex_decode_streaming_leading_whitespace(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "  \t\naabb\n";
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(2, pkt_end, "Should decode 2 bytes");
    TEST_ASSERT_EQ(1, packet_complete, "Packet should be complete");
    
    return 0;
}

/*
 * Test: hex_decode_streaming multiple packets
 */
static int test_hex_decode_streaming_multiple_packets(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "aabb\nccdd\n";
    
    /* First packet */
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(2, pkt_end, "First packet should be 2 bytes");
    TEST_ASSERT_EQ(1, packet_complete, "First packet should be complete");
    TEST_ASSERT_EQ(0xAA, pkt_buf[0], "First byte of first packet");
    TEST_ASSERT_EQ(0xBB, pkt_buf[1], "Second byte of first packet");
    
    /* Continue with remaining input for second packet */
    pkt_end = 0;
    const char *remaining = input + consumed;
    size_t remaining_len = strlen(input) - consumed;
    
    ret = hex_decode_streaming(remaining, remaining_len,
                               pkt_buf, &pkt_end, sizeof(pkt_buf),
                               &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(2, pkt_end, "Second packet should be 2 bytes");
    TEST_ASSERT_EQ(1, packet_complete, "Second packet should be complete");
    TEST_ASSERT_EQ(0xCC, pkt_buf[0], "First byte of second packet");
    TEST_ASSERT_EQ(0xDD, pkt_buf[1], "Second byte of second packet");
    
    return 0;
}

/*
 * Test: hex_decode_streaming with mixed case
 */
static int test_hex_decode_streaming_mixed_case(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "AaBbCcDd\n";
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(4, pkt_end, "Should decode 4 bytes");
    TEST_ASSERT_EQ(0xAA, pkt_buf[0], "Mixed case AA");
    TEST_ASSERT_EQ(0xBB, pkt_buf[1], "Mixed case BB");
    TEST_ASSERT_EQ(0xCC, pkt_buf[2], "Mixed case CC");
    TEST_ASSERT_EQ(0xDD, pkt_buf[3], "Mixed case DD");
    
    return 0;
}

/*
 * Test: hex_decode_streaming buffer overflow protection
 */
static int test_hex_decode_streaming_overflow(void) {
    unsigned char pkt_buf[4]; /* Small buffer */
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    /* Try to decode more than buffer can hold */
    const char *input = "0102030405060708\n";
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(-1, ret, "Should return error on overflow");
    
    return 0;
}

/*
 * Test: hex_decode_streaming empty input
 */
static int test_hex_decode_streaming_empty(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "";
    int ret = hex_decode_streaming(input, 0,
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(0, pkt_end, "Should decode 0 bytes");
    TEST_ASSERT_EQ(0, consumed, "Should consume 0 chars");
    TEST_ASSERT_EQ(0, packet_complete, "Packet should not be complete");
    
    return 0;
}

/*
 * Test: hex_decode_streaming only whitespace
 */
static int test_hex_decode_streaming_only_whitespace(void) {
    unsigned char pkt_buf[256];
    size_t pkt_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    const char *input = "   \n\t\n  ";
    int ret = hex_decode_streaming(input, strlen(input),
                                   pkt_buf, &pkt_end, sizeof(pkt_buf),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Should return success");
    TEST_ASSERT_EQ(0, pkt_end, "Should decode 0 bytes");
    TEST_ASSERT_EQ(0, packet_complete, "No packet (empty packets ignored)");
    
    return 0;
}

/*
 * Test: roundtrip encode then decode
 */
static int test_roundtrip(void) {
    unsigned char original[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char encoded[64];
    unsigned char decoded[16];
    size_t decoded_end = 0;
    size_t consumed = 0;
    int packet_complete = 0;
    
    /* Encode */
    hex_encode(original, 16, encoded);
    strcat(encoded, "\n"); /* Add delimiter for decoder */
    
    /* Decode */
    int ret = hex_decode_streaming(encoded, strlen(encoded),
                                   decoded, &decoded_end, sizeof(decoded),
                                   &consumed, &packet_complete);
    
    TEST_ASSERT_EQ(0, ret, "Decode should succeed");
    TEST_ASSERT_EQ(16, decoded_end, "Should decode 16 bytes");
    TEST_ASSERT_EQ(1, packet_complete, "Packet should be complete");
    TEST_ASSERT_MEM_EQ(original, decoded, 16, "Roundtrip should preserve data");
    
    return 0;
}

/*
 * Main test runner
 */
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    TEST_SUITE_BEGIN("Hex Codec Unit Tests");
    
    printf("\n[is_hex_digit]\n");
    RUN_TEST(test_is_hex_digit_valid);
    RUN_TEST(test_is_hex_digit_invalid);
    
    printf("\n[hex_pair_to_byte]\n");
    RUN_TEST(test_hex_pair_to_byte_valid);
    RUN_TEST(test_hex_pair_to_byte_invalid);
    
    printf("\n[hex_encode]\n");
    RUN_TEST(test_hex_encode_basic);
    RUN_TEST(test_hex_encode_single);
    RUN_TEST(test_hex_encode_all_values);
    RUN_TEST(test_hex_encode_empty);
    
    printf("\n[hex_decode_streaming]\n");
    RUN_TEST(test_hex_decode_streaming_basic);
    RUN_TEST(test_hex_decode_streaming_no_delimiter);
    RUN_TEST(test_hex_decode_streaming_incomplete_pair);
    RUN_TEST(test_hex_decode_streaming_leading_whitespace);
    RUN_TEST(test_hex_decode_streaming_multiple_packets);
    RUN_TEST(test_hex_decode_streaming_mixed_case);
    RUN_TEST(test_hex_decode_streaming_overflow);
    RUN_TEST(test_hex_decode_streaming_empty);
    RUN_TEST(test_hex_decode_streaming_only_whitespace);
    
    printf("\n[Roundtrip]\n");
    RUN_TEST(test_roundtrip);
    
    TEST_SUITE_END();
}
