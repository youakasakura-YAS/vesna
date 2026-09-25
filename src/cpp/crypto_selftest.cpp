// crypto_selftest.cpp — NIST 向量验证（临时测试，不入库）
#include <cstdio>
#include <string>
#include "crypto.h"

using namespace vesna;

int main() {
    // SHA-256 FIPS 180-4 向量
    printf("sha256(empty) = %s\n", sha256Hex("").c_str());
    printf("sha256(abc)   = %s\n", sha256Hex("abc").c_str());
    printf("expect empty  = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\n");
    printf("expect abc    = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n");

    // AES-256 ECB FIPS-197 C.3 向量（块级）
    uint8_t key[32];
    for (int i = 0; i < 32; ++i) key[i] = (uint8_t)i;  // 000102...1f
    uint8_t rk[240];
    aesExpandKey(key, rk);
    uint8_t pt[16];
    for (int i = 0; i < 16; ++i) pt[i] = (uint8_t)(i * 0x11);  // 001122...ff
    uint8_t ct[16];
    aesEncryptBlock(pt, ct, rk);
    printf("aes256-ecb ct  = ");
    for (int i = 0; i < 16; ++i) printf("%02x", ct[i]);
    printf("\nexpect ecb     = 8ea2b7ca516745bfeafc49904b496089\n");
    uint8_t rt[16];
    aesDecryptBlock(ct, rt, rk);
    printf("roundtrip ok   = %s\n", std::memcmp(pt, rt, 16) == 0 ? "yes" : "NO");

    // CBC 往返 + PKCS7（含 16 倍数长度边界）
    const char* texts[] = {"", "a", "hello world", "0123456789abcdef", "0123456789abcdef0123456789abcdef"};
    bool allok = true;
    for (const char* t : texts) {
        std::string enc = aesEncryptCbc(t, "my-key");
        std::string dec = aesDecryptCbc(enc, "my-key");
        bool ok = (dec == t);
        if (!ok) allok = false;
        printf("cbc roundtrip %-40s ok=%s len=%zu\n", t, ok ? "yes" : "NO", enc.size());
    }
    printf("ALL CBC OK = %s\n", allok ? "yes" : "NO");
    return allok && sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" ? 0 : 1;
}
