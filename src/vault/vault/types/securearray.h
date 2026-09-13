#ifndef SECUREARRAY_H
#define SECUREARRAY_H

#include "sodium.h"

template <typename T, size_t N>
class SecureArray {
public:
    static constexpr size_t Size = N;

    SecureArray() {
        data_ = static_cast<T*>(sodium_malloc(Size * sizeof(T)));
    }

    ~SecureArray() {
        sodium_free(data_);
    }

    SecureArray(const SecureArray& other) = delete;
    SecureArray& operator=(const SecureArray& other) = delete;

    SecureArray(SecureArray&& other) noexcept {
        data_ = other.data_;
        other.data_ = nullptr;
    }

    SecureArray& operator=(SecureArray&& other) noexcept {
        if (this != &other) {
            sodium_free(data_);
            data_ = other.data_;
            other.data_ = nullptr;
        }
        return *this;
    }

    T* data() const {
        return data_;
    }

    constexpr size_t size() const {
        return Size;
    }

private:
    T* data_ {};
};

#endif // SECUREARRAY_H
