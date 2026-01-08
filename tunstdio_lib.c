#include "tunstdio_lib.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>

int is_hex_digit(char c) {
    return (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

int hex_pair_to_byte(const char *hex) {
    if (!hex || !is_hex_digit(hex[0]) || !is_hex_digit(hex[1])) {
        return -1;
    }
    
    char pair[3] = {hex[0], hex[1], '\0'};
    return (int)strtol(pair, NULL, 16);
}

void hex_encode(const unsigned char *data, size_t len, char *hex_out) {
    static const char hex_chars[] = "0123456789abcdef";
    
    for (size_t i = 0; i < len; i++) {
        hex_out[i * 2] = hex_chars[(data[i] >> 4) & 0x0F];
        hex_out[i * 2 + 1] = hex_chars[data[i] & 0x0F];
    }
    hex_out[len * 2] = '\0';
}

int hex_decode_streaming(const char *hex_in, size_t hex_len,
                         unsigned char *pkt_buf, size_t *pkt_buf_end, size_t pkt_buf_sz,
                         size_t *consumed, int *packet_complete) {
    size_t i;
    *packet_complete = 0;
    
    for (i = 0; i < hex_len; i++) {
        if (isspace((unsigned char)hex_in[i])) {
            // Whitespace acts as packet delimiter
            if (*pkt_buf_end > 0) {
                *packet_complete = 1;
                *consumed = i + 1;
                return 0;
            }
            // Skip leading/consecutive whitespace
            continue;
        }
        
        // Need at least 2 chars for a hex pair
        if (hex_len - i < 2) {
            break;
        }
        
        // Validate hex characters
        if (!is_hex_digit(hex_in[i]) || !is_hex_digit(hex_in[i + 1])) {
            // Invalid hex, skip this character
            continue;
        }
        
        // Check for buffer overflow
        if (*pkt_buf_end >= pkt_buf_sz) {
            return -1;
        }
        
        int byte_val = hex_pair_to_byte(&hex_in[i]);
        if (byte_val < 0) {
            continue;
        }
        
        pkt_buf[*pkt_buf_end] = (unsigned char)byte_val;
        (*pkt_buf_end)++;
        i++; // Skip the second hex char (loop will increment once more)
    }
    
    *consumed = i;
    return 0;
}

int parse_ip_prefix(const char *input, char *ip_out, size_t ip_out_sz, int *prefix_out) {
    if (!input || !ip_out || !prefix_out || ip_out_sz == 0) {
        return -1;
    }
    
    *prefix_out = -1;
    
    const char *slash = strchr(input, '/');
    
    if (slash) {
        size_t ip_len = (size_t)(slash - input);
        if (ip_len >= ip_out_sz) {
            return -1; // IP too long
        }
        
        strncpy(ip_out, input, ip_len);
        ip_out[ip_len] = '\0';
        
        // Parse prefix
        const char *prefix_str = slash + 1;
        if (*prefix_str == '\0') {
            return -1; // Empty prefix after slash
        }
        
        char *endptr;
        long prefix = strtol(prefix_str, &endptr, 10);
        
        if (*endptr != '\0' || prefix < 0 || prefix > 32) {
            return -1; // Invalid prefix
        }
        
        *prefix_out = (int)prefix;
    } else {
        size_t len = strlen(input);
        if (len >= ip_out_sz) {
            return -1;
        }
        strncpy(ip_out, input, ip_out_sz);
        ip_out[ip_out_sz - 1] = '\0';
    }
    
    return 0;
}

int validate_ip_address(const char *ip_str) {
    struct in_addr addr;
    return inet_pton(AF_INET, ip_str, &addr) == 1;
}

uint32_t prefix_to_netmask(int prefix_len) {
    if (prefix_len <= 0) {
        return 0;
    }
    if (prefix_len >= 32) {
        return htonl(0xFFFFFFFFU);
    }
    uint32_t mask = 0xFFFFFFFFU << (32 - prefix_len);
    return htonl(mask);
}
