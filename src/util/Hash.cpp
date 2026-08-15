#include "Hash.h"

#include <mbedtls/sha256.h>
#include <ctype.h>

namespace friendbox::util {

String sha256Hex(const String& input) {
    unsigned char digest[32]{};
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    if (mbedtls_sha256_starts_ret(&ctx, 0) != 0 ||
        mbedtls_sha256_update_ret(
            &ctx,
            reinterpret_cast<const unsigned char*>(input.c_str()),
            input.length()) != 0 ||
        mbedtls_sha256_finish_ret(&ctx, digest) != 0) {
        mbedtls_sha256_free(&ctx);
        return String();
    }

    mbedtls_sha256_free(&ctx);

    static const char* hex = "0123456789abcdef";
    char output[65];

    for (size_t i = 0; i < 32; ++i) {
        output[i * 2] = hex[digest[i] >> 4];
        output[i * 2 + 1] = hex[digest[i] & 0x0F];
    }

    output[64] = '\0';
    return String(output);
}

bool isSha256Hex(const String& value) {
    if (value.length() != 64) return false;
    for (size_t i = 0; i < value.length(); ++i) {
        if (!isxdigit(static_cast<unsigned char>(value[i]))) return false;
    }
    return true;
}

}  // namespace friendbox::util
