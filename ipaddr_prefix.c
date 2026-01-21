/*
 * ipaddr_prefix.c - Prefix and netmask operations
 */

#include "ipaddr.h"

#include <string.h>

/*
 * Get the maximum prefix length for an address.
 */
int ipaddr_max_prefix(const ipaddr_t *addr)
{
    return ipaddr_is_ipv4(addr) ? 32 : 128;
}

/*
 * Compute the netmask for a given prefix length.
 */
void ipaddr_netmask(const ipaddr_t *addr, ipaddr_t *mask)
{
    int max_bits = ipaddr_max_prefix(addr);
    int prefix = addr->prefix_len;
    uint128_t val;

    /* Initialize mask with same family as addr */
    memset(mask, 0, sizeof(*mask));
    mask->addr.sa.sa_family = ipaddr_family(addr);
    mask->prefix_len = max_bits;
    mask->has_prefix = false;

    /* Compute mask value */
    if (prefix == 0) {
        val = 0;
    } else if (prefix >= max_bits) {
        val = (uint128_t)-1 >> (128 - max_bits);
    } else {
        /* Create mask with 'prefix' 1-bits followed by zeros */
        uint128_t all_ones = (uint128_t)-1 >> (128 - max_bits);
        val = all_ones ^ (all_ones >> prefix);
    }

    ipaddr_from_uint128(mask, val, mask);
}

/*
 * Compute the hostmask (inverse of netmask).
 */
void ipaddr_hostmask(const ipaddr_t *addr, ipaddr_t *mask)
{
    int max_bits = ipaddr_max_prefix(addr);
    int prefix = addr->prefix_len;
    uint128_t val;

    /* Initialize mask with same family as addr */
    memset(mask, 0, sizeof(*mask));
    mask->addr.sa.sa_family = ipaddr_family(addr);
    mask->prefix_len = max_bits;
    mask->has_prefix = false;

    /* Compute hostmask value (inverse of netmask) */
    if (prefix >= max_bits) {
        val = 0;
    } else {
        uint128_t all_ones = (uint128_t)-1 >> (128 - max_bits);
        val = all_ones >> prefix;
    }

    ipaddr_from_uint128(mask, val, mask);
}
