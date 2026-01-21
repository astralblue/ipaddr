/*
 * ipaddr_format.c - IP address formatting functions
 */

#include "ipaddr.h"

#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>

/*
 * Format just the address portion (no prefix) to a string buffer.
 */
int ipaddr_format_addr(const ipaddr_t *addr, char *buf, size_t buflen)
{
    socklen_t salen;
    int rc;

    if (ipaddr_is_ipv4(addr)) {
        salen = sizeof(struct sockaddr_in);
    } else {
        salen = sizeof(struct sockaddr_in6);
    }

    rc = getnameinfo(&addr->addr.sa, salen, buf, buflen, NULL, 0, NI_NUMERICHOST);
    if (rc != 0)
        return IPADDR_ERR_INTERNAL;

    return IPADDR_OK;
}

/*
 * Format netmask to string buffer.
 */
static int format_netmask(const ipaddr_t *addr, char *buf, size_t buflen)
{
    ipaddr_t mask;
    ipaddr_netmask(addr, &mask);
    return ipaddr_format_addr(&mask, buf, buflen);
}

/*
 * Format an IP address to a string buffer.
 */
int ipaddr_format(const ipaddr_t *addr, char *buf, size_t buflen, bool netmask_mode)
{
    int rc;
    size_t len;

    rc = ipaddr_format_addr(addr, buf, buflen);
    if (rc != IPADDR_OK)
        return rc;

    /* Append prefix if present */
    if (addr->has_prefix) {
        len = strlen(buf);
        if (netmask_mode) {
            char maskbuf[IPADDR_INET6_ADDRSTRLEN];
            rc = format_netmask(addr, maskbuf, sizeof(maskbuf));
            if (rc != IPADDR_OK)
                return rc;
            snprintf(buf + len, buflen - len, "/%s", maskbuf);
        } else {
            snprintf(buf + len, buflen - len, "/%d", addr->prefix_len);
        }
    }

    return IPADDR_OK;
}

/*
 * Format the address as a packed hex string (no colons/dots).
 */
int ipaddr_format_packed(const ipaddr_t *addr, char *buf, size_t buflen)
{
    const uint8_t *bytes = (const uint8_t *)ipaddr_bytes(addr);
    size_t len = ipaddr_bytes_len(addr);
    size_t needed = len * 2 + 1;

    if (buflen < needed)
        return IPADDR_ERR_INTERNAL;

    for (size_t i = 0; i < len; i++) {
        sprintf(buf + i * 2, "%02x", bytes[i]);
    }
    buf[len * 2] = '\0';

    return IPADDR_OK;
}
