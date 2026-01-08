#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "linux_if.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <poll.h>

#include "tunstdio_lib.h"

#define MAX_PACKET_SIZE 0xffff

int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (*dev) {
        snprintf(ifr.ifr_name, IFNAMSIZ, "%s", dev);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        perror("ioctl");
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
    return fd;
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }

    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    char tun_name[IFNAMSIZ];
    int tun_fd;
    struct ifreq ifr;
    struct sockaddr_in addr;
    int sockfd;
    ssize_t n;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip_address/prefix>\n", argv[0]);
        exit(1);
    }

    tun_name[0] = '\0';
    tun_fd = tun_alloc(tun_name);
    if (tun_fd < 0) {
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, tun_name);

    /* Parse IP address and optional prefix using library function */
    char ip_addr_str[INET_ADDRSTRLEN];
    int prefix_len;

    if (parse_ip_prefix(argv[1], ip_addr_str, sizeof(ip_addr_str), &prefix_len) < 0) {
        fprintf(stderr, "Invalid IP address/prefix: %s\n", argv[1]);
        exit(1);
    }

    /* Validate IP address using library function */
    if (!validate_ip_address(ip_addr_str)) {
        fprintf(stderr, "Invalid IP address: %s\n", ip_addr_str);
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr_str, &addr.sin_addr);
    memcpy(&ifr.ifr_addr, &addr, sizeof(struct sockaddr));

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
        perror("ioctl(SIOCSIFADDR)");
        exit(1);
    }

    if (prefix_len != -1) {
        struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
        memset(netmask, 0, sizeof(struct sockaddr_in));
        netmask->sin_family = AF_INET;
        /* Use library function for netmask calculation */
        netmask->sin_addr.s_addr = prefix_to_netmask(prefix_len);

        if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
            perror("ioctl(SIOCSIFNETMASK)");
            exit(1);
        }
    }

    ifr.ifr_flags |= IFF_UP;
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        exit(1);
    }

    close(sockfd);

    fprintf(stderr, "TUN device %s is up with IP %s\n", tun_name, argv[1]);

    struct pollfd fds[2];
    memset(fds, 0, sizeof(fds));
    fds[0].events = fds[1].events = POLLIN;
    fds[0].fd = STDIN_FILENO;
    fds[1].fd = tun_fd;

    unsigned char tun_buf[MAX_PACKET_SIZE];
    size_t stdio_buf_ptr = 0;
    char stdio_buf[MAX_PACKET_SIZE * 2];
    size_t stdio_pkt_buf_end = 0;
    unsigned char stdio_pkt_buf[MAX_PACKET_SIZE];

    /* Buffer for hex-encoded output */
    char hex_output[MAX_PACKET_SIZE * 2 + 2];

    int pollret;
    while ((pollret = poll(fds, 2, -1)) != 0) {
        if (pollret < 0) {
            perror("poll");
            exit(1);
        }

        if (fds[0].revents & POLLIN) {
            ssize_t bytes_read = read(STDIN_FILENO, &stdio_buf[stdio_buf_ptr], 
                                      sizeof(stdio_buf) - stdio_buf_ptr);
            if (bytes_read <= 0) {
                if (bytes_read < 0) {
                    perror("read(stdin)");
                }
                break;
            }
            stdio_buf_ptr += bytes_read;
            fprintf(stderr, "Read bytes, stdio_buf_ptr %ld\n", stdio_buf_ptr);

            /* Use library function to decode hex data */
            size_t consumed = 0;
            int packet_complete = 0;
            
            while (stdio_buf_ptr > 0) {
                int ret = hex_decode_streaming(stdio_buf, stdio_buf_ptr,
                                               stdio_pkt_buf, &stdio_pkt_buf_end,
                                               sizeof(stdio_pkt_buf),
                                               &consumed, &packet_complete);
                
                if (ret < 0) {
                    fprintf(stderr, "Packet buffer overflow\n");
                    stdio_pkt_buf_end = 0;
                    break;
                }

                /* Move consumed data out of buffer */
                if (consumed > 0) {
                    memmove(stdio_buf, &stdio_buf[consumed], stdio_buf_ptr - consumed);
                    stdio_buf_ptr -= consumed;
                }

                if (packet_complete && stdio_pkt_buf_end > 0) {
                    fprintf(stderr, "Writing packet of %zu bytes to TUN\n", stdio_pkt_buf_end);
                    if (write(tun_fd, stdio_pkt_buf, stdio_pkt_buf_end) < 0) {
                        perror("write(tun_fd)");
                        exit(1);
                    }
                    stdio_pkt_buf_end = 0;
                } else if (!packet_complete) {
                    /* Need more data */
                    break;
                }
            }
        }

        if (fds[1].revents & POLLIN) {
            n = read(tun_fd, tun_buf, sizeof(tun_buf));
            if (n < 0) {
                perror("read(tun_fd)");
                exit(1);
            }
            fprintf(stderr, "Read %zd bytes from tun\n", n);

            /* Use library function to encode to hex */
            hex_encode(tun_buf, n, hex_output);
            fprintf(stdout, "%s\n", hex_output);
            fflush(stdout);
        }
    }

    return 0;
}
