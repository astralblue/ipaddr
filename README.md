# ipaddr - IP Address Manipulation Tool

A pure C implementation providing features similar to Python's `ipaddress` module for command-line IP address manipulation and querying.

## Synopsis

```bash
ipaddr [OPTIONS] <address> [command [arguments...]]...
```

Commands can be chained: operations that output addresses can feed into subsequent operations.

## Building

```bash
make
sudo make install  # installs to /usr/local/bin by default
```

## Address Formats

The tool accepts three input formats:
- **Address**: `192.168.1.1` or `2001:db8::1`
- **CIDR/Network**: `192.168.1.0/24` or `2001:db8::/32`
- **Interface Address**: `192.168.1.30/28` (address with prefix length, may have non-zero host bits)

Addresses are parsed using `getaddrinfo(AI_NUMERICHOST)` and normalized through `getnameinfo(NI_NUMERICHOST)`, ensuring proper handling of IPv6 zone IDs and address normalization.

### Prefix Length vs Netmask

The prefix length can be specified in two ways:
- **Decimal prefix length**: `/24`, `/64`
- **Netmask notation**: `/255.255.255.0`, `/ffff:ffff:ffff:ffff::`

Internally, all netmasks are converted to prefix lengths. Netmasks must represent valid CIDR prefixes (contiguous 1-bits followed by 0-bits).

## Global Options

- `-M` : Print prefix lengths as netmasks instead of `/N` notation

## Commands

### Basic Information

#### Default (no command)
Prints the normalized address after `getaddrinfo`→`getnameinfo` roundtrip.

```bash
ipaddr 192.168.001.1
# Output: 192.168.1.1

ipaddr 2001:0db8:0000:0000:0000:0000:0000:0001
# Output: 2001:db8::1 (normalized by getnameinfo)

ipaddr fe80::1%eth0
# Output: fe80::1%eth0
```

Note: `getnameinfo()` may preserve embedded IPv4 syntax for IPv4-mapped/compatible addresses (e.g., `::ffff:192.168.1.1`) rather than converting to pure hexadecimal form.

#### `version`
Prints IP version (4 or 6).

```bash
ipaddr 192.168.1.1 version
# Output: 4

ipaddr ::1 version
# Output: 6
```

#### `packed`
Prints hex bytes of the address (no separators).

```bash
ipaddr 127.0.0.1 packed
# Output: 7f000001

ipaddr ::1 packed
# Output: 00000000000000000000000000000001
```

#### `to-int`
Prints the address as a decimal integer (32-bit for IPv4, 128-bit for IPv6).

```bash
ipaddr 192.168.1.1 to-int
# Output: 3232235777

ipaddr ::1 to-int
# Output: 1
```

### Prefix/Netmask Information

#### `prefix-length` (alias: `prefixlen`)
Prints the prefix length. Returns 32 for IPv4 addresses or 128 for IPv6 addresses without explicit prefix.

```bash
ipaddr 192.168.1.0/24 prefix-length
# Output: 24

ipaddr 192.168.1.1 prefix-length
# Output: 32
```

#### `netmask`
Prints the netmask in address form.

```bash
ipaddr 192.168.1.0/24 netmask
# Output: 255.255.255.0

ipaddr 2001:db8::/32 netmask
# Output: ffff:ffff::
```

#### `hostmask`
Prints the hostmask (inverse of netmask) in address form.

```bash
ipaddr 192.168.1.0/24 hostmask
# Output: 0.0.0.255

ipaddr 2001:db8::/32 hostmask
# Output: ::ffff:ffff:ffff:ffff:ffff:ffff
```

### Address Components

#### `address`
Prints only the address part, discarding prefix length if present.

```bash
ipaddr 192.168.1.30/28 address
# Output: 192.168.1.30
```

#### `network`
Prints the network address (zeroes host bits) with prefix length.

```bash
ipaddr 192.168.1.30/28 network
# Output: 192.168.1.16/28

ipaddr 192.168.1.1 network
# Output: 192.168.1.1/32
```

#### `broadcast`
Prints the broadcast address for a network. Cannot be used with plain addresses (without prefix length).

```bash
ipaddr 192.168.1.16/28 broadcast
# Output: 192.168.1.31

ipaddr 2001:db8::/64 broadcast
# Output: 2001:db8::ffff:ffff:ffff:ffff
```

### IPv6-Specific Commands

#### `zone-id`
Prints the zone ID including the `%` prefix. Empty for IPv4.

```bash
ipaddr fe80::1%eth0 zone-id
# Output: %eth0

ipaddr 192.168.1.1 zone-id
# Output: (empty)
```

#### `scope-id`
Prints the numeric scope ID. Returns 0 for IPv4.

```bash
ipaddr fe80::1%2 scope-id
# Output: 2

ipaddr 192.168.1.1 scope-id
# Output: 0
```

#### `ipv4`
Extracts the last 32 bits and prints them in IPv4 notation. For IPv4 addresses, returns normalized form.

```bash
ipaddr ::ffff:0102:0304 ipv4
# Output: 1.2.3.4

ipaddr 192.168.1.1 ipv4
# Output: 192.168.1.1
```

#### `6to4`
Extracts the embedded IPv4 address from a 6to4 address (2002::/16). Returns error if not a valid 6to4 address.

```bash
ipaddr 2002:c000:0201::1 6to4
# Output: 192.0.2.1
```

#### `teredo server|client`
Extracts the Teredo server or client IPv4 address from a Teredo address (2001::/32).

```bash
ipaddr 2001:0000:4136:e378:8000:63bf:3fff:fdd2 teredo server
# Output: 65.54.227.120

ipaddr 2001:0000:4136:e378:8000:63bf:3fff:fdd2 teredo client
# Output: 192.0.2.45
```

### Address Classification

Returns exit code 0 (true) or 1 (false).

```bash
ipaddr <address> is-multicast
ipaddr <address> is-private
ipaddr <address> is-global
ipaddr <address> is-unspecified
ipaddr <address> is-reserved
ipaddr <address> is-loopback
ipaddr <address> is-link-local
```

Examples:

```bash
ipaddr ::1 is-loopback
# Exit code: 0 (true)

ipaddr 192.168.1.1 is-private
# Exit code: 0 (true)

ipaddr 8.8.8.8 is-global
# Exit code: 0 (true)
```

Note: `is-private` covers RFC 1918 addresses, IPv6 ULAs (fc00::/7), and deprecated site-local addresses (fec0::/10).

### Network Operations

#### `num-addresses`
Prints the number of addresses in the network. Returns 1 for plain addresses.

```bash
ipaddr 192.168.1.0/24 num-addresses
# Output: 256

ipaddr 2001:db8::/64 num-addresses
# Output: 18446744073709551616
```

#### `host <index>`
Returns the Nth address in the network. Supports negative indexing.

```bash
ipaddr 192.168.1.16/28 host 0
# Output: 192.168.1.16

ipaddr 192.168.1.16/28 host 1
# Output: 192.168.1.17

ipaddr 192.168.1.16/28 host -1
# Output: 192.168.1.31

ipaddr 192.168.1.16/28 host -2
# Output: 192.168.1.30
```

Note: `host 0` is equivalent to `network`, and `host -1` is equivalent to `broadcast`.

#### `host-index`
Returns the host portion of an interface address as a decimal integer.

```bash
ipaddr 192.168.1.35/28 host-index
# Output: 3

ipaddr 192.168.1.16/28 host-index
# Output: 0
```

#### `subnet <prefixlen> <index>`
Returns the Nth subnet with the given prefix length. Supports negative indexing.

`<prefixlen>` can be:
- Absolute: `28`, `64`
- Relative (prefixed with `+`): `+4`, `+8`

```bash
ipaddr 192.168.1.0/24 subnet 28 0
# Output: 192.168.1.0/28

ipaddr 192.168.1.0/24 subnet 28 3
# Output: 192.168.1.48/28

ipaddr 192.168.1.0/24 subnet +5 -2
# Equivalent to: ipaddr 192.168.1.0/24 subnet 29 -2
# Output: 192.168.1.192/29
```

For interface addresses, preserves the host portion:

```bash
ipaddr 192.168.1.1/24 subnet 28 2
# Output: 192.168.1.33/28
```

#### `super <prefixlen>`
Returns the supernet with the given prefix length.

`<prefixlen>` can be:
- Absolute: `16`, `48`
- Relative (prefixed with `-`): `-8`, `-16`

```bash
ipaddr 192.168.1.0/24 super 16
# Output: 192.168.0.0/16

ipaddr 192.168.1.0/24 super -8
# Equivalent to: ipaddr 192.168.1.0/24 super 16
# Output: 192.168.0.0/16
```

### Relationship Tests

Returns exit code 0 (true) or 1 (false).

#### `in <network>`
Tests if the left operand is contained within the right network.

```bash
ipaddr 192.168.1.1/24 in 192.168.0.0/16
# Exit code: 0 (true)

ipaddr 192.168.1.1/16 in 192.168.1.0/24
# Exit code: 1 (false)
```

For interface addresses, only the network portion is compared (host bits ignored).

#### `contains <address/network>`
Equivalent to `<address/network> in <left-operand>`.

```bash
ipaddr 192.168.0.0/16 contains 192.168.1.1/24
# Exit code: 0 (true)
```

#### `overlaps <network>`
Tests if two networks overlap (either contains the other).

```bash
ipaddr 192.168.1.0/24 overlaps 192.168.1.128/25
# Exit code: 0 (true)

ipaddr 192.168.1.0/24 overlaps 192.168.2.0/24
# Exit code: 1 (false)
```

### Comparison Operations

Compares two addresses/networks. Networks are compared by network address first, then prefix length, then host bits (for interface addresses).

```bash
ipaddr <addr1> eq <addr2>   # Equal
ipaddr <addr1> ne <addr2>   # Not equal
ipaddr <addr1> lt <addr2>   # Less than
ipaddr <addr1> le <addr2>   # Less than or equal
ipaddr <addr1> gt <addr2>   # Greater than
ipaddr <addr1> ge <addr2>   # Greater than or equal
```

Returns exit code 0 (true) or 1 (false).

## Implementation Notes

### Parsing and Internal Representation

1. **Address parsing**: Uses `getaddrinfo(AI_NUMERICHOST)` for AF-independent parsing
2. **Internal storage**: Uses `struct sockaddr_storage` to preserve address family and IPv6 scope ID
3. **Address printing**: Uses `getnameinfo(NI_NUMERICHOST)` for normalized output
4. **Netmask handling**: Netmasks are validated (must be contiguous 1-bits) and converted to prefix length
5. **CIDR flag**: Each address structure tracks whether it's a plain address or CIDR/interface address

### Prefix Length vs Netmask

By default, prefix lengths are printed as `/N`. Use `-M` to force netmask notation:

```bash
ipaddr -M 192.168.1.0/24
# Output: 192.168.1.0/255.255.255.0
```

### IPv6 Zone IDs

Zone IDs (scope IDs) are preserved through the parsing pipeline and included in output where present. They depend on the local system's network interface configuration.

### 128-bit Integer Operations

Commands requiring enumeration (`host`, `subnet`, `num-addresses`) need 128-bit integer arithmetic for IPv6. These operations use appropriate integer types and algorithms to handle the full IPv6 address space.

## Operation Chaining

Operations that output addresses/networks can be chained together, allowing the output of one operation to become the input for the next. This enables complex transformations in a single command.

### Chaining Syntax

```bash
ipaddr <address> <operation1> [args...] <operation2> [args...] <operation3> [args...]
```

### Chainable Operations

The following operations output addresses/networks and can be chained:
- Default (normalization)
- `address`
- `network`
- `broadcast`
- `host <index>`
- `subnet <prefixlen> <index>`
- `super <prefixlen>`
- `ipv4`
- `6to4`
- `teredo server|client`

### Non-Chainable Operations

Operations that output non-address data (numbers, booleans) terminate the chain:
- `version`, `packed`, `to-int`
- `prefix-length`, `netmask`, `hostmask`
- `zone-id`, `scope-id`
- `num-addresses`, `host-index`
- `is-*` (classification tests)
- `in`, `contains`, `overlaps`
- `eq`, `ne`, `lt`, `le`, `gt`, `ge`

### Chaining Examples

#### Basic Subnet Navigation

```bash
# Get the 19th host in the 2nd /20 subnet of 192.168.0.0/16
ipaddr 192.168.0.0/16 subnet 20 1 host 19
# Output: 192.168.16.19

# Get broadcast of the 5th /24 subnet of 10.0.0.0/16
ipaddr 10.0.0.0/16 subnet 24 4 broadcast
# Output: 10.0.4.255

# Get network address of a specific subnet
ipaddr 192.168.0.0/16 subnet 24 10 network
# Output: 192.168.10.0/24
```

#### Supernet and Subnet Operations

```bash
# Go to supernet, then get a different subnet
ipaddr 192.168.5.0/24 super 16 subnet 24 50
# Output: 192.168.50.0/24

# Navigate through network hierarchy
ipaddr 10.1.2.3/24 network super 16 subnet 20 3 host 100
# Output: 10.1.48.100
```

#### Working with Embedded IPv4

```bash
# Extract IPv4 from 6to4, then get network info
ipaddr 2002:c000:0201::1 6to4 to-int
# Output: 3221226241

# Get the broadcast of extracted IPv4 (if it were /24)
ipaddr ::ffff:192.168.1.1 ipv4 network
# Output: 192.168.1.1/32 (plain address becomes /32)
```

#### Complex Subnet Calculations

```bash
# Get last host in the last subnet
ipaddr 10.0.0.0/16 subnet 24 -1 host -1
# Output: 10.0.255.255

# Get the first host (after network address) in a subnet
ipaddr 172.16.0.0/12 subnet 24 0 host 1
# Output: 172.16.0.1

# Navigate to a specific subnet and get its supernet
ipaddr 192.168.0.0/16 subnet 28 100 super 24
# Output: 192.168.6.0/24
```

#### Preserving Host Bits in Interface Addresses

```bash
# Subnet an interface address (preserves host portion)
ipaddr 192.168.1.1/16 subnet 24 5 host 50
# Output: 192.168.5.50

# This works because:
# 1. 192.168.1.1/16 subnet 24 5 -> 192.168.5.1/24 (host .1 preserved)
# 2. 192.168.5.1/24 host 50 -> 192.168.5.50
```

#### Extracting and Converting

```bash
# Get Teredo client address and convert to integer
ipaddr 2001:0000:4136:e378:8000:63bf:3fff:fdd2 teredo client to-int
# Output: 3221225997

# Get broadcast address and convert to packed form
ipaddr 10.0.0.0/24 broadcast packed
# Output: 0a0000ff
```

#### Comparison After Transformation

```bash
# Check if two subnets overlap
ipaddr 10.0.0.0/16 subnet 24 5 in 10.0.0.0/20
# Exit code: 0 (true)

# Compare derived addresses
ipaddr 192.168.0.0/16 subnet 24 10 host 5 lt 192.168.10.10
# Exit code: 0 (true, 192.168.10.5 < 192.168.10.10)
```

#### Practical Use Cases

```bash
# Allocate DHCP pool: get addresses 10-254 in the first subnet
for i in {10..254}; do
  ipaddr 10.0.0.0/16 subnet 24 0 host $i
done

# Find the gateway (first host) in each of 8 subnets
for i in {0..7}; do
  ipaddr 172.16.0.0/16 subnet 24 $i host 1
done

# Get the last usable address in a subnet (broadcast - 1)
ipaddr 192.168.1.0/24 host -2
# Output: 192.168.1.254
```

## Exit Codes

- `0`: Success (or true for boolean tests)
- `1`: Failure (or false for boolean tests)
- `2`: Invalid arguments or usage error

## Examples

```bash
# Normalize addresses
ipaddr 192.168.001.001
# 192.168.1.1

# Extract network
ipaddr 192.168.1.35/24 network
# 192.168.1.0/24

# List first 5 hosts
for i in {0..4}; do ipaddr 10.0.0.0/24 host $i; done
# 10.0.0.0
# 10.0.0.1
# 10.0.0.2
# 10.0.0.3
# 10.0.0.4

# Check if address is private
if ipaddr 192.168.1.1 is-private; then
    echo "Private address"
fi

# Subnet calculation
ipaddr 10.0.0.0/8 subnet 16 256
# 10.1.0.0/16

# Extract IPv4 from IPv4-mapped IPv6
ipaddr ::ffff:192.168.1.1 ipv4
# 192.168.1.1

# Compare addresses
ipaddr 192.168.1.1 lt 192.168.1.2
# Exit code: 0 (true)
```

## See Also

- RFC 4291 (IPv6 Addressing Architecture)
- RFC 4193 (Unique Local IPv6 Unicast Addresses)
- RFC 3056 (Connection of IPv6 Domains via IPv4 Clouds - 6to4)
- RFC 4380 (Teredo: Tunneling IPv6 over UDP through NATs)
- Python `ipaddress` module documentation
