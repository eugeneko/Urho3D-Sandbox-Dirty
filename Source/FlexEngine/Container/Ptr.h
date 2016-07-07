#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Container/Swap.h>

#include <type_traits>

namespace FlexEngine
{

/// Unique pointer template class.
template <class T> class UniquePtr
{
public:
    /// Construct empty.
    UniquePtr() { }

    /// Construct empty.
    UniquePtr(std::nullptr_t) { }

    /// Construct from pointer.
    explicit UniquePtr(T* ptr) : ptr_(ptr) { }

    /// Move-construct from UniquePtr.
    UniquePtr(UniquePtr && up) : ptr_(up.Detach()) { }

    /// Assign from pointer.
    UniquePtr& operator = (T* ptr)
    {
        Reset(ptr);
        return *this;
    }

    /// Move-assign from pointer UniquePtr
    UniquePtr& operator = (UniquePtr && up)
    {
        Reset(up.Detach());
        return *this;
    }

    /// Point to the object.
    T* operator ->() const
    {
        assert(ptr_);
        return ptr_;
    }

    /// Dereference the object.
    T& operator *() const
    {
        assert(ptr_);
        return *ptr_;
    }

    /// Test for less than with another shared pointer.
    template <class U>
    bool operator <(const UniquePtr<U>& rhs) const { return ptr_ < rhs.ptr_; }

    /// Test for equality with another shared pointer.
    template <class U>
    bool operator ==(const UniquePtr<U>& rhs) const { return ptr_ == rhs.ptr_; }

    /// Test for inequality with another shared pointer.
    template <class U>
    bool operator !=(const UniquePtr<U>& rhs) const { return ptr_ != rhs.ptr_; }

    /// Swap with another UniquePtr.
    void Swap(UniquePtr& up) { Swap(ptr_, up.ptr_); }

    /// Detach pointer from UniquePtr without destroying.
    T* Detach()
    {
        T* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }

    /// Check if the pointer is null.
    bool Null() const { return ptr_ == nullptr; }

    /// Check if the pointer is not null.
    bool NotNull() const { return ptr_ != nullptr; }

    /// Return the raw pointer.
    T* Get() const { return ptr_; }

    /// Reset.
    void Reset(T* ptr = nullptr)
    {
        if (ptr_)
        {
            delete ptr_;
        }
        ptr_ = ptr;
    }

    /// Return hash value for HashSet & HashMap.
    unsigned ToHash() const { return static_cast<unsigned>((static_cast<size_t>(ptr_) / sizeof(T))); }

    /// Destruct.
    ~UniquePtr()
    {
        Reset();
    }

private:
    T* ptr_ = nullptr;

};

/// Cast LVref to RVrev.
template <class T>
T && Move(T& object)
{
    return static_cast<T&&>(object);
}

/// Swap UniquePtr-s.
template <class T>
void Swap(UniquePtr<T>& first, UniquePtr<T>& second)
{
    first.Swap(second);
}

/// Make UniquePtr.
template <class T, class ... Args>
UniquePtr<T> MakeUnique(Args && ... args)
{
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

/// Make SharedPtr.
template <class T, class ... Args>
SharedPtr<T> MakeShared(Args && ... args)
{
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

}
