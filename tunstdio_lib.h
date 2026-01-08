#ifndef TUNSTDIO_LIB_H
#define TUNSTDIO_LIB_H

#include <stddef.h>
#include <stdint.h>

/**
 * Parse an IP address with optional CIDR prefix notation.
 * 
 * @param input     Input string (e.g., "192.168.1.1/24" or "10.0.0.1")
 * @param ip_out    Buffer to store the IP address string (must be at least INET_ADDRSTRLEN)
 * @param ip_out_sz Size of the ip_out buffer
 * @param prefix_out Pointer to store the prefix length (-1 if no prefix specified)
 * @return 0 on success, -1 on error
 */
int parse_ip_prefix(const char *input, char *ip_out, size_t ip_out_sz, int *prefix_out);

/**
 * Validate an IP address string.
 * 
 * @param ip_str The IP address string to validate
 * @return 1 if valid, 0 if invalid
 */
int validate_ip_address(const char *ip_str);

/**
 * Decode hex-encoded data from a buffer, handling whitespace as packet delimiters.
 * 
 * This function processes a buffer containing hex-encoded data. Whitespace characters
 * act as packet delimiters. The function processes complete hex pairs and returns
 * information about how many input characters were consumed.
 * 
 * @param hex_in      Input buffer containing hex-encoded data
 * @param hex_len     Length of input buffer
 * @param pkt_buf     Output buffer for the current packet being built
 * @param pkt_buf_end Pointer to current position in pkt_buf (updated by function)
 * @param pkt_buf_sz  Size of the packet buffer
 * @param consumed    Pointer to store number of input bytes consumed
 * @param packet_complete Set to 1 if a complete packet was found (whitespace delimiter hit)
 * @return 0 on success, -1 on error (buffer overflow)
 */
int hex_decode_streaming(const char *hex_in, size_t hex_len, 
                         unsigned char *pkt_buf, size_t *pkt_buf_end, size_t pkt_buf_sz,
                         size_t *consumed, int *packet_complete);

/**
 * Decode a single hex pair to a byte.
 * 
 * @param hex Two-character hex string (must be valid hex)
 * @return The decoded byte value, or -1 if invalid hex
 */
int hex_pair_to_byte(const char *hex);

/**
 * Encode a byte buffer to hex string.
 * 
 * @param data    Input byte buffer
 * @param len     Length of input buffer
 * @param hex_out Output buffer (must be at least len*2 + 1 bytes)
 */
void hex_encode(const unsigned char *data, size_t len, char *hex_out);

/**
 * Check if a character is a valid hex digit.
 * 
 * @param c The character to check
 * @return 1 if valid hex digit, 0 otherwise
 */
int is_hex_digit(char c);

/**
 * Calculate netmask from prefix length.
 * 
 * @param prefix_len Prefix length (0-32)
 * @return Network mask in network byte order
 */
uint32_t prefix_to_netmask(int prefix_len);

#endif /* TUNSTDIO_LIB_H */
