/*
 * ipaddr_network.c - Network operations
 */

#include "ipaddr.h"

#include <string.h>

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
 * Compute hostmask value for a given prefix length and bit width.
 */
static uint128_t compute_hostmask(int prefix, int max_bits)
{
    if (prefix >= max_bits) {
        return 0;
    } else {
        uint128_t all_ones = (uint128_t)-1 >> (128 - max_bits);
        return all_ones >> prefix;
    }
}

/*
 * Compute the network address (zeroed host bits).
 */
void ipaddr_network(const ipaddr_t *addr, ipaddr_t *net)
{
    int max_bits = ipaddr_max_prefix(addr);
    uint128_t val = ipaddr_to_uint128(addr);
    uint128_t mask = compute_netmask(addr->prefix_len, max_bits);

    memcpy(net, addr, sizeof(*net));
    net->has_prefix = true;
    ipaddr_from_uint128(net, val & mask, net);
}

/*
 * Compute the broadcast address (all host bits set to 1).
 */
void ipaddr_broadcast(const ipaddr_t *addr, ipaddr_t *bcast)
{
    int max_bits = ipaddr_max_prefix(addr);
    uint128_t val = ipaddr_to_uint128(addr);
    uint128_t hostmask = compute_hostmask(addr->prefix_len, max_bits);

    memcpy(bcast, addr, sizeof(*bcast));
    bcast->has_prefix = false;
    bcast->prefix_len = max_bits;
    ipaddr_from_uint128(bcast, val | hostmask, bcast);
}

/*
 * Get the host at a given index within the network.
 */
int ipaddr_host(const ipaddr_t *net, int128_t index, ipaddr_t *host)
{
    int max_bits = ipaddr_max_prefix(net);
    uint128_t net_val = ipaddr_to_uint128(net);
    uint128_t netmask = compute_netmask(net->prefix_len, max_bits);
    uint128_t hostmask = compute_hostmask(net->prefix_len, max_bits);
    uint128_t num_hosts = hostmask + 1;
    uint128_t host_offset;

    /* Handle negative indices */
    if (index < 0) {
        /* Convert negative index to positive offset from end */
        uint128_t abs_index = (uint128_t)(-index);
        if (abs_index > num_hosts)
            return IPADDR_ERR_USAGE;
        host_offset = num_hosts - abs_index;
    } else {
        host_offset = (uint128_t)index;
        if (host_offset >= num_hosts)
            return IPADDR_ERR_USAGE;
    }

    memcpy(host, net, sizeof(*host));
    host->has_prefix = false;
    host->prefix_len = max_bits;
    ipaddr_from_uint128(host, (net_val & netmask) | host_offset, host);

    return IPADDR_OK;
}

/*
 * Get the index of the current address within its network.
 */
uint128_t ipaddr_host_index(const ipaddr_t *addr)
{
    int max_bits = ipaddr_max_prefix(addr);
    uint128_t val = ipaddr_to_uint128(addr);
    uint128_t hostmask = compute_hostmask(addr->prefix_len, max_bits);

    return val & hostmask;
}

/*
 * Get the number of addresses in the network.
 */
uint128_t ipaddr_num_addresses(const ipaddr_t *addr)
{
    int max_bits = ipaddr_max_prefix(addr);
    int host_bits = max_bits - addr->prefix_len;

    if (host_bits <= 0)
        return 1;
    if (host_bits >= 128)
        return (uint128_t)-1; /* Maximum possible */

    return (uint128_t)1 << host_bits;
}

/*
 * Get a subnet of the current network.
 */
int ipaddr_subnet(const ipaddr_t *addr, int new_prefix, int128_t index,
                  bool preserve_host, ipaddr_t *subnet)
{
    int max_bits = ipaddr_max_prefix(addr);

    /* Validate new prefix */
    if (new_prefix < addr->prefix_len || new_prefix > max_bits)
        return IPADDR_ERR_USAGE;

    int subnet_bits = new_prefix - addr->prefix_len;
    uint128_t num_subnets;

    if (subnet_bits == 0) {
        num_subnets = 1;
    } else if (subnet_bits >= 128) {
        num_subnets = (uint128_t)-1;
    } else {
        num_subnets = (uint128_t)1 << subnet_bits;
    }

    /* Handle negative indices */
    uint128_t subnet_index;
    if (index < 0) {
        uint128_t abs_index = (uint128_t)(-index);
        if (abs_index > num_subnets)
            return IPADDR_ERR_USAGE;
        subnet_index = num_subnets - abs_index;
    } else {
        subnet_index = (uint128_t)index;
        if (subnet_index >= num_subnets)
            return IPADDR_ERR_USAGE;
    }

    /* Compute subnet address */
    uint128_t addr_val = ipaddr_to_uint128(addr);
    uint128_t old_netmask = compute_netmask(addr->prefix_len, max_bits);
    uint128_t new_hostmask = compute_hostmask(new_prefix, max_bits);

    /* Base network address */
    uint128_t base_net = addr_val & old_netmask;

    /* Subnet offset in address space */
    int host_bits = max_bits - new_prefix;
    uint128_t subnet_offset;
    if (host_bits >= 128) {
        subnet_offset = 0;
    } else {
        subnet_offset = subnet_index << host_bits;
    }

    /* New subnet network address */
    uint128_t subnet_net = base_net + subnet_offset;

    /* Preserve host bits if requested (interface address mode) */
    uint128_t result;
    if (preserve_host) {
        uint128_t host_bits_val = addr_val & new_hostmask;
        result = subnet_net | host_bits_val;
    } else {
        result = subnet_net;
    }

    memcpy(subnet, addr, sizeof(*subnet));
    subnet->prefix_len = new_prefix;
    subnet->has_prefix = true;
    ipaddr_from_uint128(subnet, result, subnet);

    return IPADDR_OK;
}

/*
 * Get the supernet of the current network.
 */
int ipaddr_super(const ipaddr_t *addr, int new_prefix, ipaddr_t *super)
{
    int max_bits = ipaddr_max_prefix(addr);

    /* Validate new prefix (must be less than current) */
    if (new_prefix < 0 || new_prefix > addr->prefix_len)
        return IPADDR_ERR_USAGE;

    uint128_t addr_val = ipaddr_to_uint128(addr);
    uint128_t new_netmask = compute_netmask(new_prefix, max_bits);

    memcpy(super, addr, sizeof(*super));
    super->prefix_len = new_prefix;
    super->has_prefix = true;
    ipaddr_from_uint128(super, addr_val & new_netmask, super);

    return IPADDR_OK;
}
