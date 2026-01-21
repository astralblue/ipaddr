/*
 * ipaddr_compare.c - Address comparison and containment operations
 */

#include "ipaddr.h"

#include <string.h>

/*
 * Compare two IP addresses.
 * Returns: -1 if a < b, 0 if a == b, 1 if a > b.
 * Comparison order: family, then address value, then prefix length.
 */
int ipaddr_cmp(const ipaddr_t *a, const ipaddr_t *b)
{
    int fam_a = ipaddr_family(a);
    int fam_b = ipaddr_family(b);

    /* Compare by family first (IPv4 < IPv6) */
    if (fam_a != fam_b)
        return (fam_a == AF_INET) ? -1 : 1;

    /* Compare by address value */
    uint128_t val_a = ipaddr_to_uint128(a);
    uint128_t val_b = ipaddr_to_uint128(b);

    if (val_a < val_b)
        return -1;
    if (val_a > val_b)
        return 1;

    /* Address values equal - compare by prefix length */
    if (a->prefix_len < b->prefix_len)
        return -1;
    if (a->prefix_len > b->prefix_len)
        return 1;

    return 0;
}

/*
 * Compute netmask value for a given prefix length and bit width.
 */
static uint128_t compute_netmask(int prefix, int max_bits)
{
    if (prefix == 0) {
        return 0;
    } else if (prefix >= max_bits) {
        return (uint128_t)-1 >> (128 - max_bits);
    } else {
        uint128_t all_ones = (uint128_t)-1 >> (128 - max_bits);
        return all_ones ^ (all_ones >> prefix);
    }
}

/*
 * Get the network start address as uint128.
 */
static uint128_t get_network_start(const ipaddr_t *addr)
{
    int max_bits = ipaddr_max_prefix(addr);
    uint128_t val = ipaddr_to_uint128(addr);
    uint128_t mask = compute_netmask(addr->prefix_len, max_bits);
    return val & mask;
}

/*
 * Get the network end address as uint128.
 */
static uint128_t get_network_end(const ipaddr_t *addr)
{
    int max_bits = ipaddr_max_prefix(addr);
    uint128_t val = ipaddr_to_uint128(addr);
    uint128_t mask = compute_netmask(addr->prefix_len, max_bits);
    uint128_t hostmask = ~mask & ((uint128_t)-1 >> (128 - max_bits));
    return (val & mask) | hostmask;
}

/*
 * Check if network a is contained within network b.
 * Returns true if a is a subnet of (or equal to) b.
 */
bool ipaddr_in(const ipaddr_t *a, const ipaddr_t *b)
{
    /* Must be same family */
    if (ipaddr_family(a) != ipaddr_family(b))
        return false;

    /* a must have equal or more specific prefix than b */
    if (a->prefix_len < b->prefix_len)
        return false;

    /* Check if a's network address falls within b's range */
    uint128_t a_start = get_network_start(a);
    uint128_t b_start = get_network_start(b);
    uint128_t b_end = get_network_end(b);

    return a_start >= b_start && a_start <= b_end;
}

/*
 * Check if network a contains network b.
 */
bool ipaddr_contains(const ipaddr_t *a, const ipaddr_t *b)
{
    return ipaddr_in(b, a);
}

/*
 * Check if networks a and b overlap.
 */
bool ipaddr_overlaps(const ipaddr_t *a, const ipaddr_t *b)
{
    /* Must be same family */
    if (ipaddr_family(a) != ipaddr_family(b))
        return false;

    uint128_t a_start = get_network_start(a);
    uint128_t a_end = get_network_end(a);
    uint128_t b_start = get_network_start(b);
    uint128_t b_end = get_network_end(b);

    /* Ranges overlap if neither is completely before the other */
    return !(a_end < b_start || b_end < a_start);
}
