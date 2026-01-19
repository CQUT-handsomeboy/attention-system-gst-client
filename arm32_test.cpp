#include <iostream>
#include <openssl/opensslv.h>
#include <openssl/crypto.h>

int main() {
    std::cout << "Compile-time OpenSSL version: " << OPENSSL_VERSION_TEXT << std::endl;
    std::cout << "Run-time OpenSSL version: " << OpenSSL_version(OPENSSL_VERSION) << std::endl;
    return 0;
}