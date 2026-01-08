// Redefining stuff found in linux/if.h and linux/if_tun.h because some
// platforms don't have those (like musl on ubuntu)

#ifndef _LINUX_IF_H
#define _LINUX_IF_H

#include <sys/socket.h>

#define IFNAMSIZ	16

#define IFF_UP		0x0001
#define IFF_BROADCAST	0x0002
#define IFF_DEBUG	0x0004
#define IFF_LOOPBACK	0x0008
#define IFF_POINTOPOINT	0x0010
#define IFF_NOTRAILERS	0x0020
#define IFF_RUNNING	0x0040
#define IFF_NOARP	0x0080
#define IFF_PROMISC	0x0100
#define IFF_ALLMULTI	0x0200
#define IFF_MASTER	0x0400
#define IFF_SLAVE	0x0800
#define IFF_MULTICAST	0x1000
#define IFF_PORTSEL	0x2000
#define IFF_AUTOMEDIA	0x4000
#define IFF_DYNAMIC	0x8000

#define IFF_TUN		0x0001
#define IFF_TAP		0x0002
#define IFF_NO_PI	0x1000

#define TUNSETIFF    _IOW('T', 202, int)

struct ifmap {
	unsigned long   mem_start;
	unsigned long   mem_end;
	unsigned short  base_addr;
	unsigned char   irq;
	unsigned char   dma;
	unsigned char   port;
};

struct ifreq {
	char ifr_name[IFNAMSIZ];
	union {
		struct sockaddr ifr_addr;
		struct sockaddr ifr_dstaddr;
		struct sockaddr ifr_broadaddr;
		struct sockaddr ifr_netmask;
		struct sockaddr ifr_hwaddr;
		short           ifr_flags;
		int             ifr_ifindex;
		int             ifr_metric;
		int             ifr_mtu;
		struct ifmap    ifr_map;
		char            ifr_slave[IFNAMSIZ];
		char            ifr_newname[IFNAMSIZ];
		char           *ifr_data;
	};
};

#endif
