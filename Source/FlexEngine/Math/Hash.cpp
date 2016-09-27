#include <FlexEngine/Math/Hash.h>

#include <Urho3D/Math/BoundingBox.h>

namespace FlexEngine
{

Hash::Hash(unsigned long long hash /*= 0*/)
    : hash_(hash)
{
}

void Hash::Reset(unsigned long long hash /*= 0*/)
{
    hash_ = hash;
}

void Hash::HashInt64(long long value)
{
    HashUInt64(static_cast<unsigned long long>(value));
}

void Hash::HashInt(int value)
{
    HashUInt(static_cast<unsigned>(value));
}

void Hash::HashUInt64(unsigned long long value)
{
    hash_ ^= value + 0x9e3779b9 + (hash_<<6) + (hash_>>2);
}

void Hash::HashUInt(unsigned value)
{
    HashUInt64(value);
}

void Hash::HashPointer(const void* value)
{
    HashInt64(static_cast<unsigned long long>(reinterpret_cast<std::size_t>(value)));
}

void Hash::HashFloat(float value)
{
    HashUInt(*reinterpret_cast<const unsigned*>(&value));
}

void Hash::HashDouble(double value)
{
    HashUInt64(*reinterpret_cast<const unsigned long long*>(&value));
}

void Hash::HashIntRect(const IntRect& value)
{
    HashInt(value.left_);
    HashInt(value.top_);
    HashInt(value.right_);
    HashInt(value.bottom_);
}

void Hash::HashIntVector2(const IntVector2& value)
{
    HashInt(value.x_);
    HashInt(value.y_);
}

void Hash::HashRect(const Rect& value)
{
    HashVector2(value.min_);
    HashVector2(value.max_);
}

void Hash::HashVector2(const Vector2& value)
{
    HashFloat(value.x_);
    HashFloat(value.y_);
}

void Hash::HashVector3(const Vector3& value)
{
    HashFloat(value.x_);
    HashFloat(value.y_);
    HashFloat(value.z_);
}

void Hash::HashVector4(const Vector4& value)
{
    HashFloat(value.x_);
    HashFloat(value.y_);
    HashFloat(value.z_);
    HashFloat(value.w_);
}

void Hash::HashQuaternion(const Quaternion& value)
{
    HashFloat(value.x_);
    HashFloat(value.y_);
    HashFloat(value.z_);
    HashFloat(value.w_);
}

void Hash::HashMatrix3(const Matrix3& value)
{
    for (int i = 0; i < 9; ++i)
        HashFloat(value.Data()[i]);
}

void Hash::HashMatrix3x4(const Matrix3x4& value)
{
    for (int i = 0; i < 12; ++i)
        HashFloat(value.Data()[i]);
}

void Hash::HashMatrix4(const Matrix4& value)
{
    for (int i = 0; i < 16; ++i)
        HashFloat(value.Data()[i]);
}

void Hash::HashColor(const Color& value)
{
    HashFloat(value.r_);
    HashFloat(value.g_);
    HashFloat(value.b_);
    HashFloat(value.a_);
}

void Hash::HashBoundingBox(const BoundingBox& value)
{
    HashVector3(value.min_);
    HashVector3(value.max_);
}

void Hash::HashString(const String& value)
{
    HashUInt(value.ToHash());
}

void Hash::HashBuffer(const PODVector<unsigned char>& buffer)
{
    const unsigned* blocks = reinterpret_cast<const unsigned*>(buffer.Buffer());
    for (unsigned i = 0; i < buffer.Size() / 4; ++i)
        HashUInt(blocks[i]);

    const unsigned tailStart = buffer.Size() / 4 * 4;
    unsigned tail = 0;
    for (unsigned i = tailStart; i < buffer.Size(); ++i)
        tail |= buffer[i] << (8 * (i - tailStart));
    HashUInt(tail);
}

void Hash::HashResourceRef(const ResourceRef& value)
{
    HashUInt(value.type_);
    HashString(value.name_);
}

void Hash::HashResourceRefList(const ResourceRefList& value)
{
    HashUInt(value.type_);
    HashUInt(value.names_.Size());
    for (unsigned i = 0; i < value.names_.Size(); ++i)
        HashString(value.names_[i]);
}

void Hash::HashVariant(const Variant& value)
{
    switch (value.GetType())
    {
    case VAR_NONE: HashInt(0); break;
    case VAR_INT: HashInt(value.GetInt()); break;
    case VAR_BOOL: HashInt(value.GetBool()); break;
    case VAR_FLOAT: HashFloat(value.GetFloat()); break;
    case VAR_VECTOR2: HashVector2(value.GetVector2()); break;
    case VAR_VECTOR3: HashVector3(value.GetVector3()); break;
    case VAR_VECTOR4: HashVector4(value.GetVector4()); break;
    case VAR_QUATERNION: HashQuaternion(value.GetQuaternion()); break;
    case VAR_COLOR: HashColor(value.GetColor()); break;
    case VAR_STRING: HashString(value.GetString()); break;
    case VAR_BUFFER: HashBuffer(value.GetBuffer()); break;
    case VAR_VOIDPTR: HashPointer(value.GetVoidPtr()); break;
    case VAR_RESOURCEREF: HashResourceRef(value.GetResourceRef()); break;
    case VAR_RESOURCEREFLIST: HashResourceRefList(value.GetResourceRefList()); break;
    case VAR_VARIANTVECTOR: HashVariantVector(value.GetVariantVector()); break;
    case VAR_VARIANTMAP: HashVariantMap(value.GetVariantMap()); break;
    case VAR_INTRECT: HashIntRect(value.GetIntRect()); break;
    case VAR_INTVECTOR2: HashIntVector2(value.GetIntVector2()); break;
    case VAR_PTR: HashPointer(value.GetPtr()); break;
    case VAR_MATRIX3: HashMatrix3(value.GetMatrix3()); break;
    case VAR_MATRIX3X4: HashMatrix3x4(value.GetMatrix3x4()); break;
    case VAR_MATRIX4: HashMatrix4(value.GetMatrix4()); break;
    case VAR_DOUBLE: HashDouble(value.GetDouble()); break;
    case VAR_STRINGVECTOR: HashStringVector(value.GetStringVector()); break;
    default:
        break;
    }
}

void Hash::HashVariantVector(const VariantVector& value)
{
    HashUInt(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i)
        HashVariant(value[i]);
}

void Hash::HashStringVector(const StringVector& value)
{
    HashUInt(value.Size());
    for (unsigned i = 0; i < value.Size(); ++i)
        HashString(value[i]);
}

void Hash::HashVariantMap(const VariantMap& value)
{
    HashUInt(value.Size());
    for (VariantMap::ConstIterator i = value.Begin(); i != value.End(); ++i)
    {
        HashUInt(i->first_);
        HashVariant(i->second_);
    }
}

}
