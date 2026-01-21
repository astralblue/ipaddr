#!/bin/sh
#
# test.sh - CLI test suite for ipaddr
#
# Usage: ./test.sh [path/to/ipaddr]
#

set -e

IPADDR="${IPADDR:-./ipaddr}"
PASS=0
FAIL=0

# Test output equality
t() {
    expected="$1"; shift
    actual=$("$IPADDR" "$@" 2>&1) || true
    if [ "$expected" = "$actual" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        echo "FAIL: $IPADDR $*"
        echo "  Expected: '$expected'"
        echo "  Got:      '$actual'"
    fi
}

# Test exit code
te() {
    expected_exit="$1"; shift
    set +e
    "$IPADDR" "$@" >/dev/null 2>&1
    actual_exit=$?
    set -e
    if [ "$expected_exit" = "$actual_exit" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        echo "FAIL: $IPADDR $* (exit code)"
        echo "  Expected exit: $expected_exit"
        echo "  Got exit:      $actual_exit"
    fi
}

echo "=== Default (Normalization) Tests ==="

# IPv4 normalization
t "192.168.1.1" 192.168.001.001
t "192.168.1.0/24" 192.168.1.0/24
t "192.168.1.0/24" "192.168.1.0/255.255.255.0"
t "10.0.0.0/8" 10.0.0.0/8

# IPv6 normalization
t "2001:db8::1" 2001:0db8:0000:0000:0000:0000:0000:0001
t "::1" 0:0:0:0:0:0:0:1
t "fe80::" fe80:0000::
t "2001:db8::/32" 2001:db8::/32

# -M flag (netmask output)
t "192.168.1.0/255.255.255.0" -M 192.168.1.0/24
t "10.0.0.0/255.0.0.0" -M 10.0.0.0/8

echo "=== version Tests ==="

t "4" 192.168.1.1 version
t "4" 10.0.0.0/8 version
t "6" ::1 version
t "6" 2001:db8::/32 version
t "6" fe80::1%lo version

echo "=== packed Tests ==="

t "7f000001" 127.0.0.1 packed
t "c0a80101" 192.168.1.1 packed
t "0a000000" 10.0.0.0 packed
t "00000000000000000000000000000001" ::1 packed
t "20010db8000000000000000000000001" 2001:db8::1 packed
t "00000000000000000000ffff7f000001" ::ffff:127.0.0.1 packed

echo "=== to-int Tests ==="

t "2130706433" 127.0.0.1 to-int
t "3232235777" 192.168.1.1 to-int
t "167772160" 10.0.0.0 to-int
t "1" ::1 to-int
t "0" :: to-int
t "42540766411282592856903984951653826561" 2001:db8::1 to-int

echo "=== prefix-length / prefixlen Tests ==="

t "24" 192.168.1.0/24 prefix-length
t "24" 192.168.1.0/24 prefixlen
t "32" 192.168.1.1 prefix-length
t "8" 10.0.0.0/8 prefix-length
t "64" 2001:db8::/64 prefix-length
t "128" 2001:db8::1 prefix-length
t "0" 0.0.0.0/0 prefix-length

echo "=== netmask Tests ==="

t "255.255.255.0" 192.168.1.0/24 netmask
t "255.0.0.0" 10.0.0.0/8 netmask
t "255.255.255.255" 192.168.1.1 netmask
t "255.255.255.240" 192.168.1.0/28 netmask
t "0.0.0.0" 0.0.0.0/0 netmask
t "ffff:ffff:ffff:ffff::" 2001:db8::/64 netmask
t "ffff:ffff::" 2001:db8::/32 netmask

echo "=== hostmask Tests ==="

t "0.0.0.255" 192.168.1.0/24 hostmask
t "0.255.255.255" 10.0.0.0/8 hostmask
t "0.0.0.0" 192.168.1.1 hostmask
t "0.0.0.15" 192.168.1.0/28 hostmask
t "::ffff:ffff:ffff:ffff" 2001:db8::/64 hostmask

echo "=== address Tests ==="

t "192.168.1.30" 192.168.1.30/28 address
t "192.168.1.0" 192.168.1.0/24 address
t "192.168.1.1" 192.168.1.1 address
t "2001:db8::1" 2001:db8::1/64 address

echo "=== network Tests ==="

t "192.168.1.16/28" 192.168.1.30/28 network
t "192.168.1.0/24" 192.168.1.0/24 network
t "192.168.1.0/24" 192.168.1.255/24 network
t "192.168.1.1/32" 192.168.1.1 network
t "10.0.0.0/8" 10.255.255.255/8 network
t "2001:db8::/64" 2001:db8::ffff/64 network

echo "=== broadcast Tests ==="

t "192.168.1.31" 192.168.1.16/28 broadcast
t "192.168.1.255" 192.168.1.0/24 broadcast
t "10.255.255.255" 10.0.0.0/8 broadcast
t "2001:db8::ffff:ffff:ffff:ffff" 2001:db8::/64 broadcast
te 2 192.168.1.1 broadcast

echo "=== is-* Classification Tests ==="

# is-loopback
te 0 127.0.0.1 is-loopback
te 0 127.255.255.255 is-loopback
te 1 128.0.0.1 is-loopback
te 0 ::1 is-loopback
te 1 ::2 is-loopback

# is-private
te 0 10.0.0.1 is-private
te 0 172.16.0.1 is-private
te 0 172.31.255.255 is-private
te 1 172.32.0.1 is-private
te 0 192.168.1.1 is-private
te 1 8.8.8.8 is-private
te 0 fc00::1 is-private
te 0 fd12:3456::1 is-private

# is-global
te 0 8.8.8.8 is-global
te 0 1.1.1.1 is-global
te 1 10.0.0.1 is-global
te 1 192.168.1.1 is-global
te 0 2001:db8::1 is-global

# is-multicast
te 0 224.0.0.1 is-multicast
te 0 239.255.255.255 is-multicast
te 1 240.0.0.1 is-multicast
te 0 ff02::1 is-multicast
te 1 fe80::1 is-multicast

# is-link-local
te 0 169.254.1.1 is-link-local
te 1 169.255.1.1 is-link-local
te 0 fe80::1 is-link-local
te 0 fe81::1 is-link-local
te 1 fec0::1 is-link-local

# is-unspecified
te 0 0.0.0.0 is-unspecified
te 1 0.0.0.1 is-unspecified
te 0 :: is-unspecified
te 1 ::1 is-unspecified

# is-reserved
te 0 240.0.0.1 is-reserved
te 0 255.255.255.255 is-reserved
te 1 239.255.255.255 is-reserved

echo "=== num-addresses Tests ==="

t "256" 192.168.1.0/24 num-addresses
t "16" 192.168.1.0/28 num-addresses
t "1" 192.168.1.1 num-addresses
t "16777216" 10.0.0.0/8 num-addresses
t "4294967296" 0.0.0.0/0 num-addresses
t "18446744073709551616" 2001:db8::/64 num-addresses

echo "=== host Tests ==="

t "192.168.1.16" 192.168.1.16/28 host 0
t "192.168.1.17" 192.168.1.16/28 host 1
t "192.168.1.31" 192.168.1.16/28 host -1
t "192.168.1.30" 192.168.1.16/28 host -2
t "192.168.1.0" 192.168.1.0/24 host 0
t "192.168.1.255" 192.168.1.0/24 host 255
t "192.168.1.255" 192.168.1.0/24 host -1
t "10.0.0.100" 10.0.0.0/24 host 100
t "2001:db8::1" 2001:db8::/64 host 1
t "2001:db8::ffff:ffff:ffff:ffff" 2001:db8::/64 host -1

echo "=== host-index Tests ==="

t "0" 192.168.1.16/28 host-index
t "3" 192.168.1.35/28 host-index
t "14" 192.168.1.30/28 host-index
t "255" 192.168.1.255/24 host-index
t "0" 10.0.0.0/8 host-index

echo "=== subnet Tests ==="

t "192.168.1.0/28" 192.168.1.0/24 subnet 28 0
t "192.168.1.48/28" 192.168.1.0/24 subnet 28 3
t "192.168.1.240/28" 192.168.1.0/24 subnet 28 -1
t "192.168.1.0/28" "192.168.1.0/24" subnet +4 0
t "10.0.0.0/16" 10.0.0.0/8 subnet 16 0
t "10.1.0.0/16" 10.0.0.0/8 subnet 16 1
t "10.255.0.0/16" 10.0.0.0/8 subnet 16 -1
t "2001:db8:0:1::/64" 2001:db8::/32 subnet 64 1

# Interface address: preserve host bits
t "192.168.1.33/28" 192.168.1.1/24 subnet 28 2

echo "=== super Tests ==="

t "192.168.0.0/16" 192.168.1.0/24 super 16
t "192.0.0.0/8" 192.168.1.0/24 super 8
t "10.0.0.0/8" 10.1.2.0/24 super 8
t "192.168.0.0/16" 192.168.1.0/24 super -8
t "2001:db8::/32" 2001:db8:1::/48 super 32
te 2 192.168.1.0/24 super 28

echo "=== in / contains / overlaps Tests ==="

# in
te 0 192.168.1.0/24 in 192.168.0.0/16
te 0 192.168.1.1 in 192.168.1.0/24
te 1 192.168.0.0/16 in 192.168.1.0/24
te 0 10.0.0.0/24 in 10.0.0.0/8
te 1 10.0.0.0/8 in 10.0.0.0/24

# contains
te 0 192.168.0.0/16 contains 192.168.1.0/24
te 1 192.168.1.0/24 contains 192.168.0.0/16

# overlaps
te 0 192.168.1.0/24 overlaps 192.168.1.128/25
te 0 192.168.1.128/25 overlaps 192.168.1.0/24
te 1 192.168.1.0/24 overlaps 192.168.2.0/24
te 0 10.0.0.0/8 overlaps 10.1.0.0/16

echo "=== Comparison Tests (eq/ne/lt/le/gt/ge) ==="

te 0 192.168.1.1 eq 192.168.1.1
te 1 192.168.1.1 eq 192.168.1.2
te 0 192.168.1.1 ne 192.168.1.2
te 1 192.168.1.1 ne 192.168.1.1
te 0 192.168.1.1 lt 192.168.1.2
te 1 192.168.1.2 lt 192.168.1.1
te 1 192.168.1.1 lt 192.168.1.1
te 0 192.168.1.1 le 192.168.1.2
te 0 192.168.1.1 le 192.168.1.1
te 0 192.168.1.2 gt 192.168.1.1
te 0 192.168.1.1 ge 192.168.1.1

# With CIDR - compare network, then prefix len
te 0 192.168.1.0/24 lt 192.168.2.0/24
te 0 192.168.1.0/24 lt 192.168.1.0/25

echo "=== IPv6-Specific Tests ==="

# ipv4
t "1.2.3.4" ::ffff:1.2.3.4 ipv4
t "1.2.3.4" ::ffff:0102:0304 ipv4
t "192.168.1.1" 192.168.1.1 ipv4
t "0.0.0.1" ::1 ipv4

# 6to4
t "192.0.2.1" 2002:c000:0201::1 6to4
t "1.2.3.4" 2002:0102:0304::1 6to4
te 2 2001:db8::1 6to4

# teredo
t "65.54.227.120" 2001:0000:4136:e378:8000:63bf:3fff:fdd2 teredo server
t "192.0.2.45" 2001:0000:4136:e378:8000:63bf:3fff:fdd2 teredo client
te 2 2002::1 teredo server

# scope-id (numeric)
t "0" 192.168.1.1 scope-id
t "0" 2001:db8::1 scope-id

echo "=== Chaining Tests ==="

t "192.168.16.19" 192.168.0.0/16 subnet 20 1 host 19
t "10.0.4.255" 10.0.0.0/16 subnet 24 4 broadcast
t "192.168.10.0/24" 192.168.0.0/16 subnet 24 10 network
t "192.168.50.0/24" 192.168.5.0/24 super 16 subnet 24 50
t "10.1.48.100" 10.1.2.3/24 network super 16 subnet 20 3 host 100
t "10.0.255.255" 10.0.0.0/16 subnet 24 -1 host -1
t "192.168.6.0/24" 192.168.0.0/16 subnet 28 100 super 24

# Chain ending with non-chainable
t "3232236900" 192.168.0.0/16 subnet 24 5 host 100 to-int
te 0 10.0.0.0/16 subnet 24 5 in 10.0.0.0/20

echo "=== Error Handling Tests ==="

te 2 192.168.1.256 version
te 2 not-an-address version
te 2 192.168.1.1/33 version
te 2 192.168.1.1/255.0.255.0 version
te 2 192.168.1.1 broadcast
te 2 192.168.1.0/24 super 28
te 2 192.168.1.0/24 subnet 20 0
te 2 192.168.1.1 unknowncmd

echo ""
echo "========================================"
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo "========================================"

[ "$FAIL" -eq 0 ] && exit 0 || exit 1
