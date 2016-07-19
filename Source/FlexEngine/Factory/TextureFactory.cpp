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
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/View.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

namespace FlexEngine
{

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

Vector<SharedPtr<Image>> RenderStaticModel(Context* context, SharedPtr<Model> model, const Vector<SharedPtr<Material>>& materials,
    unsigned width, unsigned height, const Vector<SharedPtr<XMLFile>>& renderPaths, const Vector<OrthoCameraDescription>& cameras)
{
    Vector<SharedPtr<Image>> images;
    for (XMLFile* renderPath : renderPaths)
    {
        // Prepare views
        Vector<ViewDescription> views;
        for (const OrthoCameraDescription& cameraDesc : cameras)
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

            // Setup model
            StaticModel* staticModel = viewDesc.node_->CreateComponent<StaticModel>();
            if (staticModel)
            {
                staticModel->SetModel(model);
                for (unsigned i = 0; i < materials.Size(); ++i)
                {
                    staticModel->SetMaterial(i, materials[i]);
                }
            }

            // Setup other fields
            viewDesc.viewport_ = cameraDesc.viewport_;
            viewDesc.renderPath_ = renderPath;

            views.Push(viewDesc);
        }

        // Render views and save image
        const SharedPtr<Texture2D> texture = RenderViews(context, width, height, views);
        const SharedPtr<Image> image = ConvertTextureToImage(texture);
        if (image)
        {
            images.Push(image);
        }
    }
    return images;
}

void GenerateTexturesFromXML(XMLElement& node, ResourceCache& resourceCache, const FactoryContext& factoryContext)
{
    // Load size
    const XMLElement outputNode = node.GetChild("output");
    const String widthValue = outputNode.GetChild("width").GetValue();
    const String heightValue = outputNode.GetChild("height").GetValue();
    const unsigned width = To<unsigned>(widthValue);
    const unsigned height = To<unsigned>(heightValue);

    if (width == 0)
    {
        URHO3D_LOGERRORF("Procedural texture node <output/width> has unexpected value '%s'", widthValue.CString());
        return;
    }

    if (height == 0)
    {
        URHO3D_LOGERRORF("Procedural texture node <output/height> has unexpected value '%s'", heightValue.CString());
        return;
    }

    // Load outputs
    Vector<String> outputNames;
    Vector<SharedPtr<XMLFile>> renderPaths;
    for (XMLElement textureNode = outputNode.GetChild("texture"); textureNode; textureNode = textureNode.GetNext("texture"))
    {
        const String renderPathName = textureNode.GetAttribute("renderpath");
        const String outputName = factoryContext.SanitateName(textureNode.GetValue());
        const SharedPtr<XMLFile> renderPath(resourceCache.GetResource<XMLFile>(renderPathName));
        if (!renderPath)
        {
            URHO3D_LOGERRORF("Procedural texture render path '%s' was not found", renderPathName.CString());
            return;
        }

        renderPaths.Push(renderPath);
        outputNames.Push(outputName);
    }

    // Skip generation if possible
    bool alreadyGenerated = true;
    for (const String& outputName : outputNames)
    {
        if (!resourceCache.GetFile(outputName))
        {
            alreadyGenerated = false;
            break;
        }
    }

    if (!factoryContext.forceGeneration_ && alreadyGenerated)
    {
        return;
    }

    // Load source
    const XMLElement sourceNode = node.GetChild("source");
    if (!sourceNode)
    {
        URHO3D_LOGERROR("Procedural texture must have <source> node");
        return;
    }

    // Load model
    const String modelName = factoryContext.SanitateName(sourceNode.GetChild("model").GetValue());
    if (modelName.Empty())
    {
        URHO3D_LOGERROR("Procedural texture must have <source/model> node");
        return;
    }

    const SharedPtr<Model> model(resourceCache.GetResource<Model>(modelName));
    if (!model)
    {
        URHO3D_LOGERRORF("Procedural texture source model '%s' was not found", modelName.CString());
        return;
    }

    // Load materials
    const XMLElement materialsNode = sourceNode.GetChild("materials");
    Vector<SharedPtr<Material>> materials;
    for (XMLElement materialNode = materialsNode.GetChild("material"); materialNode; materialNode = materialNode.GetNext("material"))
    {
        const String materialName = factoryContext.SanitateName(materialNode.GetValue());
        const SharedPtr<Material> material(resourceCache.GetResource<Material>(materialName));
        if (!material)
        {
            URHO3D_LOGERRORF("Procedural texture source material '%s' was not found", materialName.CString());
            return;
        }

        materials.Push(material);
    }

    // Load proxy geometry type
    const XMLElement patternNode = node.GetChild("pattern");
    const Vector<OrthoCameraDescription> cameras = GenerateProxyCamerasFromXML(model->GetBoundingBox(), width, height, patternNode);

    // Render
    const Vector<SharedPtr<Image>> images = RenderStaticModel(
        resourceCache.GetContext(), model, materials, width, height, renderPaths, cameras);

    // Save
    assert(outputNames.Size() == images.Size());
    for (unsigned i = 0; i < outputNames.Size(); ++i)
    {
        // Save
        const String& outputFileName = factoryContext.outputDirectory_ + outputNames[i];
        CreateDirectoriesToFile(resourceCache, outputFileName);
        images[i]->SavePNG(outputFileName);

        // Reload
        SharedPtr<Texture2D> resource(resourceCache.GetResource<Texture2D>(outputNames[i]));
        resourceCache.ReloadResource(resource);
    }
}

}
