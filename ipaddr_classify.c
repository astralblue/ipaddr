/*
 * ipaddr_classify.c - IP address classification functions
 */

#include "ipaddr.h"

#include <string.h>
#include <arpa/inet.h>

/*
 * Network range structure for classification tables.
 */
typedef struct {
    uint128_t network;
    int       prefix;
} net_range_t;

/*
 * Check if an address matches a network range.
 */
static bool in_range(const ipaddr_t *addr, uint128_t network, int prefix)
{
    uint128_t val = ipaddr_to_uint128(addr);
    int max_bits = ipaddr_max_prefix(addr);
    uint128_t mask;

    if (prefix == 0) {
        mask = 0;
    } else if (prefix >= max_bits) {
        mask = (uint128_t)-1 >> (128 - max_bits);
    } else {
        uint128_t all_ones = (uint128_t)-1 >> (128 - max_bits);
        mask = all_ones ^ (all_ones >> prefix);
    }

    return (val & mask) == network;
}

/*
 * Check if an IPv4 address matches any range in a table.
 */
static bool match_ipv4_ranges(const ipaddr_t *addr, const net_range_t *ranges, int count)
{
    if (!ipaddr_is_ipv4(addr))
        return false;

    for (int i = 0; i < count; i++) {
        if (in_range(addr, ranges[i].network, ranges[i].prefix))
            return true;
    }
    return false;
}

/*
 * Check if an IPv6 address matches any range in a table.
 * (Currently unused but kept for potential future use)
 */
#if 0
static bool match_ipv6_ranges(const ipaddr_t *addr, const net_range_t *ranges, int count)
{
    if (!ipaddr_is_ipv6(addr))
        return false;

    for (int i = 0; i < count; i++) {
        if (in_range(addr, ranges[i].network, ranges[i].prefix))
            return true;
    }
    return false;
}
#endif

/*
 * IPv4 loopback: 127.0.0.0/8
 */
bool ipaddr_is_loopback(const ipaddr_t *addr)
{
    if (ipaddr_is_ipv4(addr)) {
        return in_range(addr, 0x7f000000ULL, 8); /* 127.0.0.0/8 */
    } else {
        /* IPv6 loopback is exactly ::1 */
        uint128_t val = ipaddr_to_uint128(addr);
        return val == 1;
    }
}

/*
 * Private address ranges (RFC 1918 for IPv4, ULA for IPv6).
 */
bool ipaddr_is_private(const ipaddr_t *addr)
{
    if (ipaddr_is_ipv4(addr)) {
        /* RFC 1918 private ranges */
        static const net_range_t ranges[] = {
            { 0x0a000000ULL, 8 },   /* 10.0.0.0/8 */
            { 0xac100000ULL, 12 },  /* 172.16.0.0/12 */
            { 0xc0a80000ULL, 16 },  /* 192.168.0.0/16 */
        };
        return match_ipv4_ranges(addr, ranges, sizeof(ranges)/sizeof(ranges[0]));
    } else {
        /* IPv6 ULA: fc00::/7 */
        uint128_t val = ipaddr_to_uint128(addr);
        uint128_t fc00 = (uint128_t)0xfc00 << 112;
        uint128_t mask = (uint128_t)0xfe00 << 112;
        return (val & mask) == fc00;
    }
}

/*
 * Global unicast addresses.
 */
bool ipaddr_is_global(const ipaddr_t *addr)
{
    if (ipaddr_is_ipv4(addr)) {
        /* Not private, not loopback, not link-local, not multicast, not reserved */
        return !ipaddr_is_private(addr) &&
               !ipaddr_is_loopback(addr) &&
               !ipaddr_is_link_local(addr) &&
               !ipaddr_is_multicast(addr) &&
               !ipaddr_is_reserved(addr) &&
               !ipaddr_is_unspecified(addr);
    } else {
        /* IPv6 global unicast: 2000::/3 */
        uint128_t val = ipaddr_to_uint128(addr);
        uint128_t prefix_2000 = (uint128_t)0x2000 << 112;
        uint128_t mask = (uint128_t)0xe000 << 112;
        return (val & mask) == prefix_2000;
    }
}

/*
 * Multicast addresses.
 */
bool ipaddr_is_multicast(const ipaddr_t *addr)
{
    if (ipaddr_is_ipv4(addr)) {
        /* 224.0.0.0/4 */
        return in_range(addr, 0xe0000000ULL, 4);
    } else {
        /* ff00::/8 */
        uint128_t val = ipaddr_to_uint128(addr);
        uint128_t ff00 = (uint128_t)0xff00 << 112;
        uint128_t mask = (uint128_t)0xff00 << 112;
        return (val & mask) == ff00;
    }
}

/*
 * Link-local addresses.
 */
bool ipaddr_is_link_local(const ipaddr_t *addr)
{
    if (ipaddr_is_ipv4(addr)) {
        /* 169.254.0.0/16 */
        return in_range(addr, 0xa9fe0000ULL, 16);
    } else {
        /* fe80::/10 */
        uint128_t val = ipaddr_to_uint128(addr);
        uint128_t fe80 = (uint128_t)0xfe80 << 112;
        uint128_t mask = (uint128_t)0xffc0 << 112;
        return (val & mask) == fe80;
    }
}

/*
 * Unspecified address (0.0.0.0 or ::).
 */
bool ipaddr_is_unspecified(const ipaddr_t *addr)
{
    uint128_t val = ipaddr_to_uint128(addr);
    return val == 0;
}

/*
 * Reserved addresses.
 */
bool ipaddr_is_reserved(const ipaddr_t *addr)
{
    if (ipaddr_is_ipv4(addr)) {
        /* 240.0.0.0/4 (excluding 255.255.255.255 broadcast, but including it for is-reserved) */
        return in_range(addr, 0xf0000000ULL, 4);
    } else {
        /* For IPv6, addresses not in known categories could be reserved */
        /* Check for reserved ranges like ::ffff:0:0/96 (IPv4-mapped), etc. */
        /* Simplified: not global, not link-local, not multicast, not loopback, not unspecified */
        return !ipaddr_is_global(addr) &&
               !ipaddr_is_link_local(addr) &&
               !ipaddr_is_multicast(addr) &&
               !ipaddr_is_loopback(addr) &&
               !ipaddr_is_unspecified(addr) &&
               !ipaddr_is_private(addr);
    }
}
