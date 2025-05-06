#if !defined(__GRAPHICS_H)
#define      __GRAPHICS_H

#include "raymath.h"
#include "error.hpp"
#include "window.hpp"
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
class UniformBuffer1 {
public:
    UniformBuffer1( GraphicsWindow *wnd, MVP mvp );
    ~UniformBuffer1( void );
    int32_t BindBuffer( VkCommandBuffer cmd );

    void UpdateMVP( MVP mvp );
private:
    void UploadMVP( void );

    GraphicsWindow *window;
    MVP mvp;
    Buffer *uBuffer;
    VkDescriptorSet descSet;
};
class UniformBuffer2 {
public:
    UniformBuffer2( GraphicsWindow *wnd, uint32_t capacity );
    ~UniformBuffer2( void );
    int32_t BindBuffer( VkCommandBuffer cmd );

    void UpdateMVP( MVP *mvps, uint32_t nmvp, uint32_t offset );
private:
    void UploadMVP( void );

    GraphicsWindow *window;
    Buffer *uBuffer;
    VkDescriptorSet descSet;
};
class UniformBuffer3{
public:
    UniformBuffer3( GraphicsWindow *wnd, uint32_t capacity );
    ~UniformBuffer3( void );
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

    int32_t Draw( VertexBuffer& vb );
    int32_t DrawIndexed( VertexBuffer& vb, IndexBuffer& ib, std::vector<UniformBuffer1*> ubs );
    int32_t DrawIndexed2( VertexBuffer& vb, IndexBuffer& ib, UniformBuffer2& ub );
    int32_t DrawIndexed3( std::vector<VertexBuffer*>& vbs, IndexBuffer& ib, UniformBuffer2& ub );
    int32_t DrawIndexed4( std::vector<VertexBuffer*>& vbs, IndexBuffer& ib, UniformBuffer2& ub, VulkanImage& vi );
    int32_t DrawIndexed5( std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vibuffs, UniformBuffer2& ub, VulkanImage& vi );
    int32_t DrawIndexed6( std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vibuffs, UniformBuffer3& ub, VulkanImage& vi, Matrix vp );

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
    VkDescriptorPool           GetDescriptorPool( void ){
        return uniformPool;
    }
    VkPipelineLayout           GetPipelineLayout( void ){
        return pipelineLayout;
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

    VkDescriptorSetLayout      uniformLayout;
    VkDescriptorSetLayout      textureLayout;
    VkDescriptorPool           uniformPool;
    
    VkPipelineLayout           pipelineLayout;
    VkPipeline                 pipeline;
    VkCommandPool              cmdpool;
    
    VkSemaphore                semaphore_imageGrabbed;
    VkSemaphore                semaphore_renderDone;
    VkFence                    fense_inFlight;

    UniformBuffer1 *ub;

    VkExtent2D ex;
};

class VulkanImage{
public:
    VulkanImage(GraphicsWindow *window, uint32_t width, uint32_t height, Image::Pixel *data){
        wnd = window;
        uint32_t sizeofTex = width * height * 4;

        uint32_t queues[1] = { wnd->GetGraphicsQueueFamily() };
        VkImageCreateInfo ici = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent = {.width = width, .height = height, .depth = 1},
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
        vkCreateImage(wnd->GetDevice(), &ici, nullptr, &img);

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements( wnd->GetDevice(), img, &memRequirements);

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
        vkAllocateMemory( wnd->GetDevice(), &mai, nullptr, &imgM );
        vkBindImageMemory( wnd->GetDevice(), img, imgM, 0 );

        VkImageViewCreateInfo ivci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = img,
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
        vkCreateImageView( wnd->GetDevice(), &ivci, nullptr, &imgV );


        Buffer staging(
            wnd->GetDevice(),
            wnd->GetPhysicalDevice(),
            sizeofTex,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            (
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            )
        );

        Image::Pixel *texData;
        vkMapMemory(wnd->GetDevice(), staging.GetMemory(), 0, sizeofTex, 0, (void **)&texData);

        for (uint32_t y = 0; y < height; ++y){
            for (uint32_t x = 0; x < width; ++x){
                texData[(y * width) + x] = data[(y * width) + x];
            }
        }

        vkUnmapMemory(wnd->GetDevice(), staging.GetMemory());

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
            .image = img,
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
                .width = width,
                .height = height,
                .depth = 1
            }
        };
        vkCmdCopyBufferToImage(
            transferDataCommand.GetCmd(),
            staging.GetBuffer(),
            img,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &bic
        );

        VkImageMemoryBarrier imb2 = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Ignoring anything on image
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // Not swapping queue
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, // ownership of img
            .image = img,
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
            wnd->GetDescriptorLayout2()
        };
        VkDescriptorSetAllocateInfo sdAI = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = wnd->GetDescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = uniformLayouts
        };

        if (vkAllocateDescriptorSets(wnd->GetDevice(), &sdAI, &descSet) != VK_SUCCESS) {
            Error() << "Couldn't allocate descriptor sets! (2)";
        }

        VkDescriptorImageInfo samplerInfo = {
            .sampler = sampler,
            .imageView = imgV,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &samplerInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };

        // Update the descriptor set to bind the uniform buffer
        vkUpdateDescriptorSets(wnd->GetDevice(), 1, &descriptorWrite, 0, nullptr);
    }
    void Bind( VkCommandBuffer cmd ){
        VkDescriptorSet desSets[1] = {
            descSet
        };
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            wnd->GetPipelineLayout(),
            1,
            1, desSets,
            0, nullptr
        );
    }
private:
    GraphicsWindow *wnd;

    VkImage        img;
    VkImageView    imgV;
    VkDeviceMemory imgM;

    VkSampler sampler;
    VkDescriptorSet descSet;
};

#endif