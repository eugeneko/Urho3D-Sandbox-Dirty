#pragma once

#include <FlexEngine/Common.h>

#include <Urho3D/Core/Variant.h>

namespace Urho3D
{

class BoundingBox;

}

namespace FlexEngine
{

/// Hash generator.
class Hash
{
public:
    /// Construct.
    explicit Hash(unsigned long long hash = 0);
    /// Reset.
    void Reset(unsigned long long hash = 0);
    /// Get 64-bit hash value.
    unsigned long long GetHash64() const { return hash_; }
    /// Get hash value.
    unsigned GetHash() const { return static_cast<unsigned>((hash_ & 0xffffffff) ^ (hash_ >> 32)); }

    /// Hash a 64-bit integer.
    void HashInt64(long long value);
    /// Hash a 32-bit integer.
    void HashInt(int value);
    /// Hash a 64-bit unsigned integer.
    void HashUInt64(unsigned long long value);
    /// Hash unsigned integer.
    void HashUInt(unsigned value);
    /// Hash pointer.
    void HashPointer(const void* value);
    /// Hash a float.
    void HashFloat(float value);
    /// Hash a double.
    void HashDouble(double value);
    /// Hash an IntRect.
    void HashIntRect(const IntRect& value);
    /// Hash an IntVector2.
    void HashIntVector2(const IntVector2& value);
    /// Hash a Rect.
    void HashRect(const Rect& value);
    /// Hash a Vector2.
    void HashVector2(const Vector2& value);
    /// Hash a Vector3.
    void HashVector3(const Vector3& value);
    /// Hash a Vector4.
    void HashVector4(const Vector4& value);
    /// Hash a quaternion.
    void HashQuaternion(const Quaternion& value);
    /// Hash a Matrix3.
    void HashMatrix3(const Matrix3& value);
    /// Hash a Matrix3x4.
    void HashMatrix3x4(const Matrix3x4& value);
    /// Hash a Matrix4.
    void HashMatrix4(const Matrix4& value);
    /// Hash a color.
    void HashColor(const Color& value);
    /// Hash a bounding box.
    void HashBoundingBox(const BoundingBox& value);
    /// Hash a null-terminated string.
    void HashString(const String& value);
    /// Hash a buffer, with size encoded as VLE.
    void HashBuffer(const PODVector<unsigned char>& buffer);
    /// Hash a resource reference.
    void HashResourceRef(const ResourceRef& value);
    /// Hash a resource reference list.
    void HashResourceRefList(const ResourceRefList& value);
    /// Hash a variant.
    void HashVariant(const Variant& value);
    /// Hash a variant vector.
    void HashVariantVector(const VariantVector& value);
    /// Hash a variant vector.
    void HashStringVector(const StringVector& value);
    /// Hash a variant map.
    void HashVariantMap(const VariantMap& value);
    /// Hash enum.
    template <class T>
    void HashEnum(T value) { return HashUInt(static_cast<unsigned>(value)); }

private:
    /// Hash.
    unsigned long long hash_;

};

}

