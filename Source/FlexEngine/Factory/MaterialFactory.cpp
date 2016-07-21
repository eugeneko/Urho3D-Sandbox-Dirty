#include <FlexEngine/Factory/MaterialFactory.h>

#include <FlexEngine/Factory/FactoryContext.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>

namespace FlexEngine
{

namespace
{

/// Check that variant type is vector or float.
bool IsVariantVector(VariantType type)
{
    switch (type)
    {
    case Urho3D::VAR_FLOAT:
    case Urho3D::VAR_VECTOR2:
    case Urho3D::VAR_VECTOR3:
    case Urho3D::VAR_VECTOR4:
        return true;
    default:
        return false;
    }
}

/// Uniform map.
using UniformGroup = HashMap<String, Variant>;

/// Get uniform group from node.
UniformGroup GetUniformGroup(XMLElement node)
{
    UniformGroup result;
    for (XMLElement uniformNode = node.GetChild("uniform"); uniformNode; uniformNode = uniformNode.GetNext("uniform"))
    {
        const String name = uniformNode.GetAttribute("name");
        if (name.Empty())
        {
            URHO3D_LOGERROR("Unfiorm name mustn't be empty");
            continue;
        }

        const String valueString = uniformNode.GetAttribute("value");
        if (valueString.Trimmed().Empty())
        {
            URHO3D_LOGERRORF("Value of unfiorm '%s' mustn't be empty", name.CString());
            continue;
        }

        const Variant value = ToVectorVariant(valueString);
        if (!IsVariantVector(value.GetType()))
        {
            URHO3D_LOGERRORF("Value of unfiorm '%s' must have from 1 to 4 components", name.CString());
            continue;
        }

        result.Insert(MakePair(name, value));
    }
    return result;
}

/// Material description.
struct MaterialDescription 
{
    /// Name of uniforms group to inject.
    String uniformsGroup_;
    /// Source material name.
    String sourceMaterialName_;
    /// Destination material name.
    String destMaterialName_;
};

}

void GenerateMaterialsFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    // Read uniform groups
    HashMap<String, UniformGroup> uniformGroups;
    for (XMLElement uniformsNode = node.GetChild("uniforms"); uniformsNode; uniformsNode = uniformsNode.GetNext("uniforms"))
    {
        const String& uniformsName = uniformsNode.GetAttribute("name");
        if (uniformsName.Empty())
        {
            URHO3D_LOGERROR("Uniform group name mustn't be empty");
            continue;
        }

        uniformGroups.Insert(MakePair(uniformsName, GetUniformGroup(uniformsNode)));
    }

    // Load materials
    Vector<MaterialDescription> materialDescs;
    for (XMLElement materialNode = node.GetChild("material"); materialNode; materialNode = materialNode.GetNext("material"))
    {
        // Load material description
        MaterialDescription desc;
        desc.uniformsGroup_ = materialNode.GetAttribute("uniforms");
        desc.sourceMaterialName_ = factoryContext.ExpandName(materialNode.GetAttribute("source"));
        desc.destMaterialName_ = factoryContext.ExpandName(materialNode.GetAttribute("dest"));

        // Check fields
        if (desc.sourceMaterialName_.Empty())
        {
            URHO3D_LOGERROR("Source material name mustn't be empty");
            continue;
        }
        if (desc.destMaterialName_.Empty())
        {
            URHO3D_LOGERROR("Destination material name mustn't be empty");
            continue;
        }

        materialDescs.Push(desc);
    }

    // Skip generation if possible
    bool alreadyGenerated = true;
    for (const MaterialDescription& desc : materialDescs)
    {
        if (!resourceCache.GetFile(desc.destMaterialName_))
        {
            alreadyGenerated = false;
            break;
        }
    }

    if (!factoryContext.forceGeneration_ && alreadyGenerated)
    {
        return;
    }

    // Generate materials
    for (const MaterialDescription& desc : materialDescs)
    {
        // Load source material
        SharedPtr<Material> sourceMaterial(resourceCache.GetResource<Material>(desc.sourceMaterialName_));
        if (!sourceMaterial)
        {
            URHO3D_LOGERRORF("Source material '%s' wasn't found", desc.sourceMaterialName_.CString());
            continue;
        }

        // Find uniform group
        if (!uniformGroups.Contains(desc.uniformsGroup_))
        {
            URHO3D_LOGERRORF("Uniform group '%s' wasn't found", desc.uniformsGroup_.CString());
            continue;
        }

        // Fill destination material
        SharedPtr<Material> destMaterial(sourceMaterial->Clone(desc.destMaterialName_));
        const UniformGroup& uniformGroup = uniformGroups[desc.uniformsGroup_];
        for (const HashMap<String, Variant>::KeyValue& elem : uniformGroup)
        {
            destMaterial->SetShaderParameter(elem.first_, elem.second_);
        }

        // Save material
        const String& outputFileName = factoryContext.outputDirectory_ + desc.destMaterialName_;
        CreateDirectoriesToFile(resourceCache, outputFileName);
        File outputFile(resourceCache.GetContext(), outputFileName, FILE_WRITE);
        if (outputFile.IsOpen())
        {
            destMaterial->Save(outputFile);
            outputFile.Close();

            // Reload
            resourceCache.ReloadResourceWithDependencies(desc.destMaterialName_);
        }
        else
        {
            URHO3D_LOGERRORF("Cannot save material to '%s'", outputFileName.CString());
        }
    }
}

}
