#pragma once
#include <memory>
#include "util.h"

// Note: T's destructor will not always be invoked
template <class T>
struct DynArray
{
    DynArray() : capacity(16), size(0)
    {
        arr = std::make_unique<T[]>(capacity);
    };

    DynArray(const DynArray& other) : size(other.size), capacity(other.capacity)
    {
        // make deep copy
        arr = std::make_unique<T[]>(other.capacity);
        std::copy(std::begin(other.arr), std::end(other.arr), std::begin(arr));
    }

    DynArray(DynArray&& other) noexcept = default;

    DynArray& operator=(const DynArray& rhs)
    {
        size = rhs.size;
        if (arr == nullptr || capacity < rhs.capacity)
        {
            capacity = rhs.capacity;
            arr = std::make_unique<T[]>(rhs.capacity);
        }
        // make deep copy
        std::copy(std::begin(rhs.arr), std::end(rhs.arr), std::begin(arr));
        return *this;
    }

    DynArray& operator=(const DynArray&& rhs) = default;

    ~DynArray() = default;

    T& operator[](const uint idx) const
    {
        assert(idx < capacity);
        return arr[idx];
    }

    void push(T val)
    {
        if (size == capacity)
        {
            // resize
            capacity *= 2;
            std::unique_ptr<T[]> tarr = std::make_unique<T[]>(capacity);
            std::move(std::begin(arr), std::end(arr), std::begin(tarr));
            arr = std::move(tarr);
        }
        assert(size < capacity);
        arr[size] = std::move(val);
        size++;
    }

    void erase()
    {
        size = 0;
    }

    std::unique_ptr<T[]> arr;
    uint capacity;
    uint size;
};