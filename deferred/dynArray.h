#pragma once
#include "util.h"

template <class T>
struct DynArray
{
    DynArray() : capacity(16), size(0)
    {
        arr = new T[capacity];
    };

    ~DynArray()
    {
        if (arr != nullptr)
        {
            delete[] arr;
        }
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
            T* tarr = new T[capacity];
            memcpy(tarr, arr, sizeof(T) * size);
            delete[] arr;
            arr = tarr;
        }
        arr[size] = val;
        size++;
    }

    T* arr;
    uint capacity;
    uint size;
};