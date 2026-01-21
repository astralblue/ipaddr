/*
 * main.c - IP address manipulation command-line tool
 *
 * Usage: ipaddr [-M] ADDRESS [COMMAND [ARGS...]] ...
 */

#include "ipaddr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/*
 * Print usage information.
 */
static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [-M] ADDRESS [COMMAND [ARGS...]] ...\n"
        "\n"
        "Options:\n"
        "  -M        Output prefix as netmask (e.g., /255.255.255.0)\n"
        "\n"
        "Commands:\n"
        "  (none)           Print normalized address\n"
        "  version          Print IP version (4 or 6)\n"
        "  packed           Print address as hex bytes\n"
        "  to-int           Print address as decimal integer\n"
        "  prefix-length    Print prefix length\n"
        "  prefixlen        Alias for prefix-length\n"
        "  netmask          Print network mask\n"
        "  hostmask         Print host mask\n"
        "  address          Print address without prefix\n"
        "  network          Print network address\n"
        "  broadcast        Print broadcast address\n"
        "  num-addresses    Print number of addresses in network\n"
        "  host INDEX       Print host at index (negative from end)\n"
        "  host-index       Print index of address in network\n"
        "  subnet PLEN IDX  Print subnet (PLEN: prefix or +N relative)\n"
        "  super PLEN       Print supernet (PLEN: prefix or -N relative)\n"
        "  is-loopback      Exit 0 if loopback, 1 otherwise\n"
        "  is-private       Exit 0 if private, 1 otherwise\n"
        "  is-global        Exit 0 if global unicast, 1 otherwise\n"
        "  is-multicast     Exit 0 if multicast, 1 otherwise\n"
        "  is-link-local    Exit 0 if link-local, 1 otherwise\n"
        "  is-unspecified   Exit 0 if unspecified (0.0.0.0/::), 1 otherwise\n"
        "  is-reserved      Exit 0 if reserved, 1 otherwise\n"
        "  zone-id          Print IPv6 zone ID\n"
        "  scope-id         Print IPv6 numeric scope ID\n"
        "  ipv4             Extract IPv4 from IPv6 or pass through\n"
        "  6to4             Extract IPv4 from 6to4 address\n"
        "  teredo server    Extract Teredo server address\n"
        "  teredo client    Extract Teredo client address\n"
        "  in ADDR          Exit 0 if in network ADDR, 1 otherwise\n"
        "  contains ADDR    Exit 0 if contains network ADDR, 1 otherwise\n"
        "  overlaps ADDR    Exit 0 if overlaps network ADDR, 1 otherwise\n"
        "  eq ADDR          Exit 0 if equal to ADDR, 1 otherwise\n"
        "  ne ADDR          Exit 0 if not equal to ADDR, 1 otherwise\n"
        "  lt ADDR          Exit 0 if less than ADDR, 1 otherwise\n"
        "  le ADDR          Exit 0 if less than or equal to ADDR, 1 otherwise\n"
        "  gt ADDR          Exit 0 if greater than ADDR, 1 otherwise\n"
        "  ge ADDR          Exit 0 if greater than or equal to ADDR, 1 otherwise\n"
        "\n"
        "Commands can be chained; chainable commands update the current address.\n",
        prog);
}

/* Forward declarations for command handlers */
static int cmd_default(ipaddr_ctx_t *ctx);
static int cmd_version(ipaddr_ctx_t *ctx);
static int cmd_packed(ipaddr_ctx_t *ctx);
static int cmd_to_int(ipaddr_ctx_t *ctx);
static int cmd_prefix_length(ipaddr_ctx_t *ctx);
static int cmd_netmask(ipaddr_ctx_t *ctx);
static int cmd_hostmask(ipaddr_ctx_t *ctx);
static int cmd_address(ipaddr_ctx_t *ctx);
static int cmd_network(ipaddr_ctx_t *ctx);
static int cmd_broadcast(ipaddr_ctx_t *ctx);
static int cmd_num_addresses(ipaddr_ctx_t *ctx);
static int cmd_host(ipaddr_ctx_t *ctx);
static int cmd_host_index(ipaddr_ctx_t *ctx);
static int cmd_subnet(ipaddr_ctx_t *ctx);
static int cmd_super(ipaddr_ctx_t *ctx);
static int cmd_is_loopback(ipaddr_ctx_t *ctx);
static int cmd_is_private(ipaddr_ctx_t *ctx);
static int cmd_is_global(ipaddr_ctx_t *ctx);
static int cmd_is_multicast(ipaddr_ctx_t *ctx);
static int cmd_is_link_local(ipaddr_ctx_t *ctx);
static int cmd_is_unspecified(ipaddr_ctx_t *ctx);
static int cmd_is_reserved(ipaddr_ctx_t *ctx);
static int cmd_zone_id(ipaddr_ctx_t *ctx);
static int cmd_scope_id(ipaddr_ctx_t *ctx);
static int cmd_ipv4(ipaddr_ctx_t *ctx);
static int cmd_6to4(ipaddr_ctx_t *ctx);
static int cmd_teredo(ipaddr_ctx_t *ctx);
static int cmd_in(ipaddr_ctx_t *ctx);
static int cmd_contains(ipaddr_ctx_t *ctx);
static int cmd_overlaps(ipaddr_ctx_t *ctx);
static int cmd_eq(ipaddr_ctx_t *ctx);
static int cmd_ne(ipaddr_ctx_t *ctx);
static int cmd_lt(ipaddr_ctx_t *ctx);
static int cmd_le(ipaddr_ctx_t *ctx);
static int cmd_gt(ipaddr_ctx_t *ctx);
static int cmd_ge(ipaddr_ctx_t *ctx);

/*
 * Command table.
 */
static const cmd_t commands[] = {
    /* name           alias          min max chain prefix handler */
    { "version",      NULL,          0,  0,  false, false, cmd_version },
    { "packed",       NULL,          0,  0,  false, false, cmd_packed },
    { "to-int",       NULL,          0,  0,  false, false, cmd_to_int },
    { "prefix-length", "prefixlen",  0,  0,  false, false, cmd_prefix_length },
    { "netmask",      NULL,          0,  0,  false, false, cmd_netmask },
    { "hostmask",     NULL,          0,  0,  false, false, cmd_hostmask },
    { "address",      NULL,          0,  0,  true,  false, cmd_address },
    { "network",      NULL,          0,  0,  true,  false, cmd_network },
    { "broadcast",    NULL,          0,  0,  false, true,  cmd_broadcast },
    { "num-addresses", NULL,         0,  0,  false, false, cmd_num_addresses },
    { "host",         NULL,          1,  1,  true,  false, cmd_host },
    { "host-index",   NULL,          0,  0,  false, false, cmd_host_index },
    { "subnet",       NULL,          2,  2,  true,  true,  cmd_subnet },
    { "super",        NULL,          1,  1,  true,  true,  cmd_super },
    { "is-loopback",  NULL,          0,  0,  false, false, cmd_is_loopback },
    { "is-private",   NULL,          0,  0,  false, false, cmd_is_private },
    { "is-global",    NULL,          0,  0,  false, false, cmd_is_global },
    { "is-multicast", NULL,          0,  0,  false, false, cmd_is_multicast },
    { "is-link-local", NULL,         0,  0,  false, false, cmd_is_link_local },
    { "is-unspecified", NULL,        0,  0,  false, false, cmd_is_unspecified },
    { "is-reserved",  NULL,          0,  0,  false, false, cmd_is_reserved },
    { "zone-id",      NULL,          0,  0,  false, false, cmd_zone_id },
    { "scope-id",     NULL,          0,  0,  false, false, cmd_scope_id },
    { "ipv4",         NULL,          0,  0,  true,  false, cmd_ipv4 },
    { "6to4",         NULL,          0,  0,  true,  false, cmd_6to4 },
    { "teredo",       NULL,          1,  1,  true,  false, cmd_teredo },
    { "in",           NULL,          1,  1,  false, false, cmd_in },
    { "contains",     NULL,          1,  1,  false, false, cmd_contains },
    { "overlaps",     NULL,          1,  1,  false, false, cmd_overlaps },
    { "eq",           NULL,          1,  1,  false, false, cmd_eq },
    { "ne",           NULL,          1,  1,  false, false, cmd_ne },
    { "lt",           NULL,          1,  1,  false, false, cmd_lt },
    { "le",           NULL,          1,  1,  false, false, cmd_le },
    { "gt",           NULL,          1,  1,  false, false, cmd_gt },
    { "ge",           NULL,          1,  1,  false, false, cmd_ge },
    { NULL, NULL, 0, 0, false, false, NULL }
};

/*
 * Find a command by name or alias.
 */
static const cmd_t *find_command(const char *name)
{
    for (const cmd_t *cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(name, cmd->name) == 0)
            return cmd;
        if (cmd->alias != NULL && strcmp(name, cmd->alias) == 0)
            return cmd;
    }
    return NULL;
}

/*
 * Get next argument from context.
 */
static const char *next_arg(ipaddr_ctx_t *ctx)
{
    if (ctx->argc <= 0)
        return NULL;
    ctx->argc--;
    return *ctx->argv++;
}

/* ========== Command Handlers ========== */

static int cmd_default(ipaddr_ctx_t *ctx)
{
    char buf[IPADDR_MAX_ADDRSTRLEN + 33];
    int rc = ipaddr_format(&ctx->current, buf, sizeof(buf), ctx->netmask_mode);
    if (rc != IPADDR_OK)
        return rc;
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_version(ipaddr_ctx_t *ctx)
{
    printf("%d\n", ipaddr_is_ipv4(&ctx->current) ? 4 : 6);
    return IPADDR_OK;
}

static int cmd_packed(ipaddr_ctx_t *ctx)
{
    char buf[33];
    int rc = ipaddr_format_packed(&ctx->current, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_to_int(ipaddr_ctx_t *ctx)
{
    char buf[IPADDR_UINT128_STRLEN];
    uint128_t val = ipaddr_to_uint128(&ctx->current);
    uint128_to_str(val, buf, sizeof(buf));
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_prefix_length(ipaddr_ctx_t *ctx)
{
    printf("%d\n", ctx->current.prefix_len);
    return IPADDR_OK;
}

static int cmd_netmask(ipaddr_ctx_t *ctx)
{
    ipaddr_t mask;
    char buf[IPADDR_INET6_ADDRSTRLEN];

    ipaddr_netmask(&ctx->current, &mask);
    int rc = ipaddr_format_addr(&mask, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_hostmask(ipaddr_ctx_t *ctx)
{
    ipaddr_t mask;
    char buf[IPADDR_INET6_ADDRSTRLEN];

    ipaddr_hostmask(&ctx->current, &mask);
    int rc = ipaddr_format_addr(&mask, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_address(ipaddr_ctx_t *ctx)
{
    char buf[IPADDR_INET6_ADDRSTRLEN];
    int rc = ipaddr_format_addr(&ctx->current, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current to be address-only for chaining */
    ctx->current.has_prefix = false;
    ctx->current.prefix_len = ipaddr_max_prefix(&ctx->current);

    return IPADDR_OK;
}

static int cmd_network(ipaddr_ctx_t *ctx)
{
    ipaddr_t net;
    char buf[IPADDR_MAX_ADDRSTRLEN + 8];

    ipaddr_network(&ctx->current, &net);
    int rc = ipaddr_format(&net, buf, sizeof(buf), ctx->netmask_mode);
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining */
    ctx->current = net;

    return IPADDR_OK;
}

static int cmd_broadcast(ipaddr_ctx_t *ctx)
{
    ipaddr_t bcast;
    char buf[IPADDR_INET6_ADDRSTRLEN];

    ipaddr_broadcast(&ctx->current, &bcast);
    int rc = ipaddr_format_addr(&bcast, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_num_addresses(ipaddr_ctx_t *ctx)
{
    char buf[IPADDR_UINT128_STRLEN];
    uint128_t num = ipaddr_num_addresses(&ctx->current);
    uint128_to_str(num, buf, sizeof(buf));
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_host(ipaddr_ctx_t *ctx)
{
    const char *arg = next_arg(ctx);
    if (arg == NULL) {
        fprintf(stderr, "host: missing index argument\n");
        return IPADDR_ERR_USAGE;
    }

    char *endp;
    long long index = strtoll(arg, &endp, 10);
    if (*endp != '\0') {
        fprintf(stderr, "host: invalid index '%s'\n", arg);
        return IPADDR_ERR_USAGE;
    }

    ipaddr_t host;
    int rc = ipaddr_host(&ctx->current, (int128_t)index, &host);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "host: index out of range\n");
        return rc;
    }

    char buf[IPADDR_INET6_ADDRSTRLEN];
    rc = ipaddr_format_addr(&host, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining (as host address, no prefix) */
    ctx->current = host;

    return IPADDR_OK;
}

static int cmd_host_index(ipaddr_ctx_t *ctx)
{
    char buf[IPADDR_UINT128_STRLEN];
    uint128_t idx = ipaddr_host_index(&ctx->current);
    uint128_to_str(idx, buf, sizeof(buf));
    printf("%s\n", buf);
    return IPADDR_OK;
}

static int cmd_subnet(ipaddr_ctx_t *ctx)
{
    const char *plen_arg = next_arg(ctx);
    const char *idx_arg = next_arg(ctx);

    if (plen_arg == NULL || idx_arg == NULL) {
        fprintf(stderr, "subnet: requires PLEN and INDEX arguments\n");
        return IPADDR_ERR_USAGE;
    }

    /* Parse prefix length (absolute or +N relative) */
    int new_prefix;
    if (plen_arg[0] == '+') {
        char *endp;
        long rel = strtol(plen_arg + 1, &endp, 10);
        if (*endp != '\0') {
            fprintf(stderr, "subnet: invalid prefix '%s'\n", plen_arg);
            return IPADDR_ERR_USAGE;
        }
        new_prefix = ctx->current.prefix_len + (int)rel;
    } else {
        char *endp;
        long abs = strtol(plen_arg, &endp, 10);
        if (*endp != '\0') {
            fprintf(stderr, "subnet: invalid prefix '%s'\n", plen_arg);
            return IPADDR_ERR_USAGE;
        }
        new_prefix = (int)abs;
    }

    /* Parse index */
    char *endp;
    long long index = strtoll(idx_arg, &endp, 10);
    if (*endp != '\0') {
        fprintf(stderr, "subnet: invalid index '%s'\n", idx_arg);
        return IPADDR_ERR_USAGE;
    }

    /* Check if this is an interface address (has host bits set) */
    bool preserve_host = (ipaddr_host_index(&ctx->current) != 0);

    ipaddr_t subnet;
    int rc = ipaddr_subnet(&ctx->current, new_prefix, (int128_t)index,
                           preserve_host, &subnet);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "subnet: invalid subnet parameters\n");
        return rc;
    }

    char buf[IPADDR_MAX_ADDRSTRLEN + 8];
    rc = ipaddr_format(&subnet, buf, sizeof(buf), ctx->netmask_mode);
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining */
    ctx->current = subnet;

    return IPADDR_OK;
}

static int cmd_super(ipaddr_ctx_t *ctx)
{
    const char *plen_arg = next_arg(ctx);

    if (plen_arg == NULL) {
        fprintf(stderr, "super: requires PLEN argument\n");
        return IPADDR_ERR_USAGE;
    }

    /* Parse prefix length (absolute or -N relative) */
    int new_prefix;
    if (plen_arg[0] == '-') {
        char *endp;
        long rel = strtol(plen_arg + 1, &endp, 10);
        if (*endp != '\0') {
            fprintf(stderr, "super: invalid prefix '%s'\n", plen_arg);
            return IPADDR_ERR_USAGE;
        }
        new_prefix = ctx->current.prefix_len - (int)rel;
    } else {
        char *endp;
        long abs = strtol(plen_arg, &endp, 10);
        if (*endp != '\0') {
            fprintf(stderr, "super: invalid prefix '%s'\n", plen_arg);
            return IPADDR_ERR_USAGE;
        }
        new_prefix = (int)abs;
    }

    ipaddr_t super;
    int rc = ipaddr_super(&ctx->current, new_prefix, &super);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "super: prefix must be less than current (%d)\n",
                ctx->current.prefix_len);
        return rc;
    }

    char buf[IPADDR_MAX_ADDRSTRLEN + 8];
    rc = ipaddr_format(&super, buf, sizeof(buf), ctx->netmask_mode);
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining */
    ctx->current = super;

    return IPADDR_OK;
}

/* is-* commands return 0 for true, 1 for false */

static int cmd_is_loopback(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_loopback(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_is_private(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_private(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_is_global(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_global(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_is_multicast(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_multicast(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_is_link_local(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_link_local(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_is_unspecified(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_unspecified(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_is_reserved(ipaddr_ctx_t *ctx)
{
    return ipaddr_is_reserved(&ctx->current) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_zone_id(ipaddr_ctx_t *ctx)
{
    const char *zone = ipaddr_zone_id(&ctx->current);
    if (zone == NULL) {
        printf("\n");
    } else {
        printf("%s\n", zone);
    }
    return IPADDR_OK;
}

static int cmd_scope_id(ipaddr_ctx_t *ctx)
{
    printf("%u\n", ipaddr_scope_id(&ctx->current));
    return IPADDR_OK;
}

static int cmd_ipv4(ipaddr_ctx_t *ctx)
{
    ipaddr_t v4;
    int rc = ipaddr_to_ipv4(&ctx->current, &v4);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "ipv4: cannot extract IPv4 address\n");
        return rc;
    }

    char buf[IPADDR_INET_ADDRSTRLEN];
    rc = ipaddr_format_addr(&v4, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining */
    ctx->current = v4;

    return IPADDR_OK;
}

static int cmd_6to4(ipaddr_ctx_t *ctx)
{
    ipaddr_t v4;
    int rc = ipaddr_6to4(&ctx->current, &v4);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "6to4: not a 6to4 address\n");
        return rc;
    }

    char buf[IPADDR_INET_ADDRSTRLEN];
    rc = ipaddr_format_addr(&v4, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining */
    ctx->current = v4;

    return IPADDR_OK;
}

static int cmd_teredo(ipaddr_ctx_t *ctx)
{
    const char *mode_arg = next_arg(ctx);
    if (mode_arg == NULL) {
        fprintf(stderr, "teredo: requires 'server' or 'client' argument\n");
        return IPADDR_ERR_USAGE;
    }

    int mode;
    if (strcmp(mode_arg, "server") == 0) {
        mode = 0;
    } else if (strcmp(mode_arg, "client") == 0) {
        mode = 1;
    } else {
        fprintf(stderr, "teredo: invalid mode '%s' (use 'server' or 'client')\n",
                mode_arg);
        return IPADDR_ERR_USAGE;
    }

    ipaddr_t result;
    int rc = ipaddr_teredo(&ctx->current, mode, &result);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "teredo: not a Teredo address\n");
        return rc;
    }

    char buf[IPADDR_INET_ADDRSTRLEN];
    rc = ipaddr_format_addr(&result, buf, sizeof(buf));
    if (rc != IPADDR_OK)
        return rc;
    if (!ctx->silent)
        printf("%s\n", buf);

    /* Update current for chaining */
    ctx->current = result;

    return IPADDR_OK;
}

/* Helper for parsing a second address argument */
static int parse_second_addr(ipaddr_ctx_t *ctx, ipaddr_t *other)
{
    const char *arg = next_arg(ctx);
    if (arg == NULL) {
        fprintf(stderr, "missing address argument\n");
        return IPADDR_ERR_USAGE;
    }

    const char *errmsg;
    int rc = ipaddr_parse(arg, other, &errmsg);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "invalid address '%s': %s\n", arg, errmsg);
        return rc;
    }

    return IPADDR_OK;
}

static int cmd_in(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return ipaddr_in(&ctx->current, &other) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_contains(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return ipaddr_contains(&ctx->current, &other) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_overlaps(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return ipaddr_overlaps(&ctx->current, &other) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_eq(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return (ipaddr_cmp(&ctx->current, &other) == 0) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_ne(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return (ipaddr_cmp(&ctx->current, &other) != 0) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_lt(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return (ipaddr_cmp(&ctx->current, &other) < 0) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_le(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return (ipaddr_cmp(&ctx->current, &other) <= 0) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_gt(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return (ipaddr_cmp(&ctx->current, &other) > 0) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

static int cmd_ge(ipaddr_ctx_t *ctx)
{
    ipaddr_t other;
    int rc = parse_second_addr(ctx, &other);
    if (rc != IPADDR_OK)
        return rc;
    return (ipaddr_cmp(&ctx->current, &other) >= 0) ? IPADDR_OK : IPADDR_ERR_BOOL;
}

/*
 * Main entry point.
 */
int main(int argc, char **argv)
{
    ipaddr_ctx_t ctx = { 0 };
    int opt;
    int rc;

    /* Parse options ('+' forces POSIX behavior: stop at first non-option) */
    while ((opt = getopt(argc, argv, "+Mh")) != -1) {
        switch (opt) {
        case 'M':
            ctx.netmask_mode = true;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return IPADDR_ERR_USAGE;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
        fprintf(stderr, "Error: address required\n");
        usage(argv[0] ? argv[0] : "ipaddr");
        return IPADDR_ERR_USAGE;
    }

    /* Parse initial address */
    const char *errmsg;
    rc = ipaddr_parse(argv[0], &ctx.current, &errmsg);
    if (rc != IPADDR_OK) {
        fprintf(stderr, "Error: %s: %s\n", argv[0], errmsg);
        return rc;
    }

    /* Set up remaining args for command processing */
    ctx.argc = argc - 1;
    ctx.argv = argv + 1;

    /* If no commands, just print normalized address */
    if (ctx.argc == 0) {
        return cmd_default(&ctx);
    }

    /* Process commands */
    while (ctx.argc > 0) {
        const char *cmd_name = next_arg(&ctx);
        const cmd_t *cmd = find_command(cmd_name);

        if (cmd == NULL) {
            fprintf(stderr, "Error: unknown command '%s'\n", cmd_name);
            return IPADDR_ERR_USAGE;
        }

        /* Check for required prefix */
        if (cmd->needs_prefix && !ctx.current.has_prefix) {
            fprintf(stderr, "Error: %s requires an address with prefix (e.g., /24)\n",
                    cmd_name);
            return IPADDR_ERR_USAGE;
        }

        /* Check argument count */
        if (ctx.argc < cmd->min_args) {
            fprintf(stderr, "Error: %s requires %d argument(s)\n",
                    cmd_name, cmd->min_args);
            return IPADDR_ERR_USAGE;
        }

        /*
         * For chainable commands, suppress output if there are more commands
         * after this one's arguments. We peek ahead to check.
         */
        if (cmd->chainable) {
            int args_remaining = ctx.argc - cmd->min_args;
            ctx.silent = (args_remaining > 0);
        } else {
            ctx.silent = false;
        }

        /* Execute command */
        rc = cmd->handler(&ctx);
        if (rc != IPADDR_OK) {
            return rc;
        }
    }

    return IPADDR_OK;
}
