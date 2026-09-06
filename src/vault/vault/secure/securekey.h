#ifndef SECUREKEY_H
#define SECUREKEY_H

#include "sodium.h"

#include <iostream>

class SecureKey {
public:
    SecureKey() {
        data = static_cast<unsigned char*>(sodium_allocarray(32, sizeof(unsigned char)));
    }

    ~SecureKey() {
        sodium_free(data);
    }

    SecureKey(const SecureKey& other) = delete;
    SecureKey& operator=(const SecureKey& other) = delete;

    SecureKey(SecureKey&& other) noexcept {
        data = other.data;
        other.data = nullptr;
    }

    SecureKey& operator=(SecureKey&& other) noexcept {
        if (this != &other) {
            sodium_free(data);
            data = other.data;
            other.data = nullptr;
        }
        return *this;
    }

    unsigned char* data {};
};

#endif // SECUREKEY_H
