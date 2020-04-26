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
            for (int i = 0; i < size; i++)
            {
                // loop to not mess up com object ref counts
                for (int i = 0; i < size; i++)
                {
                    tarr[i] = arr[i];
                }
            }
            delete[] arr;
            arr = tarr;
        }
        assert(size < capacity);
        arr[size] = val;
        size++;
    }

    DynArray(const DynArray& other)
    {
        assert(arr != nullptr);
        delete[] arr;
        size = other.size;
        capacity = other.capacity;
        // make deep copy
        arr = new T[other.capacity];
        memcpy(arr, other.arr, other.size * sizeof(T));
    }

    DynArray(DynArray&& other)
    {
        assert(0); // not tested.. do we want this to be destructive?
        if (arr != nullptr)
        {
            delete[] arr;
        }

        size = other.size;
        capacity = other.capacity;
        arr = other.arr; // move the pointer
        other.arr = nullptr;
        other.capacity = 0;
        other.size = 0;
    }

    DynArray& operator=(const DynArray& rhs)
    {
        assert(arr != nullptr);
        delete[] arr;
        size = rhs.size;
        capacity = rhs.capacity;
        // make deep copy
        arr = new T[rhs.capacity];
        for (int i = 0; i < rhs.size; i++)
        {
            // loop to not mess up com object ref counts
            arr[i] = rhs.arr[i];
        }
        return *this;
    }

    DynArray& operator=(const DynArray&& rhs)
    {
        assert(0); // not tested.. do we want this to be destructive?
        if (arr != nullptr)
        {
            delete[] arr;
        }

        size = rhs.size;
        capacity = rhs.capacity;
        arr = rhs.arr; // move the pointer
        rhs.arr = nullptr;
        rhs.capacity = 0;
        rhs.size = 0;
        return *this;
    }

    T* arr;
    uint capacity;
    uint size;
};