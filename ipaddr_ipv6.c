/*
 * ipaddr_ipv6.c - IPv6-specific operations
 */

#include "ipaddr.h"

#include <string.h>
#include <stdio.h>
#include <net/if.h>

/*
 * Get the zone ID string from an IPv6 address.
 * Returns NULL if no zone ID is present.
 *
 * Note: Zone ID is typically stored as scope_id numeric value.
 * This function converts it back to interface name if possible.
 */
const char *ipaddr_zone_id(const ipaddr_t *addr)
{
    static char zone_buf[IF_NAMESIZE];

    if (!ipaddr_is_ipv6(addr))
        return NULL;

    uint32_t scope = addr->addr.sin6.sin6_scope_id;
    if (scope == 0)
        return NULL;

    /* Try to convert to interface name */
    if (if_indextoname(scope, zone_buf) != NULL)
        return zone_buf;

    /* Fallback to numeric string */
    snprintf(zone_buf, sizeof(zone_buf), "%u", scope);
    return zone_buf;
}

/*
 * Get the numeric scope ID from an IPv6 address.
 */
uint32_t ipaddr_scope_id(const ipaddr_t *addr)
{
    if (!ipaddr_is_ipv6(addr))
        return 0;
    return addr->addr.sin6.sin6_scope_id;
}

/*
 * Extract IPv4 address from IPv6 (mapped, compatible, or any).
 */
int ipaddr_to_ipv4(const ipaddr_t *addr, ipaddr_t *v4)
{
    memset(v4, 0, sizeof(*v4));
    v4->addr.sa.sa_family = AF_INET;
    v4->prefix_len = 32;
    v4->has_prefix = false;

    if (ipaddr_is_ipv4(addr)) {
        /* Already IPv4, just copy */
        memcpy(&v4->addr.sin.sin_addr, &addr->addr.sin.sin_addr, 4);
        v4->has_prefix = addr->has_prefix;
        if (addr->has_prefix && addr->prefix_len <= 32)
            v4->prefix_len = addr->prefix_len;
        return IPADDR_OK;
    }

    /* IPv6 - extract last 32 bits */
    const uint8_t *bytes = (const uint8_t *)&addr->addr.sin6.sin6_addr;
    memcpy(&v4->addr.sin.sin_addr, bytes + 12, 4);

    return IPADDR_OK;
}

/*
 * Check if an address is a 6to4 address (2002::/16).
 */
static bool is_6to4(const ipaddr_t *addr)
{
    if (!ipaddr_is_ipv6(addr))
        return false;

    const uint8_t *bytes = (const uint8_t *)&addr->addr.sin6.sin6_addr;
    return bytes[0] == 0x20 && bytes[1] == 0x02;
}

/*
 * Extract the embedded IPv4 address from a 6to4 address.
 */
int ipaddr_6to4(const ipaddr_t *addr, ipaddr_t *v4)
{
    if (!is_6to4(addr))
        return IPADDR_ERR_USAGE;

    memset(v4, 0, sizeof(*v4));
    v4->addr.sa.sa_family = AF_INET;
    v4->prefix_len = 32;
    v4->has_prefix = false;

    /* 6to4 format: 2002:XXXX:XXXX::/48 where XXXX:XXXX is the IPv4 address */
    const uint8_t *bytes = (const uint8_t *)&addr->addr.sin6.sin6_addr;
    memcpy(&v4->addr.sin.sin_addr, bytes + 2, 4);

    return IPADDR_OK;
}

/*
 * Check if an address is a Teredo address (2001:0000::/32).
 */
static bool is_teredo(const ipaddr_t *addr)
{
    if (!ipaddr_is_ipv6(addr))
        return false;

    const uint8_t *bytes = (const uint8_t *)&addr->addr.sin6.sin6_addr;
    return bytes[0] == 0x20 && bytes[1] == 0x01 &&
           bytes[2] == 0x00 && bytes[3] == 0x00;
}

/*
 * Extract Teredo information.
 * mode: 0 = server address, 1 = client address
 *
 * Teredo format: 2001:0000:SSSS:SSSS:XXXX:CCCC:CCCC:CCCC
 *   SSSS:SSSS = Server IPv4 address
 *   CCCC:CCCC:CCCC = Obfuscated client IPv4 (XOR with 0xFFFFFFFF)
 *   XXXX = flags and obfuscated port (not used here)
 */
int ipaddr_teredo(const ipaddr_t *addr, int mode, ipaddr_t *result)
{
    if (!is_teredo(addr))
        return IPADDR_ERR_USAGE;

    memset(result, 0, sizeof(*result));
    result->addr.sa.sa_family = AF_INET;
    result->prefix_len = 32;
    result->has_prefix = false;

    const uint8_t *bytes = (const uint8_t *)&addr->addr.sin6.sin6_addr;
    uint8_t *out = (uint8_t *)&result->addr.sin.sin_addr;

    if (mode == 0) {
        /* Server address: bytes 4-7 */
        memcpy(out, bytes + 4, 4);
    } else {
        /* Client address: bytes 12-15, XOR'd with 0xFF each */
        out[0] = bytes[12] ^ 0xff;
        out[1] = bytes[13] ^ 0xff;
        out[2] = bytes[14] ^ 0xff;
        out[3] = bytes[15] ^ 0xff;
    }

    return IPADDR_OK;
}
