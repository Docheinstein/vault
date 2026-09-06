#ifndef SECURESTRING_H
#define SECURESTRING_H

#include "sodium.h"

#include <cstring>
#include <iostream>

class SecureString {
public:
    SecureString() {
        capacity_ = 0;
        size_ = 0;
    }

    ~SecureString() {
        // std::cout << std::endl
        // << "SecureString::~SecureString: erasing SecureString content with sodium_free" << std::endl;
        sodium_free(data_);
    }

    SecureString(const SecureString& other) = delete;
    SecureString& operator=(const SecureString& other) = delete;

    SecureString(SecureString&& other) noexcept {
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SecureString& operator=(SecureString&& other) noexcept {
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

    void append(SecureString&& other) {
        reserve(size_ + other.size_);
        memcpy(data_ + size_, other.data_, other.size_);
        size_ += other.size_;
    }

    void append(const SecureString& other) {
        reserve(size_ + other.size_);
        memcpy(data_ + size_, other.data_, other.size_);
        size_ += other.size_;
    }

    void append(const unsigned char* data, size_t size) {
        reserve(size_ + size);
        memcpy(data_ + size_, data, size);
        size_ += size;
    }

    unsigned char* prepare_append(size_t count) {
        reserve(size_ + count);
        return data_ + size_;
    }

    void commit_append(size_t actual) {
        size_ += actual;
    }

    void reserve(size_t new_capacity) {
        // std::cout << std::endl << "SecureString::reserve called for (" << new_capacity << ")" << std::endl;
        if (capacity_ < new_capacity) {
            if (capacity_ == 0) {
                static constexpr size_t INITIAL_CAPACITY = 16;
                capacity_ = INITIAL_CAPACITY;
            }

            do {
                capacity_ *= 2;
            } while (capacity_ < new_capacity);
            // std::cout << std::endl
            // << "SecureString::reserve: allocating SecureString content with sodium_malloc(" << capacity_
            // << ")" << std::endl;

            auto* new_data = static_cast<unsigned char*>(sodium_malloc(capacity_));
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

    unsigned char* data() const {
        return data_;
    }

private:
    size_t capacity_ {};
    size_t size_ {};
    unsigned char* data_ {};
};

#endif // SECURESTRING_H
