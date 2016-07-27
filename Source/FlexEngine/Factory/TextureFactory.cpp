#include <FlexEngine/Factory/TextureFactory.h>

#include <FlexEngine/Core/StringUtils.h>
#include <FlexEngine/Factory/FactoryContext.h>
#include <FlexEngine/Factory/ProxyGeometryFactory.h>
#include <FlexEngine/Resource/ResourceCacheHelpers.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/View.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

namespace
{

/// Get texture unit by name.
TextureUnit ParseTextureUnit(const String& name)
{
    const String str = name.ToLower().Trimmed();
    if (str == "diffuse" || str == "diff" || str == "0")
    {
        return TU_DIFFUSE;
    }
    else if(str == "normal" || str == "norm" || str == "1")
    {
        return TU_NORMAL;
    }
    else if (str == "specular" || str == "spec" || str == "2")
    {
        return TU_SPECULAR;
    }
    else if (str == "emissive" || str == "3")
    {
        return TU_EMISSIVE;
    }
    else
    {
        return MAX_TEXTURE_UNITS;
    }
}

}

SharedPtr<Texture2D> RenderViews(Context* context, unsigned width, unsigned height, const Vector<ViewDescription>& views)
{
    // Allocate destination buffers
    SharedPtr<Texture2D> texture = MakeShared<Texture2D>(context);
    texture->SetSize(width, height, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    SharedPtr<RenderSurface> renderSurface(texture->GetRenderSurface());

    // Prepare views
    Graphics* graphics = context->GetSubsystem<Graphics>();
    if (graphics->BeginFrame())
    {
        for (const ViewDescription& desc : views)
        {
            // Construct scene
            Scene scene(context);
            Octree* octree = scene.CreateComponent<Octree>();
            Zone* zone = scene.CreateComponent<Zone>();
            zone->SetAmbientColor(Color(1.0f, 1.0f, 1.0f));
            zone->SetFogColor(Color::TRANSPARENT);
            zone->SetBoundingBox(BoundingBox(
                Vector3(-M_LARGE_VALUE, -M_LARGE_VALUE, -M_LARGE_VALUE), Vector3(M_LARGE_VALUE, M_LARGE_VALUE, M_LARGE_VALUE)));
            scene.AddChild(desc.node_);
            scene.AddChild(desc.camera_);

            // Get camera
            Camera* camera = desc.camera_->GetComponent<Camera>();
            if (!camera)
            {
                URHO3D_LOGERROR("Camera node must contain camera component");
                continue;
            }

            // Setup viewport
            Viewport viewport(context);
            viewport.SetCamera(camera);
            viewport.SetRect(desc.viewport_);
            viewport.SetRenderPath(desc.renderPath_);
            viewport.SetScene(&scene);

            // Render scene
            View view(context);
            view.Define(renderSurface, &viewport);
            view.Update(FrameInfo());
            view.Render();

            scene.RemoveChild(desc.node_);
            scene.RemoveChild(desc.camera_);
        }
        graphics->EndFrame();
    }

    return texture;
}

SharedPtr<Image> ConvertTextureToImage(SharedPtr<Texture2D> texture)
{
    const unsigned width = texture->GetWidth();
    const unsigned height = texture->GetHeight();
    const unsigned dataSize = texture->GetDataSize(width, height);
    if (texture->GetFormat() != Graphics::GetRGBAFormat())
    {
        URHO3D_LOGERROR("Texture must have RGBA8 format");
        return nullptr;
    }
    
    PODVector<unsigned char> buffer(dataSize);
    texture->GetData(0, buffer.Buffer());
    SharedPtr<Image> image = MakeShared<Image>(texture->GetContext());
    image->SetSize(width, height, texture->GetComponents());
    image->SetData(buffer.Buffer());
    return image;
}

Vector<ViewDescription> ConstructViewsForTexture(Context* context, const TextureDescription& desc, const TextureMap& textures)
{
    ResourceCache* resourceCache = context->GetSubsystem<ResourceCache>();
    Vector<ViewDescription> views;

    for (const OrthoCameraDescription& cameraDesc : desc.cameras_)
    {
        ViewDescription viewDesc;

        // Create camera node
        viewDesc.camera_ = MakeShared<Node>(context);
        viewDesc.camera_->SetPosition(cameraDesc.position_);
        viewDesc.camera_->SetRotation(cameraDesc.rotation_);

        // Setup camera
        Camera* camera = viewDesc.camera_->CreateComponent<Camera>();
        if (camera)
        {
            camera->SetOrthographic(true);
            camera->SetFarClip(cameraDesc.farClip_);
            camera->SetOrthoSize(cameraDesc.size_);
        }

        // Create model node
        viewDesc.node_ = MakeShared<Node>(context);

        // Setup geometry
        for (const GeometryDescription& geometryDesc : desc.geometries_)
        {
            StaticModel* staticModel = viewDesc.node_->CreateComponent<StaticModel>();
            if (staticModel)
            {
                staticModel->SetModel(geometryDesc.model_);
                for (unsigned i = 0; i < geometryDesc.materials_.Size(); ++i)
                {
                    // Clone material and override textures
                    const SharedPtr<Material> material(geometryDesc.materials_[i]->Clone());
                    for (const HashMap<TextureUnit, String>::KeyValue& unitDesc : desc.textures_)
                    {
                        SharedPtr<Texture2D> texture;
                        textures.TryGetValue(unitDesc.second_, texture);
                        if (!texture && resourceCache)
                        {
                            texture = resourceCache->GetResource<Texture2D>(unitDesc.second_);
                        }
                        if (texture)
                        {
                            material->SetTexture(unitDesc.first_, texture);
                        }
                        else
                        {
                            URHO3D_LOGERRORF("Cannot resolve input texture name '%s'", unitDesc.second_.CString());
                        }
                    }

                    staticModel->SetMaterial(i, material);
                    viewDesc.objects_.Push(material);
                }
            }
        }

        // Setup other fields
        viewDesc.viewport_ = cameraDesc.viewport_;
        viewDesc.renderPath_ = desc.renderPath_;

        views.Push(viewDesc);
    }
    
    return views;
}

SharedPtr<Texture2D> RenderTexture(Context* context, const TextureDescription& desc, const TextureMap& textures)
{
    if (desc.cameras_.Empty() || desc.geometries_.Empty() || !desc.renderPath_)
    {
        // Just fill with color if nothing to render
        const SharedPtr<Image> image = MakeShared<Image>(context);
        image->SetSize(desc.width_, desc.height_, 4);
        image->Clear(desc.color_);

        const SharedPtr<Texture2D> texture = MakeShared<Texture2D>(context);
        texture->SetData(image);
        return texture;
    }
    else
    {
        const Vector<ViewDescription> views = ConstructViewsForTexture(context, desc, textures);
        return RenderViews(context, desc.width_, desc.height_, views);
    }
}

void GenerateTexturesFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    TextureFactory textureFactory(resourceCache.GetContext());
    textureFactory.SetName(factoryContext.currentDirectory_ + "/[temporary]");
    textureFactory.Load(node);
    if (!textureFactory.CheckAllOutputs(factoryContext.outputDirectory_) || factoryContext.forceGeneration_)
    {
        textureFactory.Generate(factoryContext.outputDirectory_);
    }
}

//////////////////////////////////////////////////////////////////////////

TextureFactory::TextureFactory(Context* context)
    : Resource(context)
{
    resourceCache_ = GetSubsystem<ResourceCache>();
    if (!resourceCache_)
    {
        URHO3D_LOGERROR("Resource cache subsystem must be initialized");
        return;
    }
}

void TextureFactory::RegisterObject(Context* context)
{
    context->RegisterFactory<TextureFactory>();
}

bool TextureFactory::BeginLoad(Deserializer& source)
{
    loadXMLFile_ = MakeShared<XMLFile>(context_);
    return loadXMLFile_->Load(source);
}

bool TextureFactory::EndLoad()
{
    if (loadXMLFile_)
    {
        return Load(loadXMLFile_->GetRoot());
    }
    else
    {
        return false;
    }
}

bool TextureFactory::Load(const XMLElement& source)
{
    if (!resourceCache_)
    {
        return false;
    }

    currectDirectory_ = GetFilePath(GetName());

    for (XMLElement textureNode = source.GetChild("texture"); textureNode; textureNode = textureNode.GetNext("texture"))
    {
        TextureDescription textureDesc;

        // Special case for single-color texture
        if (textureNode.HasAttribute("color"))
        {
            const String textureName = textureNode.GetAttribute("name");
            if (textureName.Empty())
            {
                URHO3D_LOGERROR("Texture name must be specified and non-zero");
                return false;
            }

            textureDesc.color_ = textureNode.GetColor("color");
            textureDesc.width_ = 1;
            textureDesc.height_ = 1;
            AddTexture(textureName, textureDesc);
            continue;
        }

        textureDesc.width_ = textureNode.GetUInt("width");
        if (textureDesc.width_ == 0)
        {
            URHO3D_LOGERROR("Texture width must be specified and non-zero");
            return false;
        }
        textureDesc.height_ = textureNode.GetUInt("height");
        if (textureDesc.height_ == 0)
        {
            URHO3D_LOGERROR("Texture height must be specified and non-zero");
            return false;
        }

        // Load geometries
        BoundingBox boundingBox;
        for (XMLElement geometryNode = textureNode.GetChild("geometry"); geometryNode; geometryNode = geometryNode.GetNext("geometry"))
        {
            GeometryDescription geometryDesc;

            // Load model
            const String modelName = geometryNode.GetAttribute("model").Trimmed().Replaced("@", currectDirectory_);
            geometryDesc.model_ = resourceCache_->GetResource<Model>(modelName);
            if (!geometryDesc.model_)
            {
                URHO3D_LOGERRORF("Source geometry model '%s' was not found", modelName.CString());
                return false;
            }

            // Load bounding box
            boundingBox.Merge(geometryDesc.model_->GetBoundingBox());

            // Load materials
            StringVector materialsList = geometryNode.GetAttribute("materials").Trimmed().Replaced("@", currectDirectory_).Split(';');
            for (XMLElement materialNode = geometryNode.GetChild("material"); materialNode; materialNode = materialNode.GetNext("material"))
            {
                materialsList.Push(materialNode.GetAttribute("name").Trimmed().Replaced("@", currectDirectory_));
            }
            for (const String& materialName : materialsList)
            {
                const SharedPtr<Material> material(resourceCache_->GetResource<Material>(materialName));
                geometryDesc.materials_.Push(material);
                if (!material)
                {
                    URHO3D_LOGERRORF("Source geometry model '%s' was not found", modelName.CString());
                    return false;
                }
            }

            textureDesc.geometries_.Push(geometryDesc);
        }

        // Load cameras
        for (XMLElement cameraNode = textureNode.GetChild("camera"); cameraNode; cameraNode = cameraNode.GetNext("camera"))
        {
            textureDesc.cameras_ += GenerateProxyCamerasFromXML(boundingBox, textureDesc.width_, textureDesc.height_, cameraNode);
        }

        // Load textures
        for (XMLElement inputNode = textureNode.GetChild("input"); inputNode; inputNode = inputNode.GetNext("input"))
        {
            const String unitName = inputNode.GetAttribute("unit");
            const TextureUnit unit = ParseTextureUnit(unitName);
            if (unit == MAX_TEXTURE_UNITS)
            {
                URHO3D_LOGERRORF("Unrecognized input texture unit '%s'", unitName.CString());
                return false;
            }

            const String textureName = inputNode.GetAttribute("texture");
            if (textureName.Empty())
            {
                URHO3D_LOGERROR("Input texture name mustn't be empty");
                return false;
            }

            textureDesc.textures_.Populate(unit, textureName);
        }

        // Load all variations
        Vector<Pair<String, String>> variations;
        for (XMLElement variationNode = textureNode.GetChild("variation"); variationNode; variationNode = variationNode.GetNext("variation"))
        {
            variations.Push(MakePair(variationNode.GetAttribute("name"), variationNode.GetAttribute("renderpath")));
        }
        if (!textureNode.HasChild("variation"))
        {
            variations.Push(MakePair(textureNode.GetAttribute("name"), textureNode.GetAttribute("renderpath")));
        }

        for (const Pair<String, String>& variation : variations)
        {
            if (variation.first_.Empty())
            {
                URHO3D_LOGERROR("Texture variation name must be specified and non-empty");
                return false;
            }

            if (FindTexture(variation.first_) >= 0)
            {
                URHO3D_LOGERRORF("Texture variation name '%s' must be unique", variation.first_.CString());
                return false;
            }

            SharedPtr<XMLFile> renderPath(resourceCache_->GetResource<XMLFile>(variation.second_));
            if (!renderPath)
            {
                URHO3D_LOGERRORF("Texture variation render path '%s' was not found", variation.second_.CString());
                return false;
            }

            textureDesc.renderPath_ = renderPath;
            AddTexture(variation.first_, textureDesc);
        }
    }

    for (XMLElement outputNode = source.GetChild("output"); outputNode; outputNode = outputNode.GetNext("output"))
    {
        const String& textureName = outputNode.GetAttribute("name");
        if (FindTexture(textureName) < 0)
        {
            URHO3D_LOGERRORF("Output texture '%s' was not found", textureName.CString());
            return false;
        }

        const String& fileName = outputNode.GetAttribute("file").Trimmed().Replaced("@", currectDirectory_);
        AddOutput(textureName, fileName);
    }

    return true;
}

bool TextureFactory::AddTexture(const String& name, const TextureDescription& desc)
{
    if (FindTexture(name) >= 0)
    {
        return false;
    }

    textureDescs_.Push(MakePair(name, desc));
    return true;
}

void TextureFactory::RemoveAllTextures()
{
    textureDescs_.Clear();
}

void TextureFactory::AddOutput(const String& name, const String& fileName)
{
    outputs_.Push(MakePair(name, fileName));
}

void TextureFactory::RemoveAllOutputs()
{
    outputs_.Clear();
}

bool TextureFactory::CheckAllOutputs(const String& outputDirectory) const
{
    for (const Pair<String, String>& outputDesc : outputs_)
    {
        if (!resourceCache_->Exists(outputDirectory + outputDesc.second_))
        {
            return false;
        }
    }
    return true;
}

bool TextureFactory::Generate(const String& outputDirectory)
{
    if (!resourceCache_)
    {
        return false;
    }

    // Generate textures
    for (const Pair<String, TextureDescription>& textureDesc : textureDescs_)
    {
        SharedPtr<Texture2D> texture = RenderTexture(context_, textureDesc.second_, textureMap_);
        if (!texture)
        {
            URHO3D_LOGERRORF("Cannot generate texture '%s'", textureDesc.first_);
            return false;
        }
        textureMap_.Populate(textureDesc.first_, texture);
    }

    // Save textures
    for (const Pair<String, String>& outputDesc : outputs_)
    {
        if (!textureMap_.Contains(outputDesc.first_))
        {
            URHO3D_LOGERRORF("Cannot find procedural texture with internal name '%s'", outputDesc.first_);
            return false;
        }

        const SharedPtr<Texture2D> texture = textureMap_[outputDesc.first_];
        const String outputName = outputDesc.second_;

        // Save
        const String& outputFileName = outputDirectory + outputName;
        CreateDirectoriesToFile(*resourceCache_, outputFileName);
        const SharedPtr<Image> image = ConvertTextureToImage(texture);
        if (image->SavePNG(outputFileName))
        {
            resourceCache_->ReloadResourceWithDependencies(outputName);
        }
        else
        {
            URHO3D_LOGERRORF("Cannot save texture to '%s'", outputFileName.CString());
        }
    }
    return true;
}

int TextureFactory::FindTexture(const String& name) const
{
    int idx = 0;
    for (const Pair<String, TextureDescription>& texture : textureDescs_)
    {
        if (name.Compare(texture.first_, false) == 0)
        {
            return idx;
        }
        ++idx;
    }
    return -1;
}

}
