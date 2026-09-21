#ifndef SECUREVECTOR_H
#define SECUREVECTOR_H

#include <cstring>
#include <iostream>

#include "sodium.h"

template <typename T>
class SecureVector {
public:
    SecureVector() {
        capacity_ = 0;
        size_ = 0;
    }

    ~SecureVector() {
        sodium_free(data_);
    }

    SecureVector(const SecureVector& other) = delete;
    SecureVector& operator=(const SecureVector& other) = delete;

    SecureVector(SecureVector&& other) noexcept {
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SecureVector& operator=(SecureVector&& other) noexcept {
        if (this != &other) {
            sodium_free(data_);
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void append(SecureVector&& other) {
        reserve(size_ + other.size_);
        memcpy(data_ + size_, other.data_, other.size_);
        size_ += other.size_;
    }

    void append(const SecureVector& other) {
        reserve(size_ + other.size_);
        memcpy(data_ + size_, other.data_, other.size_);
        size_ += other.size_;
    }

    void append(const unsigned char* data, size_t size) {
        reserve(size_ + size);
        memcpy(data_ + size_, data, size);
        size_ += size;
    }

    void append(const char* data) {
        append(reinterpret_cast<const unsigned char*>(data), strlen(data));
    }

    void reserve(size_t new_capacity) {
        if (capacity_ < new_capacity) {
            if (capacity_ == 0) {
                static constexpr size_t INITIAL_CAPACITY = 16;
                capacity_ = INITIAL_CAPACITY;
            }

            do {
                capacity_ *= 2;
            } while (capacity_ < new_capacity);

            auto* new_data = static_cast<unsigned char*>(sodium_malloc(capacity_ * sizeof(T)));
            if (data_) {
                memcpy(new_data, data_, size_);
                sodium_free(data_);
            }

            data_ = new_data;
        }
    }

    void resize(size_t new_size) {
        reserve(new_size);
        size_ = new_size;
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capacity_;
    }

    T* data() const {
        return data_;
    }

private:
    size_t capacity_ {};
    size_t size_ {};
    T* data_ {};
};

#endif // SECUREVECTOR_H
