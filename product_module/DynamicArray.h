#pragma once
// DynamicArray.h
// ----------------------------------------------------------------------------
// Custom Lightweight Dynamic Array Container
// Part of the Product Management and Billing Calculation Module.
//
// Pure, standalone C++ container with ZERO standard library headers.
// Completely free of <cstddef>, <utility>, <vector>, <map>, <sstream>,
// <cassert>, and <cctype>.
// Implements Rule of Five memory management with exponential resizing.
// Fully protected against self-referential push_back and emplace_back
// use-after-free bugs during reallocation.
// ----------------------------------------------------------------------------

namespace ProductBilling {

namespace Detail {
    // Custom type trait to strip references for manual move semantics without <utility>
    template <typename T>
    struct RemoveReference {
        using Type = T;
    };

    template <typename T>
    struct RemoveReference<T&> {
        using Type = T;
    };

    template <typename T>
    struct RemoveReference<T&&> {
        using Type = T;
    };
} // namespace Detail

// Custom implementation of move semantics without <utility>
template <typename T>
constexpr typename Detail::RemoveReference<T>::Type&& customMove(T&& arg) noexcept {
    return static_cast<typename Detail::RemoveReference<T>::Type&&>(arg);
}

// Custom implementation of perfect forwarding for emplace_back without <utility>
template <typename T>
constexpr T&& customForward(typename Detail::RemoveReference<T>::Type& arg) noexcept {
    return static_cast<T&&>(arg);
}

template <typename T>
constexpr T&& customForward(typename Detail::RemoveReference<T>::Type&& arg) noexcept {
    return static_cast<T&&>(arg);
}

/**
 * @brief Manually managed dynamic array with automatic capacity doubling.
 * @tparam T Type of elements stored.
 */
template <typename T>
class DynamicArray {
private:
    T*  m_data;
    int m_size;
    int m_capacity;

    void reallocate(int newCapacity) {
        T* newData = new T[newCapacity];
        for (int i = 0; i < m_size; ++i) {
            newData[i] = customMove(m_data[i]);
        }
        delete[] m_data;
        m_data = newData;
        m_capacity = newCapacity;
    }

public:
    DynamicArray()
        : m_data(nullptr)
        , m_size(0)
        , m_capacity(0)
    {
    }

    explicit DynamicArray(int initialCapacity)
        : m_data(initialCapacity > 0 ? new T[initialCapacity] : nullptr)
        , m_size(0)
        , m_capacity(initialCapacity > 0 ? initialCapacity : 0)
    {
    }

    ~DynamicArray() {
        delete[] m_data;
    }

    DynamicArray(const DynamicArray& other)
        : m_data(nullptr)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        if (m_capacity > 0) {
            m_data = new T[m_capacity];
            for (int i = 0; i < m_size; ++i) {
                m_data[i] = other.m_data[i];
            }
        }
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            T* newData = nullptr;
            if (other.m_capacity > 0) {
                newData = new T[other.m_capacity];
                for (int i = 0; i < other.m_size; ++i) {
                    newData[i] = other.m_data[i];
                }
            }
            delete[] m_data;
            m_data = newData;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
        }
        return *this;
    }

    DynamicArray(DynamicArray&& other) noexcept
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            delete[] m_data;
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            other.m_data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    int size() const { return m_size; }
    int capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }

    T& front() { return m_data[0]; }
    const T& front() const { return m_data[0]; }
    T& back() { return m_data[m_size - 1]; }
    const T& back() const { return m_data[m_size - 1]; }

    void reserve(int newCapacity) {
        if (newCapacity > m_capacity) {
            reallocate(newCapacity);
        }
    }

    void push_back(const T& element) {
        if (m_size == m_capacity) {
            T copy = element; // Preserve value BEFORE reallocating to avoid dangling reference
            int newCap = (m_capacity == 0) ? 4 : m_capacity * 2;
            reallocate(newCap);
            m_data[m_size++] = customMove(copy);
        } else {
            m_data[m_size++] = element;
        }
    }

    void push_back(T&& element) {
        if (m_size == m_capacity) {
            T copy = customMove(element); // Move into temporary BEFORE reallocating
            int newCap = (m_capacity == 0) ? 4 : m_capacity * 2;
            reallocate(newCap);
            m_data[m_size++] = customMove(copy);
        } else {
            m_data[m_size++] = customMove(element);
        }
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (m_size == m_capacity) {
            T temp(customForward<Args>(args)...); // Construct BEFORE reallocating
            int newCap = (m_capacity == 0) ? 4 : m_capacity * 2;
            reallocate(newCap);
            m_data[m_size++] = customMove(temp);
        } else {
            m_data[m_size++] = T(customForward<Args>(args)...);
        }
    }

    void pop_back() {
        if (m_size > 0) {
            --m_size;
            m_data[m_size] = T(); // Promptly release resources
        }
    }

    template <typename IndexType>
    void erase(IndexType index) {
        int idx = static_cast<int>(index);
        if (idx >= 0 && idx < m_size) {
            for (int i = idx; i + 1 < m_size; ++i) {
                m_data[i] = customMove(m_data[i + 1]);
            }
            --m_size;
            m_data[m_size] = T(); // Promptly release resources of vacated slot
        }
    }

    void clear() {
        for (int i = 0; i < m_size; ++i) {
            m_data[i] = T(); // Promptly release resources of all active elements
        }
        m_size = 0;
    }

    T& operator[](int index) {
        return m_data[index];
    }

    const T& operator[](int index) const {
        return m_data[index];
    }

    template <typename IndexType>
    T& operator[](IndexType index) {
        return m_data[index];
    }

    template <typename IndexType>
    const T& operator[](IndexType index) const {
        return m_data[index];
    }

    T* begin() { return m_data; }
    const T* begin() const { return m_data; }
    T* end() { return m_data ? (m_data + m_size) : nullptr; }
    const T* end() const { return m_data ? (m_data + m_size) : nullptr; }
};

} // namespace ProductBilling
