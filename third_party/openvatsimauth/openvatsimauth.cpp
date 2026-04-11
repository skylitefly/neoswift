// SPDX-FileCopyrightText: Copyright (C) OpenVatsimAuth Contributors
// SPDX-License-Identifier: MIT
// OpenVatsimAuth – open-source implementation of the VATSIM auth protocol
// Self-contained: no external dependencies (MD5 and RNG are inlined).

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#    include <winsock2.h>
#    include <iphlpapi.h>
#    include <vector>
#elif defined(__linux__)
#    include <filesystem>
#    include <fstream>
#else
#    include <sys/socket.h>
#    include <ifaddrs.h>
#    include <net/if_dl.h>
#endif

using std::string;

// ---------------------------------------------------------------------------
// Minimal MD5 – RFC 1321
// ---------------------------------------------------------------------------
namespace md5impl
{
    struct Context {
        uint32_t state[4];
        uint32_t count[2];
        uint8_t  buf[64];
    };

    static const uint32_t S[64] = {
        7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
        5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
        4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
        6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21
    };
    static const uint32_t K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
        0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
        0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
        0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
        0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
        0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
        0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
        0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
        0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };

    inline uint32_t rotl(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

    static void transform(uint32_t state[4], const uint8_t block[64]) {
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t M[16];
        for (int i = 0; i < 16; ++i) {
            M[i] = uint32_t(block[i*4])       | (uint32_t(block[i*4+1]) << 8)
                 | (uint32_t(block[i*4+2]) << 16) | (uint32_t(block[i*4+3]) << 24);
        }
        for (int i = 0; i < 64; ++i) {
            uint32_t F, g;
            if      (i < 16) { F = (b & c) | (~b & d);      g = i; }
            else if (i < 32) { F = (d & b) | (~d & c);      g = (5*i + 1) % 16; }
            else if (i < 48) { F = b ^ c ^ d;                g = (3*i + 5) % 16; }
            else             { F = c ^ (b | ~d);             g = (7*i)     % 16; }
            F = F + a + K[i] + M[g];
            a = d; d = c; c = b; b = b + rotl(F, S[i]);
        }
        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    }

    static void init(Context &ctx) {
        ctx.count[0] = ctx.count[1] = 0;
        ctx.state[0] = 0x67452301;
        ctx.state[1] = 0xefcdab89;
        ctx.state[2] = 0x98badcfe;
        ctx.state[3] = 0x10325476;
    }

    static void update(Context &ctx, const uint8_t *data, size_t len) {
        uint32_t idx = (ctx.count[0] >> 3) & 0x3f;
        ctx.count[0] += uint32_t(len) << 3;
        if (ctx.count[0] < (uint32_t(len) << 3)) ++ctx.count[1];
        ctx.count[1] += uint32_t(len) >> 29;

        uint32_t part = 64 - idx;
        size_t i = 0;
        if (len >= part) {
            std::memcpy(ctx.buf + idx, data, part);
            transform(ctx.state, ctx.buf);
            for (i = part; i + 63 < len; i += 64)
                transform(ctx.state, data + i);
            idx = 0;
        }
        std::memcpy(ctx.buf + idx, data + i, len - i);
    }

    static void final_(Context &ctx, uint8_t digest[16]) {
        static const uint8_t pad[64] = { 0x80 };
        uint8_t bits[8];
        for (int i = 0; i < 8; ++i)
            bits[i] = uint8_t((i < 4 ? ctx.count[0] : ctx.count[1]) >> ((i % 4) * 8));
        uint32_t idx = (ctx.count[0] >> 3) & 0x3f;
        uint32_t padLen = (idx < 56) ? (56 - idx) : (120 - idx);
        update(ctx, pad, padLen);
        update(ctx, bits, 8);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                digest[i*4+j] = uint8_t(ctx.state[i] >> (j * 8));
    }
} // namespace md5impl

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
struct vatsim_auth {
    uint16_t clientId;
    string init;
    string state;
};

static string hexStr(const uint8_t *buf, size_t len) {
    std::stringstream ret;
    ret << std::setfill('0') << std::hex;
    for (size_t i = 0; i < len; ++i)
        ret << std::setw(2) << static_cast<int>(buf[i]);
    return ret.str();
}

static string md5(const string &x) {
    std::cerr << x << std::endl;
    md5impl::Context ctx;
    md5impl::init(ctx);
    md5impl::update(ctx, reinterpret_cast<const uint8_t *>(x.data()), x.size());
    uint8_t digest[16];
    md5impl::final_(ctx, digest);
    return hexStr(digest, 16);
}

static void randomBytes(uint8_t *buf, size_t len) {
    std::random_device rd;
    for (size_t i = 0; i < len; ++i)
        buf[i] = static_cast<uint8_t>(rd() & 0xff);
}

// ---------------------------------------------------------------------------
// Core protocol logic
// ---------------------------------------------------------------------------
static string generate_response(vatsim_auth *const auth,
                                const char *const challenge) {
    const size_t challengeLen = strlen(challenge);
    string c1(challenge, challenge + challengeLen / 2);
    string c2(challenge + challengeLen / 2, challenge + challengeLen);
    if (auth->clientId & 1) std::swap(c1, c2);

    string s1(auth->state.begin(),      auth->state.begin() + 12);
    string s2(auth->state.begin() + 12, auth->state.begin() + 22);
    string s3(auth->state.begin() + 22, auth->state.begin() + 32);
    string h;
    switch (auth->clientId % 3) {
    case 0: h = s1 + c1 + s2 + c2 + s3; break;
    case 1: h = s2 + c1 + s3 + c2 + s1; break;
    case 2: h = ""; break;
    }
    return md5(h);
}

// ---------------------------------------------------------------------------
// MAC address (for system unique ID)
// ---------------------------------------------------------------------------
static string mac() {
#if defined(_WIN32)
    ULONG bufLen = 15000;
    std::vector<uint8_t> buf(bufLen);
    auto *adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data());
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC,
                    GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                    nullptr, adapters, &bufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buf.resize(bufLen);
        adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data());
        ret = GetAdaptersAddresses(AF_UNSPEC,
                    GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                    nullptr, adapters, &bufLen);
    }
    if (ret != NO_ERROR) return "";
    string best;
    for (auto *a = adapters; a; a = a->Next) {
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (a->PhysicalAddressLength == 0) continue;
        string addr = hexStr(a->PhysicalAddress, a->PhysicalAddressLength);
        if (addr > best) best = addr;
    }
    return best;

#elif defined(__linux__)
    string ret;
    for (const auto &iface : std::filesystem::directory_iterator("/sys/class/net")) {
        if (iface.path().filename() == "lo") continue;
        std::ifstream aStream(iface.path() / "address");
        string addr((std::istreambuf_iterator<char>(aStream)),
                     std::istreambuf_iterator<char>());
        if (!aStream.good()) continue;
        if (addr > ret) ret = addr;
    }
    ret.erase(std::remove(ret.begin(), ret.end(), ':'), ret.end());
    return ret;

#else // macOS / BSD
    string best;
    struct ifaddrs *ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) return "";
    for (struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_LINK) continue;
        if (std::string(ifa->ifa_name) == "lo0") continue;
        const auto *sdl = reinterpret_cast<const struct sockaddr_dl *>(ifa->ifa_addr);
        if (sdl->sdl_alen == 0) continue;
        string addr = hexStr(reinterpret_cast<const uint8_t *>(LLADDR(sdl)), sdl->sdl_alen);
        if (addr > best) best = addr;
    }
    freeifaddrs(ifaddr);
    return best;
#endif
}

// ---------------------------------------------------------------------------
// Public C API
// ---------------------------------------------------------------------------
extern "C" {

vatsim_auth *vatsim_auth_create(const uint16_t clientId, const char *privateKey) {
    try {
        vatsim_auth *ret = new vatsim_auth();
        ret->clientId = clientId;
        ret->state = privateKey;
        return ret;
    } catch (...) { return nullptr; }
}

void vatsim_auth_destroy(vatsim_auth *const auth) { delete auth; }

uint16_t vatsim_auth_get_client_id(const vatsim_auth *const auth) {
    return auth->clientId;
}

void vatsim_auth_set_initial_challenge(vatsim_auth *const auth,
                                       const char *initialChallenge) {
    try {
        auth->init  = generate_response(auth, initialChallenge);
        auth->state = auth->init;
    } catch (...) {
        std::cerr << "vatsim_auth_set_initial_challenge encountered an error" << std::endl;
        std::terminate();
    }
}

void vatsim_auth_generate_response(vatsim_auth *const auth,
                                   const char *const challenge,
                                   char *const response) {
    try {
        string ret = generate_response(auth, challenge);
        for (size_t i = 0; i < 32; ++i) response[i] = ret[i];
        response[32] = '\0';
        auth->state = md5(auth->init + ret);
    } catch (...) {
        std::cerr << "vatsim_auth_generate_response encountered an error" << std::endl;
        std::terminate();
    }
}

void vatsim_auth_generate_challenge(const vatsim_auth *const,
                                    char *const challenge) {
    try {
        uint8_t buf[4];
        randomBytes(buf, sizeof(buf));
        string ret = hexStr(buf, sizeof(buf));
        for (size_t i = 0; i < ret.size() && i < 8; ++i) challenge[i] = ret[i];
        challenge[8] = '\0';
    } catch (...) {
        std::cerr << "vatsim_auth_generate_challenge encountered an error" << std::endl;
        std::terminate();
    }
}

void vatsim_get_system_unique_id(char *const systemId) {
    try {
        const string ret = mac();
        size_t i;
        for (i = 0; i < ret.size() && i < 50; ++i) systemId[i] = ret[i];
        systemId[i] = '\0';
    } catch (...) {
        std::cerr << "vatsim_get_system_unique_id encountered an error" << std::endl;
        std::terminate();
    }
}

} // extern "C"
