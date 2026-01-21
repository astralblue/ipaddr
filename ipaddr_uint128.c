/*
 * ipaddr_uint128.c - 128-bit integer operations
 */

#include "ipaddr.h"

#include <string.h>
#include <ctype.h>

/*
 * Convert an IP address to a 128-bit unsigned integer.
 * Network byte order (big-endian) address bytes are converted to native integer.
 */
uint128_t ipaddr_to_uint128(const ipaddr_t *addr)
{
    const uint8_t *bytes = (const uint8_t *)ipaddr_bytes(addr);
    size_t len = ipaddr_bytes_len(addr);
    uint128_t result = 0;

    for (size_t i = 0; i < len; i++) {
        result = (result << 8) | bytes[i];
    }

    return result;
}

/*
 * Set an IP address from a 128-bit unsigned integer.
 * Preserves the address family and prefix info from the template.
 */
void ipaddr_from_uint128(ipaddr_t *addr, uint128_t val, const ipaddr_t *tmpl)
{
    uint8_t *bytes;
    size_t len;

    /* Copy template to get family and prefix */
    memcpy(addr, tmpl, sizeof(*addr));

    /* Get writable pointer to address bytes */
    if (ipaddr_is_ipv4(addr)) {
        bytes = (uint8_t *)&addr->addr.sin.sin_addr;
        len = 4;
    } else {
        bytes = (uint8_t *)&addr->addr.sin6.sin6_addr;
        len = 16;
    }

    /* Convert integer to network byte order */
    for (size_t i = 0; i < len; i++) {
        bytes[len - 1 - i] = (uint8_t)(val & 0xff);
        val >>= 8;
    }
}

/*
 * Convert a 128-bit unsigned integer to a decimal string.
 * Uses repeated division by 10, collecting remainders.
 */
void uint128_to_str(uint128_t val, char *buf, size_t buflen)
{
    char tmp[IPADDR_UINT128_STRLEN];
    int pos = sizeof(tmp) - 1;

    tmp[pos] = '\0';

    if (val == 0) {
        tmp[--pos] = '0';
    } else {
        while (val > 0) {
            tmp[--pos] = '0' + (val % 10);
            val /= 10;
        }
    }

    size_t len = sizeof(tmp) - 1 - pos;
    if (len >= buflen)
        len = buflen - 1;
    memcpy(buf, tmp + pos, len);
    buf[len] = '\0';
}

/*
 * Parse a decimal string to a 128-bit unsigned integer.
 */
int str_to_uint128(const char *str, uint128_t *val)
{
    uint128_t result = 0;
    uint128_t max_div_10 = ((uint128_t)-1) / 10;
    uint128_t max_mod_10 = ((uint128_t)-1) % 10;

    if (str == NULL || *str == '\0')
        return IPADDR_ERR_USAGE;

    /* Skip leading whitespace */
    while (isspace((unsigned char)*str))
        str++;

    if (*str == '\0')
        return IPADDR_ERR_USAGE;

    while (*str) {
        if (!isdigit((unsigned char)*str))
            return IPADDR_ERR_USAGE;

        int digit = *str - '0';

        /* Check for overflow */
        if (result > max_div_10 ||
            (result == max_div_10 && (uint128_t)digit > max_mod_10))
            return IPADDR_ERR_USAGE;

        result = result * 10 + digit;
        str++;
    }

    *val = result;
    return IPADDR_OK;
}
