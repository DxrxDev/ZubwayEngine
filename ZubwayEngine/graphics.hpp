#if !defined(__GRAPHICS_H)
#define      __GRAPHICS_H

#include "raymath.h"
#include "error.hpp"
#include "window.hpp"
#include <vulkan/vulkan.h>

#include <vector>
#include <array>

struct GraphicsSystemInfo;
struct Vertex;
class  GraphicsWindow;
class  VertexBuffer;

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
    uint32_t trsn;

    static std::vector<VkVertexInputBindingDescription> GetBindingDescription( void ){
        return {
            {
                .binding   = 0,
                .stride    = sizeof(Vector3) + sizeof(uint32_t),
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
                .format   = VK_FORMAT_R32_SINT,
                .offset   = sizeof(Vector3)
            }
        };
    }
};

struct MVP{
    Matrix model, view, projection;

    static uint32_t GetSize(){
        return sizeof( Matrix ) * 3;
    }
    static VkDescriptorSet GetDesc( void ){
        return {

        };
    }
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

private:
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;
    
    uint32_t sizeofdata;
    uint32_t sizeondevice;
    uint32_t findMemoryType( VkPhysicalDevice pd, uint32_t filter, VkMemoryPropertyFlags properties );
};
class VertexBuffer {
public:
    VertexBuffer( GraphicsWindow *wnd, std::vector<Vertex>& verts );
    ~VertexBuffer( );
    int32_t BindBuffer( VkCommandBuffer cmd );

    uint32_t GetVertCount( void );
    uint32_t GetSizeofData( void );
private:
    GraphicsWindow *window;
    Buffer *vertBuffer;
    std::vector<Vertex> verticies;
};
class IndexBuffer {
public:
    IndexBuffer( GraphicsWindow *wnd, uint16_t *data, size_t numI );
    ~IndexBuffer( );
    int32_t BindBuffer( VkCommandBuffer cmd );

    uint32_t GetIndCount( void );
private:
    GraphicsWindow *window;
    Buffer *iBuffer;
    uint16_t *dind;
    size_t    nind;
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
class TextureBuffer {
    
};

class Command {
public:
    Command( VkDevice device, VkCommandPool cmdpool );
    ~Command();

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
    VkDescriptorPool           GetDescriptorPool( void ){
        return uniformPool;
    }
    VkPipelineLayout           GetPipelineLayout( void ){
        return pipelineLayout;
    }
    VkPipeline                 GetPipeline( void );
    VkCommandPool              GetCommandPool( void );
    VkQueue                    GetGraphicsQueue( void );

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

#endif