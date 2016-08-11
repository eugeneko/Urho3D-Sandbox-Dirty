#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/Vector.h>

namespace FlexEngine
{

/// Reverse vector.
template <class T>
void ReverseVector(Vector<T>& vec)
{
    for (unsigned i = 0; i < vec.Size() / 2; ++i)
    {
        Swap(vec[i], vec[vec.Size() - i - 1]);
    }
}

/// Reverse POD vector.
template <class T>
void ReverseVector(PODVector<T>& vec)
{
    for (unsigned i = 0; i < vec.Size() / 2; ++i)
    {
        Swap(vec[i], vec[vec.Size() - i - 1]);
    }
}

/// Pop element from vector.
template <class T>
T PopElement(Vector<T>& vec)
{
    assert(!vec.Empty());
    const T element = vec.Back();
    vec.Pop();
    return element;
}

/// Pop element from POD vector.
template <class T>
T PopElement(PODVector<T>& vec)
{
    assert(!vec.Empty());
    const T element = vec.Back();
    vec.Pop();
    return element;
}

/// Construct vector of elements.
template <class T>
Vector<T> ConstructVector(unsigned size, const T& value)
{
    Vector<T> result(size);
    for (T& elem : result)
    {
        elem = value;
    }
    return result;
}

/// Construct POD vector of elements.
template <class T>
PODVector<T> ConstructPODVector(unsigned size, const T& value)
{
    PODVector<T> result(size);
    for (T& elem : result)
    {
        elem = value;
    }
    return result;
}

/// Push elements to vector.
template <class T, class U>
void PushElements(Vector<T>& vec, const U& container)
{
    for (const auto& element : container)
    {
        vec.Push(element);
    }
}

}
