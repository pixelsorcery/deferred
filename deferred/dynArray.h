#pragma once
#include <memory>
#include "util.h"

template <class T>
struct DynArray
{
    DynArray() : capacity(16), size(0)
    {
        arr = std::make_unique<T[]>(capacity);
    };

    ~DynArray()
    {
    }

    T& operator[] (const uint idx) const
    {
        assert(idx < capacity);
        return arr[idx];
    }

    void push(T& val)
    {
        if (size == capacity)
        {
            // resize
            capacity *= 2;
            std::unique_ptr<T[]> tarr = std::make_unique<T[]>(capacity);
            for (uint i = 0; i < size; i++)
            {
                // loop to not mess up com object ref counts
                tarr[i] = arr[i];
            }
            arr = std::move(tarr);
        }
        assert(size < capacity);
        arr[size] = val; // todo: use placement new
        size++;
    }

    void erase()
    {
        if (arr != nullptr)
        {
            size = 0;
            capacity = 16;
            arr = std::make_unique<T[]>(capacity);
        }
    }

    DynArray(const DynArray& other)
    {
        size = other.size;
        capacity = other.capacity;
        // make deep copy
        arr = std::make_unique<T[]>(other.capacity);

        for (uint i = 0; i < other.size; i++)
        {
            arr[i] = other.arr[i];
        }
    }

    DynArray(DynArray&& other)
    {
        size = other.size;
        capacity = other.capacity;
        arr = std::move(other.arr);
        other.arr = nullptr;
        other.capacity = 0;
        other.size = 0;
    }

    DynArray& operator=(const DynArray& rhs)
    {
        assert(arr != nullptr);
        size = rhs.size;
        capacity = rhs.capacity;
        // make deep copy
        arr = std::make_unique<T[]>(rhs.capacity);
        for (uint i = 0; i < rhs.size; i++)
        {
            arr[i] = rhs.arr[i];
        }
        return *this;
    }

    DynArray& operator=(const DynArray&& rhs)
    {
        size = rhs.size;
        capacity = rhs.capacity;
        arr = std::move(rhs.arr);
        rhs.arr = nullptr;
        rhs.capacity = 0;
        rhs.size = 0;
        return *this;
    }

    std::unique_ptr<T[]> arr;
    uint capacity;
    uint size;
};