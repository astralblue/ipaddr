/*
 * ipaddr.h - IP address manipulation library header
 *
 * Pure C implementation of Python's ipaddress module functionality.
 */

#ifndef IPADDR_H
#define IPADDR_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 * 128-bit unsigned integer type for IPv6 address arithmetic.
 * Supported by both GCC and Clang on 64-bit platforms.
 */
typedef __uint128_t uint128_t;
typedef __int128_t  int128_t;

/*
 * Maximum buffer sizes for address string representations.
 */
#define IPADDR_INET_ADDRSTRLEN   16   /* "255.255.255.255" + NUL */
#define IPADDR_INET6_ADDRSTRLEN  46   /* Full IPv6 + NUL */
#define IPADDR_MAX_ADDRSTRLEN    64   /* With zone ID */
#define IPADDR_UINT128_STRLEN    40   /* Max decimal digits + NUL */

/*
 * Core IP address structure.
 */
typedef struct ipaddr {
    union {
        struct sockaddr_storage ss;
        struct sockaddr         sa;
        struct sockaddr_in      sin;
        struct sockaddr_in6     sin6;
    } addr;
    int   prefix_len;   /* 0-32 for IPv4, 0-128 for IPv6 */
    bool  has_prefix;   /* explicit prefix specified? */
} ipaddr_t;

/*
 * Forward declaration for command context.
 */
typedef struct ipaddr_ctx ipaddr_ctx_t;

/*
 * Command handler function type.
 */
typedef int (*cmd_fn)(ipaddr_ctx_t *ctx);

/*
 * Command definition structure.
 */
typedef struct {
    const char *name;
    const char *alias;      /* e.g., "prefixlen" for "prefix-length" */
    int         min_args;
    int         max_args;
    bool        chainable;  /* can output feed next command? */
    bool        needs_prefix; /* requires explicit /N? */
    cmd_fn      handler;
} cmd_t;

/*
 * Command context structure.
 */
struct ipaddr_ctx {
    bool       netmask_mode;  /* -M flag: output prefix as netmask */
    bool       silent;        /* suppress output (for chained commands) */
    ipaddr_t   current;       /* current address being processed */
    int        argc;          /* remaining argument count */
    char     **argv;          /* remaining arguments */
};

/*
 * Error codes.
 */
#define IPADDR_OK           0
#define IPADDR_ERR_BOOL     1   /* Boolean false (for is-* commands) */
#define IPADDR_ERR_USAGE    2   /* Usage/parse error */
#define IPADDR_ERR_INTERNAL 3   /* Internal error */

/* ========== ipaddr_parse.c ========== */

/*
 * Parse an IP address string with optional prefix.
 * Supports:
 *   - IPv4: "192.168.1.1", "192.168.1.0/24", "192.168.1.0/255.255.255.0"
 *   - IPv6: "2001:db8::1", "2001:db8::/32", "fe80::1%eth0"
 *
 * Returns: 0 on success, non-zero on error.
 * On error, errmsg is set to a static error message string.
 */
int ipaddr_parse(const char *str, ipaddr_t *addr, const char **errmsg);

/*
 * Validate that a netmask has contiguous 1-bits.
 * Returns the prefix length (0-32 for IPv4, 0-128 for IPv6) on success,
 * or -1 if the mask is invalid.
 */
int ipaddr_validate_netmask(const ipaddr_t *mask);

/* ========== ipaddr_format.c ========== */

/*
 * Format an IP address to a string buffer.
 * If netmask_mode is true and has_prefix, append "/netmask" instead of "/N".
 *
 * Returns: 0 on success, non-zero on error.
 */
int ipaddr_format(const ipaddr_t *addr, char *buf, size_t buflen, bool netmask_mode);

/*
 * Format just the address portion (no prefix) to a string buffer.
 */
int ipaddr_format_addr(const ipaddr_t *addr, char *buf, size_t buflen);

/*
 * Format the address as a packed hex string (no colons/dots).
 * IPv4: 8 hex digits, IPv6: 32 hex digits.
 */
int ipaddr_format_packed(const ipaddr_t *addr, char *buf, size_t buflen);

/* ========== ipaddr_uint128.c ========== */

/*
 * Convert an IP address to a 128-bit unsigned integer.
 * IPv4 addresses are returned as 32-bit values in the low bits.
 */
uint128_t ipaddr_to_uint128(const ipaddr_t *addr);

/*
 * Set an IP address from a 128-bit unsigned integer.
 * Preserves the address family and prefix info from the template.
 */
void ipaddr_from_uint128(ipaddr_t *addr, uint128_t val, const ipaddr_t *tmpl);

/*
 * Convert a 128-bit unsigned integer to a decimal string.
 * buf must be at least IPADDR_UINT128_STRLEN bytes.
 */
void uint128_to_str(uint128_t val, char *buf, size_t buflen);

/*
 * Parse a decimal string to a 128-bit unsigned integer.
 * Returns 0 on success, non-zero on error.
 */
int str_to_uint128(const char *str, uint128_t *val);

/* ========== ipaddr_prefix.c ========== */

/*
 * Get the maximum prefix length for an address (32 for IPv4, 128 for IPv6).
 */
int ipaddr_max_prefix(const ipaddr_t *addr);

/*
 * Compute the netmask for a given prefix length.
 * Result is stored in mask, inheriting family from addr.
 */
void ipaddr_netmask(const ipaddr_t *addr, ipaddr_t *mask);

/*
 * Compute the hostmask (inverse of netmask) for a given prefix length.
 */
void ipaddr_hostmask(const ipaddr_t *addr, ipaddr_t *mask);

/* ========== ipaddr_classify.c ========== */

bool ipaddr_is_loopback(const ipaddr_t *addr);
bool ipaddr_is_private(const ipaddr_t *addr);
bool ipaddr_is_global(const ipaddr_t *addr);
bool ipaddr_is_multicast(const ipaddr_t *addr);
bool ipaddr_is_link_local(const ipaddr_t *addr);
bool ipaddr_is_unspecified(const ipaddr_t *addr);
bool ipaddr_is_reserved(const ipaddr_t *addr);

/* ========== ipaddr_network.c ========== */

/*
 * Compute the network address (zeroed host bits).
 */
void ipaddr_network(const ipaddr_t *addr, ipaddr_t *net);

/*
 * Compute the broadcast address (all host bits set to 1).
 */
void ipaddr_broadcast(const ipaddr_t *addr, ipaddr_t *bcast);

/*
 * Get the host at a given index within the network.
 * Negative indices count from the end (-1 = last host).
 */
int ipaddr_host(const ipaddr_t *net, int128_t index, ipaddr_t *host);

/*
 * Get the index of the current address within its network.
 */
uint128_t ipaddr_host_index(const ipaddr_t *addr);

/*
 * Get the number of addresses in the network.
 */
uint128_t ipaddr_num_addresses(const ipaddr_t *addr);

/*
 * Get a subnet of the current network.
 * new_prefix: absolute prefix length or relative (+N from current).
 * index: subnet index (negative counts from end).
 * If preserve_host is true, preserve host bits from original address.
 */
int ipaddr_subnet(const ipaddr_t *addr, int new_prefix, int128_t index,
                  bool preserve_host, ipaddr_t *subnet);

/*
 * Get the supernet of the current network.
 * new_prefix: absolute prefix length or relative (-N from current).
 */
int ipaddr_super(const ipaddr_t *addr, int new_prefix, ipaddr_t *super);

/* ========== ipaddr_ipv6.c ========== */

/*
 * Get the zone ID string from an IPv6 address.
 * Returns NULL if no zone ID is present.
 */
const char *ipaddr_zone_id(const ipaddr_t *addr);

/*
 * Get the numeric scope ID from an IPv6 address.
 */
uint32_t ipaddr_scope_id(const ipaddr_t *addr);

/*
 * Extract IPv4 address from IPv6 (mapped, compatible, or any).
 * For non-IPv6 addresses, just copies the address.
 * Returns 0 on success, non-zero if no IPv4 representation exists.
 */
int ipaddr_to_ipv4(const ipaddr_t *addr, ipaddr_t *v4);

/*
 * Extract the embedded IPv4 address from a 6to4 address.
 * Returns 0 on success, non-zero if not a 6to4 address.
 */
int ipaddr_6to4(const ipaddr_t *addr, ipaddr_t *v4);

/*
 * Extract Teredo information.
 * mode: 0 = server address, 1 = client address
 * Returns 0 on success, non-zero if not a Teredo address.
 */
int ipaddr_teredo(const ipaddr_t *addr, int mode, ipaddr_t *result);

/* ========== ipaddr_compare.c ========== */

/*
 * Compare two IP addresses.
 * Returns: -1 if a < b, 0 if a == b, 1 if a > b.
 * Comparison is by address value, then by prefix length if equal.
 */
int ipaddr_cmp(const ipaddr_t *a, const ipaddr_t *b);

/*
 * Check if network a is contained within network b.
 */
bool ipaddr_in(const ipaddr_t *a, const ipaddr_t *b);

/*
 * Check if network a contains network b.
 */
bool ipaddr_contains(const ipaddr_t *a, const ipaddr_t *b);

/*
 * Check if networks a and b overlap.
 */
bool ipaddr_overlaps(const ipaddr_t *a, const ipaddr_t *b);

/* ========== Utility functions ========== */

/*
 * Get the address family (AF_INET or AF_INET6).
 */
static inline int ipaddr_family(const ipaddr_t *addr) {
    return addr->addr.sa.sa_family;
}

/*
 * Check if address is IPv4.
 */
static inline bool ipaddr_is_ipv4(const ipaddr_t *addr) {
    return addr->addr.sa.sa_family == AF_INET;
}

/*
 * Check if address is IPv6.
 */
static inline bool ipaddr_is_ipv6(const ipaddr_t *addr) {
    return addr->addr.sa.sa_family == AF_INET6;
}

/*
 * Get pointer to raw address bytes.
 */
static inline const void *ipaddr_bytes(const ipaddr_t *addr) {
    if (ipaddr_is_ipv4(addr))
        return &addr->addr.sin.sin_addr;
    else
        return &addr->addr.sin6.sin6_addr;
}

/*
 * Get size of raw address bytes.
 */
static inline size_t ipaddr_bytes_len(const ipaddr_t *addr) {
    return ipaddr_is_ipv4(addr) ? 4 : 16;
}

#endif /* IPADDR_H */
