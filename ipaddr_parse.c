/*
 * ipaddr_parse.c - IP address parsing functions
 */

#include "ipaddr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <arpa/inet.h>

/*
 * Valid netmask byte values (must have contiguous 1-bits).
 */
static const uint8_t valid_mask_bytes[] = {
    0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00
};

/*
 * Count leading 1-bits in a byte.
 */
static int count_leading_ones(uint8_t b)
{
    for (int i = 0; i < 9; i++) {
        if (valid_mask_bytes[i] == b)
            return 8 - i;
    }
    return -1; /* invalid mask byte */
}

/*
 * Validate that a netmask has contiguous 1-bits.
 * Returns prefix length on success, -1 on invalid mask.
 */
int ipaddr_validate_netmask(const ipaddr_t *mask)
{
    const uint8_t *bytes = (const uint8_t *)ipaddr_bytes(mask);
    size_t len = ipaddr_bytes_len(mask);
    int prefix = 0;
    bool seen_non_ff = false;

    for (size_t i = 0; i < len; i++) {
        uint8_t b = bytes[i];
        int ones = count_leading_ones(b);

        if (ones < 0)
            return -1; /* invalid byte value */

        if (seen_non_ff) {
            /* After a non-ff byte, all subsequent must be 00 */
            if (b != 0x00)
                return -1;
        } else {
            prefix += ones;
            if (b != 0xff)
                seen_non_ff = true;
        }
    }

    return prefix;
}

/*
 * Try to parse a string as a netmask and convert to prefix length.
 * Returns prefix length on success, -1 if not a valid netmask.
 */
static int parse_netmask_prefix(const char *str, int family)
{
    struct addrinfo hints, *res;
    ipaddr_t mask;
    int plen;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_flags = AI_NUMERICHOST;

    if (getaddrinfo(str, NULL, &hints, &res) != 0)
        return -1;

    memset(&mask, 0, sizeof(mask));
    if (family == AF_INET) {
        memcpy(&mask.addr.sin, res->ai_addr, sizeof(struct sockaddr_in));
    } else {
        memcpy(&mask.addr.sin6, res->ai_addr, sizeof(struct sockaddr_in6));
    }
    freeaddrinfo(res);

    plen = ipaddr_validate_netmask(&mask);
    return plen;
}

/*
 * Parse an IP address string with optional prefix.
 */
int ipaddr_parse(const char *str, ipaddr_t *addr, const char **errmsg)
{
    char buf[IPADDR_MAX_ADDRSTRLEN + 33]; /* room for /prefix */
    char *slash;
    char *prefix_str = NULL;
    struct addrinfo hints, *res;
    int rc;

    memset(addr, 0, sizeof(*addr));
    *errmsg = NULL;

    if (str == NULL || *str == '\0') {
        *errmsg = "empty address string";
        return IPADDR_ERR_USAGE;
    }

    /* Copy to buffer for modification */
    if (strlen(str) >= sizeof(buf)) {
        *errmsg = "address string too long";
        return IPADDR_ERR_USAGE;
    }
    strcpy(buf, str);

    /* Find prefix separator */
    slash = strchr(buf, '/');
    if (slash != NULL) {
        *slash = '\0';
        prefix_str = slash + 1;
        addr->has_prefix = true;
    }

    /* Parse address using getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST;

    rc = getaddrinfo(buf, NULL, &hints, &res);
    if (rc != 0) {
        *errmsg = "invalid IP address";
        return IPADDR_ERR_USAGE;
    }

    /* Copy parsed address */
    if (res->ai_family == AF_INET) {
        memcpy(&addr->addr.sin, res->ai_addr, sizeof(struct sockaddr_in));
        addr->prefix_len = 32; /* default for IPv4 */
    } else if (res->ai_family == AF_INET6) {
        memcpy(&addr->addr.sin6, res->ai_addr, sizeof(struct sockaddr_in6));
        addr->prefix_len = 128; /* default for IPv6 */
    } else {
        freeaddrinfo(res);
        *errmsg = "unsupported address family";
        return IPADDR_ERR_USAGE;
    }
    freeaddrinfo(res);

    /* Parse prefix if present */
    if (prefix_str != NULL) {
        int max_prefix = ipaddr_max_prefix(addr);
        char *endp;
        long plen;

        /* First, try as a decimal number */
        plen = strtol(prefix_str, &endp, 10);
        if (*endp == '\0' && endp != prefix_str) {
            /* Valid decimal prefix */
            if (plen < 0 || plen > max_prefix) {
                *errmsg = "prefix length out of range";
                return IPADDR_ERR_USAGE;
            }
            addr->prefix_len = (int)plen;
        } else {
            /* Try as a netmask */
            int mask_plen = parse_netmask_prefix(prefix_str, ipaddr_family(addr));
            if (mask_plen < 0) {
                *errmsg = "invalid prefix length or netmask";
                return IPADDR_ERR_USAGE;
            }
            addr->prefix_len = mask_plen;
        }
    }

    return IPADDR_OK;
}
