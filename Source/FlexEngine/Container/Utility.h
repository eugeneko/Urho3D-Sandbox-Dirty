#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Container/Vector.h>

namespace Urho3D
{

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

}

}
