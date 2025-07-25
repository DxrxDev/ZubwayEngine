#include <vulkan/vulkan_core.h>
#if !defined(__GRAPHICS_H)
#define      __GRAPHICS_H

#include "raymath.h"
#include "error.hpp"
#include "window.hpp"
#include "box2D.hpp"
#include <vulkan/vulkan.h>

#include <vector>
#include <array>
#include <utility>

struct GraphicsSystemInfo;
struct Vertex;
class  GraphicsWindow;
class  VertexBuffer;
class  VulkanImage;

struct GraphicsSystemInfo{
    VkInstance instance;
//    VkDevice   device;
    uint32_t   graphics_i, present_i;
    VkQueue    graphics,   present;

    VkSharingMode sharingMode;
    VkSurfaceFormatKHR format;
};

struct Vertex{
    Vector3 pos;
    Vector2 texcoord;
    uint32_t trsn;

    static std::vector<VkVertexInputBindingDescription> GetBindingDescription( void ){
        return {
            {
                .binding   = 0,
                .stride    = sizeof(Vector3) + sizeof(Vector2) + sizeof(uint32_t),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };
    }
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescription( void ){
        return {
            {
                .location = 0,
                .binding  = 0,
                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                .offset   = 0
            },
            {
                .location = 1,
                .binding  = 0,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = sizeof(Vector3)
            },
            {
                .location = 2,
                .binding  = 0,
                .format   = VK_FORMAT_R32_SINT,
                .offset   = sizeof(Vector3) + sizeof(Vector2)
            }
        };
    }
};
struct VertexUI{
    Vector3 pos;
    Vector4 col;
    Vector2 texcoord;

    static std::vector<VkVertexInputBindingDescription> GetBindingDescription( void ){
        return {
            {
                .binding   = 0,
                .stride    = sizeof(Vector3) + sizeof(Vector4) + sizeof(Vector2),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };
    }
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescription( void ){
        return {
            {
                .location = 0,
                .binding  = 0,
                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                .offset   = 0
            },
            {
                .location = 1,
                .binding  = 0,
                .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset   = sizeof(Vector3)
            },
            {
                .location = 2,
                .binding  = 0,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = sizeof(Vector3) + sizeof(Vector4)
            }
        };
    }
};

struct MVP{
    Matrix model;

    static uint32_t GetSize(){
        return sizeof( Matrix ) * 1;
    }
    static VkDescriptorSet GetDesc( void ){
        return {

        };
    }
};
struct Image{
    struct Pixel {
        uint8_t r, g, b, a;
    };

    const Pixel *data;
    const uint32_t width, height; 
};

class Buffer{
public:
    Buffer( VkDevice device, VkPhysicalDevice pd, uint32_t sod, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties );
    ~Buffer();

    VkBuffer GetBuffer( void ){
        return buffer;
    }
    VkDeviceMemory GetMemory( void ){
        return bufferMemory;
    }

    uint32_t GetSizeOnCPU( void ){
        return sizeofdata;
    }
    uint32_t GetSizeOnGPU( void ){
        return sizeondevice;
    }

    static uint32_t findMemoryType( VkPhysicalDevice pd, uint32_t filter, VkMemoryPropertyFlags properties );
private:
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    
    uint32_t sizeofdata;
    uint32_t sizeondevice;
};
class VertexBuffer {
public:
    VertexBuffer( GraphicsWindow *wnd, std::vector<Vertex>& verts, uint32_t reserved );
    ~VertexBuffer( );
    const char *UpdateMemory( Vertex *verts, size_t numverts, size_t offset );
    int32_t BindBuffer( VkCommandBuffer cmd );

    uint32_t GetVertCount( void );
    uint32_t GetSizeofData( void );
private:

    GraphicsWindow *window;
    Buffer *vertBuffer;
    std::vector<Vertex> verticies;

    size_t numreserved;
};
class IndexBuffer {
public:
    IndexBuffer( GraphicsWindow *wnd, uint16_t *data, size_t numI, size_t reserved );
    ~IndexBuffer( );
    const char *UpdateMemory( uint16_t *inds, size_t numinds, size_t offset );
    int32_t BindBuffer( VkCommandBuffer cmd );

    uint32_t GetIndCount( void );
private:
    GraphicsWindow *window;
    Buffer *iBuffer;
    size_t reserved;
    size_t numInds;
};
class UIBuffer {
public:
    UIBuffer( GraphicsWindow *wnd, std::vector<VertexUI>& verts, uint32_t reserved );
    ~UIBuffer( );
    const char *UpdateMemory( VertexUI *verts, size_t numverts, size_t offset );
    int32_t BindBuffer( VkCommandBuffer cmd );

    uint32_t GetVertCount( void );
    uint32_t GetSizeofData( void );
private:

    GraphicsWindow *window;
    Buffer *vertBuffer;
    std::vector<VertexUI> verticies;

    size_t numreserved;
};

class UniformBuffer{
public:
    UniformBuffer( GraphicsWindow *wnd, uint32_t capacity );
    ~UniformBuffer( void );
    int32_t BindBuffer( VkCommandBuffer cmd );

    void UpdateMVP( MVP *mvps, uint32_t nmvp, uint32_t offset );
private:
    void UploadMVP( void );

    GraphicsWindow *window;
    Buffer *uBuffer;
    VkDescriptorSet descSet;
};
class TextureBuffer {
public:
    TextureBuffer( GraphicsWindow *wnd, Image img );
    ~TextureBuffer( );
    int32_t BindBuffer( );

private:
    GraphicsWindow *window;
    Image img;
    Buffer *tBuffer;
};

class DepthStencil {
public:
    DepthStencil( GraphicsWindow *wnd );
    ~DepthStencil();

private:
    GraphicsWindow *window;
    VkImage image;
    VkImageView imageview;
    VkDeviceMemory devicememory;
};

class Command {
public:
    Command( VkDevice device, VkCommandPool cmdpool );
    ~Command();

    void Begin();
    VkCommandBuffer GetCmd( void );
    VkCommandBuffer *GetPtr( void );
private:
    VkDevice device;
    VkCommandPool cmdpool;
    VkCommandBuffer cmd;
};

void InitGraphicsSystem( const char *appname );
GraphicsSystemInfo GetGraphicsSystem( void );

class GraphicsWindow : public Window{
public:
    GraphicsWindow( uint32_t width, uint32_t height, const char *title, CreationFlags flag );
    ~GraphicsWindow();

    int32_t DrawIndexed( std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vibuffs, UIBuffer& uib, UniformBuffer& ub, VulkanImage& vi, Matrix vp );

    int32_t PrepareDraws( void );
    int32_t PresentDraws( void );


    VkSurfaceKHR               GetSurface( void );
    VkDevice                   GetDevice( void );
    VkPhysicalDevice           GetPhysicalDevice( void );
    std::vector<VkImageView>   GetImageViews( void );
    VkSwapchainKHR             GetSwapchain( void );
    VkRenderPass               GetRenderPass( void );
	std::vector<VkFramebuffer> GetFrameBuffers( void );
    VkDescriptorSetLayout      GetDescriptorLayout( void ){
        return uniformLayout;
    }
    VkDescriptorSetLayout      GetDescriptorLayout2( void ){
        return textureLayout;
    }
    VkDescriptorSetLayout      GetDescriptorLayout3( void ){
        return uitextureLayout;
    }    
    VkDescriptorPool           GetDescriptorPool( void ){
        return uniformPool;
    }
    VkPipelineLayout           GetPipelineLayout( void ){
        return pipelinelayouts[0];
    }
    VkPipelineLayout           GetPipelineLayout2( void ){
        return pipelinelayouts[1];
    }
    VkPipeline                 GetPipeline( void );
    VkCommandPool              GetCommandPool( void );
    VkQueue                    GetGraphicsQueue( void );
    uint32_t                   GetGraphicsQueueFamily( void ){
        return graphicsQueueFamily;
    }

    VkExtent2D GetExtent( void );
    VkExtent2D GetExtent2( void );
private:
    int32_t CreateSwapchain( void );
    int32_t CreateRenderPassAndFrameBuffers( void );
    int32_t CreateUniformDescriptors( void );
    int32_t CreatePipeline( void );
    int32_t CreateCommandPoolAndBuffer( void );
    int32_t CreateSyncVariables( void );

    void BeginRenderPassCommand( VkCommandBuffer presentbuffer, int32_t image );
    void EndRenderPassCommand( VkCommandBuffer presentbuffer );

    std::vector<uint32_t>      queueFamiliesUsed;
    VkSurfaceKHR               surface;
    VkDevice                   device;
    VkPhysicalDevice           physicalDevice;

    uint32_t                   graphicsQueueFamily, presentQueueFamily;
    VkQueue                    graphicsQueue, presentQueue;
    
    VkSwapchainKHR             swapchain;
    std::vector<VkImageView>   images;
    VkRenderPass               rp;
    std::vector<VkFramebuffer> frames;
    uint32_t                   drawingOn;
    
    VkImage                    dsimage;
    VkImageView                dsimageview;
    VkDeviceMemory             dsmemory;

    VkDescriptorSetLayout      uniformLayout;
    VkDescriptorSetLayout      textureLayout;
    VkDescriptorSetLayout      uitextureLayout;
    VkDescriptorPool           uniformPool;
    
    VkPipelineLayout           pipelinelayouts[2];
    VkPipeline                 pipelines[2];
    VkCommandPool              cmdpool;
    
    VkSemaphore                semaphore_imageGrabbed;
    VkSemaphore                semaphore_renderDone;
    VkFence                    fense_inFlight;

    UniformBuffer *ub;

    VkExtent2D ex;
};

class VulkanImage{
public:
    VulkanImage(GraphicsWindow *window, Image& worldimg, Image& uiimg){
        wnd = window;

        uint32_t sizeoftex[2] = {
            worldimg.width * worldimg.height * 4,
            uiimg.width * uiimg.height * 4
        };

        uint32_t queues[1] = { wnd->GetGraphicsQueueFamily() };
        VkImageCreateInfo ici = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent = {.width = worldimg.width, .height = worldimg.height, .depth = 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE, // WILL DEP ON COMP
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = queues,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };
        vkCreateImage(wnd->GetDevice(), &ici, nullptr, &img[0]);
        ici.extent = {.width = uiimg.width, .height = uiimg.height, .depth = 1};
        vkCreateImage(wnd->GetDevice(), &ici, nullptr, &img[1]);

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements( wnd->GetDevice(), img[0], &memRequirements);
        VkMemoryAllocateInfo mai = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = Buffer::findMemoryType(
                wnd->GetPhysicalDevice(),
                memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            )
        };
        vkAllocateMemory( wnd->GetDevice(), &mai, nullptr, &imgM[0] );
        vkBindImageMemory( wnd->GetDevice(), img[0], imgM[0], 0 );

        vkGetImageMemoryRequirements( wnd->GetDevice(), img[1], &memRequirements);
        mai.allocationSize = memRequirements.size;
        mai.memoryTypeIndex = Buffer::findMemoryType(
            wnd->GetPhysicalDevice(),
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        vkAllocateMemory( wnd->GetDevice(), &mai, nullptr, &imgM[1] );
        vkBindImageMemory( wnd->GetDevice(), img[1], imgM[1], 0 );


        VkImageViewCreateInfo ivci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = img[0],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCreateImageView( wnd->GetDevice(), &ivci, nullptr, &imgV[0] );

        ivci.image = img[1];
        vkCreateImageView( wnd->GetDevice(), &ivci, nullptr, &imgV[1] );


        Buffer staging(
            wnd->GetDevice(),
            wnd->GetPhysicalDevice(),
            sizeoftex[0],
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            (
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );
        Buffer staging2(
            wnd->GetDevice(),
            wnd->GetPhysicalDevice(),
            sizeoftex[1],
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            (
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );

        Image::Pixel *texData;
        vkMapMemory(wnd->GetDevice(), staging.GetMemory(), 0, sizeoftex[0], 0, (void **)&texData);

        Image::Pixel *texData2;
        vkMapMemory(wnd->GetDevice(), staging2.GetMemory(), 0, sizeoftex[1], 0, (void **)&texData2);


        for (uint32_t y = 0; y < worldimg.height; ++y){
            for (uint32_t x = 0; x < worldimg.width; ++x){
                texData[(y * worldimg.width) + x] = worldimg.data[(y * worldimg.width) + x];
            }
        }
        vkUnmapMemory(wnd->GetDevice(), staging.GetMemory());

        for (uint32_t y = 0; y < uiimg.height; ++y){
            for (uint32_t x = 0; x < uiimg.width; ++x){
                texData2[(y * uiimg.width) + x] = uiimg.data[(y * uiimg.width) + x];
            }
        }
        vkUnmapMemory(wnd->GetDevice(), staging2.GetMemory());

        Command transferDataCommand(wnd->GetDevice(), wnd->GetCommandPool());
        transferDataCommand.Begin();

        VkImageMemoryBarrier imb = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Ignoring anything on image
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Not swapping queue
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // ownership of img
            .image = img[0],
            .subresourceRange = (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(
            transferDataCommand.GetCmd(),
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imb
        );

        VkBufferImageCopy bic = {
            .bufferOffset = 0,
            .bufferRowLength = 0,   // ASSUME THE EXTENT
            .bufferImageHeight = 0, // IS THE TEXTURE SIZE
            .imageSubresource = (VkImageSubresourceLayers){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = 0,
            .imageExtent = (VkExtent3D){
                .width = worldimg.width,
                .height = worldimg.height,
                .depth = 1
            }
        };
        vkCmdCopyBufferToImage(
            transferDataCommand.GetCmd(),
            staging.GetBuffer(),
            img[0],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &bic
        );
        // bic.imageExtent = (VkExtent3D){
        //     uiimg.width, uiimg.height, 1
        // };
        // vkCmdCopyBufferToImage(
        //     transferDataCommand.GetCmd(),
        //     staging2.GetBuffer(),
        //     img[1],
        //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //     1,
        //     &bic
        // );

        VkImageMemoryBarrier imb2 = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Not swapping queue
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // ownership of img
            .image = img[0],
            .subresourceRange = (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vkCmdPipelineBarrier(
            transferDataCommand.GetCmd(),
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imb2
        );

        vkEndCommandBuffer(transferDataCommand.GetCmd());
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = transferDataCommand.GetPtr(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        vkQueueSubmit(wnd->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);

        struct VkSamplerCreateInfo sci = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        };
        vkCreateSampler( wnd->GetDevice(), &sci, nullptr, &sampler );

        
        VkDescriptorSetLayout uniformLayouts[] = {
            wnd->GetDescriptorLayout2(), wnd->GetDescriptorLayout3()
        };
        VkDescriptorSetAllocateInfo sdAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = wnd->GetDescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = uniformLayouts
        };
        VkDescriptorSetAllocateInfo sdAI2 = sdAI;
        sdAI2.pSetLayouts = uniformLayouts+1;
        

        if (vkAllocateDescriptorSets(wnd->GetDevice(), &sdAI, descSet) != VK_SUCCESS) {
            Error() << "Couldn't allocate descriptor sets! (2)";
        }
        if (vkAllocateDescriptorSets(wnd->GetDevice(), &sdAI2, descSet+1) != VK_SUCCESS) {
            Error() << "Couldn't allocate descriptor sets! (2.1)";
        }

        VkDescriptorImageInfo samplerInfo[2] = {
            {
            .sampler = sampler,
            .imageView = imgV[0],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
            {
            .sampler = sampler,
            .imageView = imgV[1],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            },
        };

        VkWriteDescriptorSet descriptorWrite[] = {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descSet[0],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &samplerInfo[0],
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descSet[1],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &samplerInfo[1],
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            },
        };

        // Update the descriptor set to bind the uniform buffer
        vkUpdateDescriptorSets(wnd->GetDevice(), 2, descriptorWrite, 0, nullptr);
    }
    void Bind( VkCommandBuffer cmd, uint32_t img ){
        if (!(img == 0 ) && !(img == 1))
            Error() << "big ba;s";
        VkDescriptorSet desSets[1] = {
            descSet[img]
        };
        uint32_t fs = (img == 0) ? 1 : 0;
        VkPipelineLayout l = (img == 0) ? wnd->GetPipelineLayout() : wnd->GetPipelineLayout2();
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            l,
            fs,
            1, desSets,
            0, nullptr
        );
    }
private:
    GraphicsWindow *wnd;

    VkImage        img[2];
    VkImageView    imgV[2];
    VkDeviceMemory imgM[2];

    VkSampler sampler;
    VkDescriptorSet descSet[2];
};

#endif