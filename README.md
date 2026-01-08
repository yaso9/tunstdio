# tunstdio

tunstdio creates a virtual network interface (TUN device) and tunnels traffic through stdin/stdout. By running tunstdio on two different computers and connecting their stdio together, you can create a simple network tunnel/VPN.

## How it Works

tunstdio creates a TUN device, assigns it an IP address, and then:
- Reads packets from the TUN device, encodes them as hexadecimal, and writes to stdout
- Reads hex-encoded packets from stdin, decodes them, and writes them to the TUN device

This design allows you to use any transport mechanism you want to connect the two instances - just pipe stdin/stdout to your chosen method.

## Building

```bash
make
```

Optional: install system-wide
```bash
sudo make install
```

Run tests:
```bash
make check
```

## Download Pre-built Binaries

Statically compiled binaries are available for download from the [releases page](https://github.com/ysohail/tunstdio/releases). These binaries are built with musl-libc and are completely self-contained, requiring no additional dependencies.

## Usage

Basic usage:
```bash
sudo ./tunstdio <ip_address/prefix>
```

The IP address and optional CIDR prefix will be assigned to the TUN device.

## Examples

### TCP Tunnel with socat

**Computer A (IP: 10.0.0.1/24):**
```bash
sudo socat exec:'./tunstdio 10.0.0.1/24' TCP-LISTEN:9000,reuseaddr,fork
```

**Computer B (IP: 10.0.0.2/24):**
```bash
sudo socat exec:'./tunstdio 10.0.0.2/24' TCP:computer_a_ip:9000
```

Now you can communicate between the two computers using the 10.0.0.x addresses.

## Requirements

- Linux with TUN/TAP support
- gcc compiler
- Root/sudo access (required for creating TUN devices)

## Protocol Details

tunstdio uses a simple hex encoding protocol:
- Each packet is encoded as hexadecimal (e.g., `4567001d000040004010e0a0a0001010a000002080008b94000000`)
- Packets are delimited by newlines
- Whitespace in the input is treated as a packet delimiter

## Security Note

This is a simple tunneling tool with no encryption or authentication. For production use over untrusted networks, consider using an encrypted transport like SSH or wrapping the connection with a VPN.

## License

BSD Zero Clause License - see LICENSE file for details.
