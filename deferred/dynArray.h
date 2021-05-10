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
        for (uint i = 0; i < other.size; i++)
        {
            arr[i] = other[i];
        }
    }

    DynArray(DynArray&& other) noexcept : 
        size(other.size),
        capacity(other.capacity),
        arr(std::move(other.arr)) {}

    DynArray& operator=(const DynArray& rhs)
    {
        size = rhs.size;
        if (arr == nullptr || capacity < rhs.capacity)
        {
            capacity = rhs.capacity;
            arr = std::make_unique<T[]>(rhs.capacity);
        }
        // make deep copy
        for (uint i = 0; i < rhs.size; i++)
        {
            arr[i] = rhs[i];
        }
        return *this;
    }

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
            for (uint i = 0; i < size; i++)
            {
                tarr[i] = arr[i];
            }
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

    T& last()
    {
        assert(size > 0);
        return arr[size - 1];
    }

    uint size;
    uint capacity;
    std::unique_ptr<T[]> arr;
};