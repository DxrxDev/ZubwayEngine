#include "graphics.hpp"
#include "error.hpp"

#include <cstdint>
#include <vulkan/vulkan.h>

#include <optional>
#include <limits>
#include <fstream>
#include <cstring>
#include <cmath>
#include <vulkan/vulkan_core.h>

#if defined( __linux__ )
    #include <xcb/xcb.h>
    #include <vulkan/vulkan_xcb.h>
    #define SURFACE_PLATFORM_VK_EXTENSION VK_KHR_XCB_SURFACE_EXTENSION_NAME
#else
    #error "Oppes!!"
#endif

VkInstance vk_inst;
//VkDevice vk_device;
uint32_t vk_graphics_i, vk_present_i;
VkQueue vk_graphics, vk_present;

VkSharingMode vk_sharingMode;
VkSurfaceFormatKHR vk_format;

bool CheckSuitabilityOfDevice( VkPhysicalDevice dev, VkSurfaceKHR sur, uint32_t *graphics, uint32_t *present );

void InitGraphicsSystem( const char *appname ){
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = appname,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "zubway_engine",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = (const char*[]){"VK_LAYER_KHRONOS_validation"},
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = (const char*[]){
            VK_KHR_SURFACE_EXTENSION_NAME,
            SURFACE_PLATFORM_VK_EXTENSION
        }
    };
    if (vkCreateInstance(&createInfo, NULL, &vk_inst) != VK_SUCCESS){
        Error() << "Couldn't Create Instance!!";
    }


}

GraphicsSystemInfo GetGraphicsSystem( ){
    return {
        vk_inst,
//        vk_device,
        vk_graphics_i, vk_present_i,
        vk_graphics, vk_present,

        vk_sharingMode,
        vk_format
    };
}

Buffer::Buffer( VkDevice device, VkPhysicalDevice pd, uint32_t sod, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties ){
    this->device = device;
    sizeofdata = sod;  
    
    VkBufferCreateInfo bufferCI = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeofdata,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    vkCreateBuffer( device, &bufferCI, nullptr, &buffer );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( device, buffer, &memRequirements);
    sizeondevice = memRequirements.size;

    VkMemoryAllocateInfo memoryAI = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = sizeondevice,
        .memoryTypeIndex = findMemoryType(
            pd,
            memRequirements.memoryTypeBits,
            properties
        )
    };
    if (vkAllocateMemory(device, &memoryAI, nullptr, &bufferMemory) != VK_SUCCESS) {
        buffer = VK_NULL_HANDLE;
        return;
    }
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
Buffer::~Buffer( ){
    if (buffer == VK_NULL_HANDLE)
        return;
    vkDestroyBuffer( device, buffer, nullptr );
    vkFreeMemory( device, bufferMemory, nullptr );
}
uint32_t Buffer::findMemoryType( VkPhysicalDevice pd, uint32_t filter, VkMemoryPropertyFlags properties ){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(pd, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
        if ((filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    Error() << "Couldn't find memory type for buffer :/";
    return 0;
}

VertexBuffer::VertexBuffer( GraphicsWindow *wnd, std::vector<Vertex>& verts, uint32_t reserved ){
    window = wnd;
    verticies = verts;
    numreserved = std::max<uint64_t>(verticies.size(), (uint64_t)reserved);

    vertBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(verticies[0]) * numreserved,
        (
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
        ),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (verts.size() > 0)
        UpdateMemory(verts.data(), verts.size(), 0);
}
VertexBuffer::~VertexBuffer( ){
    delete vertBuffer;
}
const char *VertexBuffer::UpdateMemory( Vertex *verts, size_t numverts, size_t offset ){
    if ((numverts + offset) > numreserved)
        return "Trying to write out of bounds!\n";
    
    Buffer vertStageBuffer(
        window->GetDevice(), window->GetPhysicalDevice(),
        sizeof(Vertex) * numverts,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );

    void* mappeddata;
    vkMapMemory(window->GetDevice(), vertStageBuffer.GetMemory(), 0, vertStageBuffer.GetSizeOnGPU(), 0, &mappeddata);
    memcpy(mappeddata, verts, vertStageBuffer.GetSizeOnGPU());
    vkUnmapMemory(window->GetDevice(), vertStageBuffer.GetMemory());

    Command cmd(window->GetDevice(), window->GetCommandPool() );
    cmd.Begin();

    VkBufferCopy cbcp = {
        0, sizeof(Vertex) * offset,
        vertStageBuffer.GetSizeOnCPU(),
    };
    vkCmdCopyBuffer(
        cmd.GetCmd(), 
        vertStageBuffer.GetBuffer(), vertBuffer->GetBuffer(),
        1, &cbcp
    );


    vkEndCommandBuffer(cmd.GetCmd());

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = cmd.GetPtr(),
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(window->GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(window->GetGraphicsQueue());

    return nullptr;
}
int32_t VertexBuffer::BindBuffer( VkCommandBuffer cmd ){
    VkBuffer buffers[] = {vertBuffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    return 0;
}
uint32_t VertexBuffer::GetVertCount( ){
    return verticies.size();
}
uint32_t VertexBuffer::GetSizeofData( void ){
    return sizeof(verticies[0]) * verticies.size();
}

IndexBuffer::IndexBuffer( GraphicsWindow *wnd, uint16_t *data, size_t numI, size_t reserved ) {
    window = wnd;
    this->reserved = std::max(numI, reserved);

    iBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(uint16_t) * this->reserved,
        (
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
        ),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    numInds = numI;
    if (numInds > 0)
        UpdateMemory( data, numI, 0 );
}
IndexBuffer::~IndexBuffer(){
}
const char *IndexBuffer::UpdateMemory( uint16_t *inds, size_t numinds, size_t offset ){
    if ((numinds + offset) > reserved)
        return "Trying to write out of bounds!\n";
    else if ((numinds + offset) > numInds){
        numInds = (numinds + offset);
    }

    Buffer indStageBuffer(
        window->GetDevice(), window->GetPhysicalDevice(),
        sizeof(uint16_t) * numinds,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );

    void* mappeddata;
    vkMapMemory(window->GetDevice(), indStageBuffer.GetMemory(), 0, indStageBuffer.GetSizeOnGPU(), 0, &mappeddata);
    memcpy(mappeddata, inds, indStageBuffer.GetSizeOnGPU());
    vkUnmapMemory(window->GetDevice(), indStageBuffer.GetMemory());

    Command cmd(window->GetDevice(), window->GetCommandPool());
    cmd.Begin();

    VkBufferCopy cbcp = {
        0, sizeof(uint16_t) * offset,
        indStageBuffer.GetSizeOnCPU(),
    };
    vkCmdCopyBuffer(
        cmd.GetCmd(), 
        indStageBuffer.GetBuffer(), iBuffer->GetBuffer(),
        1, &cbcp
    );

    vkEndCommandBuffer(cmd.GetCmd());

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = cmd.GetPtr(),
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(window->GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(window->GetGraphicsQueue());

    return nullptr;
}
int32_t IndexBuffer::BindBuffer( VkCommandBuffer cmd ){
    vkCmdBindIndexBuffer(cmd, iBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
    return 0;
}
uint32_t IndexBuffer::GetIndCount( void ){
    return numInds;
}

UIBuffer::UIBuffer( GraphicsWindow *wnd, std::vector<VertexUI>& verts, uint32_t reserved ){
    window = wnd;
    verticies = verts;
    numreserved = std::max<uint64_t>(verticies.size(), (uint64_t)reserved);

    vertBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(VertexUI) * numreserved,
        (
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
        ),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (verts.size() > 0)
        UpdateMemory(verts.data(), verts.size(), 0);
}
UIBuffer::~UIBuffer( ){
    delete vertBuffer;
}
const char *UIBuffer::UpdateMemory( VertexUI *verts, size_t numverts, size_t offset ){
    if ((numverts + offset) > numreserved)
        return "Trying to write out of bounds!\n";
    
    Buffer vertStageBuffer(
        window->GetDevice(), window->GetPhysicalDevice(),
        sizeof(VertexUI) * numverts,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );

    void* mappeddata;
    vkMapMemory(window->GetDevice(), vertStageBuffer.GetMemory(), 0, vertStageBuffer.GetSizeOnGPU(), 0, &mappeddata);
    memcpy(mappeddata, verts, vertStageBuffer.GetSizeOnGPU());
    vkUnmapMemory(window->GetDevice(), vertStageBuffer.GetMemory());

    Command cmd(window->GetDevice(), window->GetCommandPool() );
    cmd.Begin();

    VkBufferCopy cbcp = {
        0, sizeof(VertexUI) * offset,
        vertStageBuffer.GetSizeOnCPU(),
    };
    vkCmdCopyBuffer(
        cmd.GetCmd(), 
        vertStageBuffer.GetBuffer(), vertBuffer->GetBuffer(),
        1, &cbcp
    );


    vkEndCommandBuffer(cmd.GetCmd());

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = cmd.GetPtr(),
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(window->GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(window->GetGraphicsQueue());

    return nullptr;
}
int32_t UIBuffer::BindBuffer( VkCommandBuffer cmd ){
    VkBuffer buffers[] = {vertBuffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

    return 0;
}
uint32_t UIBuffer::GetVertCount( ){
    return std::max<uint32_t>( verticies.size(), numreserved );
}
uint32_t UIBuffer::GetSizeofData( void ){
    return sizeof(verticies[0]) * verticies.size();
}

/* =====(UNIFORM BUFFER)===== */

UniformBuffer::UniformBuffer( GraphicsWindow *wnd, uint32_t capacity ){
    window = wnd;
    
    uBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        MVP::GetSize() * capacity,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );

    VkDescriptorSetLayout uniformLayouts[] = {
        window->GetDescriptorLayout()
    };
    VkDescriptorSetAllocateInfo sdAI = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = window->GetDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = uniformLayouts
    };

    if (vkAllocateDescriptorSets(window->GetDevice(), &sdAI, &descSet) != VK_SUCCESS) {
        Error() << "Couldn't allocate descriptor sets! (1)";
    }

    VkDescriptorBufferInfo bufferInfo = {
        .buffer = uBuffer->GetBuffer(),
        .offset = 0,
        .range = MVP::GetSize() * capacity
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &bufferInfo,
        .pTexelBufferView = nullptr
    };

    // Update the descriptor set to bind the uniform buffer
    vkUpdateDescriptorSets(window->GetDevice(), 1, &descriptorWrite, 0, nullptr);
}
UniformBuffer::~UniformBuffer( void ){
    delete uBuffer;
}
int32_t UniformBuffer::BindBuffer( VkCommandBuffer cmd ){
    VkDescriptorSet desSets[1] = {
        descSet
    };
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        window->GetPipelineLayout(),
        0,
        1, desSets,
        0, nullptr
    );

    return 0;
}
void UniformBuffer::UpdateMVP( MVP *mvps, uint32_t nmvp, uint32_t offset ){
    void* mappeddata;
    vkMapMemory(window->GetDevice(), uBuffer->GetMemory(), 0, uBuffer->GetSizeOnGPU(), 0, &mappeddata);

    MVP *copyAt = (MVP *)(mappeddata) + offset;
    memcpy(copyAt, mvps, MVP::GetSize() * nmvp);
    
    vkUnmapMemory(window->GetDevice(), uBuffer->GetMemory());
    // free( mappeddata );
}
void UniformBuffer::UploadMVP( void ){
}

DepthStencil::DepthStencil( GraphicsWindow *wnd ){

}
DepthStencil::~DepthStencil(){

}


//TextureBuffer::TextureBuffer( GraphicsSystemInfo )

Command::Command( VkDevice device, VkCommandPool cmdpool ){
    this->device = device;
    this->cmdpool = cmdpool;
    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cmdpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers( device, &cbai, &cmd );
}
Command::~Command(){
    vkDeviceWaitIdle( device );
    vkFreeCommandBuffers( device, cmdpool, 1, &cmd );
}
void Command::Begin(){
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
}
VkCommandBuffer Command::GetCmd( void ){
    return cmd;
}
VkCommandBuffer *Command::GetPtr( void ){
    return &cmd;
}

GraphicsWindow::GraphicsWindow( uint32_t width, uint32_t height, const char *title, CreationFlags flag )
    : Window( width, height, title, flag )
{
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .connection = (xcb_connection_t *)GetWindowSystem(),
        .window = *(xcb_window_t *)GetWindowHandle()
    };

    if (vkCreateXcbSurfaceKHR( vk_inst, &surfaceCreateInfo, NULL, &this->surface ) != VK_SUCCESS){
        Error() << "Error Couldnt Create Surface!!";
    }


    uint32_t numPhysicalDevices = 0;
    vkEnumeratePhysicalDevices(vk_inst, &numPhysicalDevices, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(numPhysicalDevices);
    vkEnumeratePhysicalDevices(vk_inst, &numPhysicalDevices, physicalDevices.data());

    std::optional<uint32_t> validAt = std::nullopt;
    
    for (uint32_t i = 0; i < physicalDevices.size(); ++i){
        if (CheckSuitabilityOfDevice( physicalDevices[i], surface, &graphicsQueueFamily, &presentQueueFamily )){
            validAt = i;
            break;
        }
    }
    if (!validAt.has_value()){
        Error() << "Couldn't find a suitable device!";
    }

    vk_sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    physicalDevice = physicalDevices[validAt.value()];

    float queuePriorities[1] = {1.0f};
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos = {{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueFamily,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities
    }};
    queueFamiliesUsed = {
        graphicsQueueFamily
    };

    if (graphicsQueueFamily != presentQueueFamily){
       deviceQueueCreateInfos.push_back(
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = presentQueueFamily,
                .queueCount = 1,
                .pQueuePriorities = queuePriorities
            }
        );
        queueFamiliesUsed.push_back(presentQueueFamily);
        vk_sharingMode = VK_SHARING_MODE_CONCURRENT;
    }

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = (uint32_t)deviceQueueCreateInfos.size(),
        .pQueueCreateInfos = deviceQueueCreateInfos.data(),
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char*[]){ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
    };

    if (vkCreateDevice(
        physicalDevice,
        &deviceCreateInfo,
        nullptr,
        &device
    ) != VK_SUCCESS){
        Error() << "Couldn't create device!";
    }

    vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamily, 0, &presentQueue);

    CreateSwapchain();
    CreateRenderPassAndFrameBuffers();
    CreateUniformDescriptors( );
    CreatePipeline();
    CreateCommandPoolAndBuffer();
    CreateSyncVariables();
}

GraphicsWindow::~GraphicsWindow(){
    vkDeviceWaitIdle( device );

    vkDestroyFence( device, fense_inFlight, nullptr );
    vkDestroySemaphore( device, semaphore_imageGrabbed, nullptr );
    vkDestroySemaphore( device, semaphore_renderDone, nullptr );

    vkDestroyCommandPool( device, cmdpool, nullptr );    
    vkDestroyPipeline( device, pipelines[0], nullptr );
    vkDestroyPipeline( device, pipelines[1], nullptr );

    vkDestroySwapchainKHR( device, swapchain, nullptr );
    //vkDestroySurfaceKHR( vk_inst, surface, nullptr );
}

VkSurfaceKHR GraphicsWindow::GetSurface( ){
    return this->surface;
}
VkDevice GraphicsWindow::GetDevice( ){
    return this->device;
}
VkPhysicalDevice GraphicsWindow::GetPhysicalDevice( ){
    return physicalDevice;
}
std::vector<VkImageView> GraphicsWindow::GetImageViews( void ){
    return images;
}
VkSwapchainKHR GraphicsWindow::GetSwapchain( ){
    return swapchain;
}
VkRenderPass GraphicsWindow::GetRenderPass( void ){
    return rp;
}
std::vector<VkFramebuffer> GraphicsWindow::GetFrameBuffers( void ){
    return frames;
}
VkPipeline GraphicsWindow::GetPipeline( void ){
    return pipelines[0];
}
VkCommandPool GraphicsWindow::GetCommandPool( void ){
    return cmdpool;
}
VkQueue GraphicsWindow::GetGraphicsQueue( void ){
    return graphicsQueue;
}

VkExtent2D GraphicsWindow::GetExtent(){
    xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(
        (xcb_connection_t *)GetWindowSystem(),
        xcb_get_geometry ((xcb_connection_t *)GetWindowSystem(), *(xcb_window_t *)GetWindowHandle()), 
        NULL
    );

    VkExtent2D ret;    
    ret.width = geom->width;
    ret.height = geom->height;

    free( geom );

    return ret;
}
VkExtent2D GraphicsWindow::GetExtent2(){
    return ex;
}

int32_t GraphicsWindow::CreateSwapchain( ){
    uint32_t scMinImages;
    VkExtent2D scExtent;
    VkPresentModeKHR scPresentMode;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, GetSurface(), &surfaceCapabilities);
    scMinImages = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && scMinImages > surfaceCapabilities.maxImageCount){
        scMinImages = surfaceCapabilities.maxImageCount;
    }

    uint32_t numSurfaceFormats = 0;
    uint32_t usingSurfaceFormat = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, GetSurface(), &numSurfaceFormats, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(numSurfaceFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, GetSurface(), &numSurfaceFormats, availableFormats.data());
    for (uint32_t i = 0; i < numSurfaceFormats; ++i){
        VkSurfaceFormatKHR sformat = availableFormats[i];
        if (sformat.format == VK_FORMAT_B8G8R8A8_SRGB && sformat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            usingSurfaceFormat = i;
    }

    vk_format = availableFormats[usingSurfaceFormat];
    
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()){
        scExtent = GetExtent();
    }
    else {
        scExtent = surfaceCapabilities.currentExtent;
    }

    uint32_t numPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, GetSurface(), &numPresentModes, nullptr);
    std::vector<VkPresentModeKHR> presentModes(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, GetSurface(), &numPresentModes, presentModes.data());
    scPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    //for (VkPresentModeKHR presentPresent : presentModes){
    //    if (presentPresent == VK_PRESENT_MODE_MAILBOX_KHR)
    //        scPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    //}

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = GetSurface(),
        .minImageCount = scMinImages,
        .imageFormat = vk_format.format,
        .imageColorSpace = vk_format.colorSpace,
        .imageExtent = scExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = vk_sharingMode,
        .queueFamilyIndexCount = (uint32_t)queueFamiliesUsed.size(),
        .pQueueFamilyIndices = queueFamiliesUsed.data(),
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = scPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr
    };

    vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);

    ex = scExtent;


    uint32_t numSCImages = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &numSCImages, nullptr);
    std::vector<VkImage> scImages(numSCImages);
    vkGetSwapchainImagesKHR(device, swapchain, &numSCImages, scImages.data());

    images = std::vector<VkImageView>(numSCImages);

    for (uint32_t i = 0; i < numSCImages; ++i){
        VkImageViewCreateInfo ivci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = scImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_format.format,
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

        vkCreateImageView( device, &ivci, nullptr, images.data() + i );
    }

    return 0;
}

int32_t GraphicsWindow::CreateRenderPassAndFrameBuffers(){
    VkAttachmentDescription colorAttachmentDes = {
		.flags = 0,
    	.format = vk_format.format,
    	.samples = VK_SAMPLE_COUNT_1_BIT,
    	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    	.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
    VkAttachmentReference colorAttachmentRef = {
    	.attachment = 0, // Index of the color attachment
    	.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

    VkAttachmentDescription dsAttachmentDes = {
        .flags = 0,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference dsAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
    	.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    	.colorAttachmentCount = 1,
    	.pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &dsAttachmentRef
	};

    VkSubpassDependency dependency = {
    	.srcSubpass = VK_SUBPASS_EXTERNAL,
    	.dstSubpass = 0,
    	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    	.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    	.srcAccessMask = 0,
    	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

    VkAttachmentDescription attachmentdescriptions[] = {colorAttachmentDes, dsAttachmentDes};
    VkRenderPassCreateInfo renderPassInfo = {
    	.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
    	.attachmentCount = 2,
    	.pAttachments = attachmentdescriptions,
    	.subpassCount = 1,
    	.pSubpasses = &subpass,
    	.dependencyCount = 1,
    	.pDependencies = &dependency
	};

    vkCreateRenderPass(device, &renderPassInfo, nullptr, &rp);

    VkResult res;

    uint32_t queuefamilyindex = GetGraphicsQueueFamily();
    VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .extent = (VkExtent3D){
            .width = GetExtent2().width,
            .height = GetExtent2().height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queuefamilyindex,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    res = vkCreateImage( device, &ici, nullptr, &dsimage );
    if (res != VK_SUCCESS)
        Error() << "couldnt create image";

    VkMemoryRequirements memoryreq;
    vkGetImageMemoryRequirements( device, dsimage, &memoryreq );
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryreq.size,
        .memoryTypeIndex = Buffer::findMemoryType(
            GetPhysicalDevice(),
            memoryreq.memoryTypeBits, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        )
    };
    
    if (vkAllocateMemory( device, &allocInfo, nullptr, &dsmemory) != VK_SUCCESS) {
        Error() << "Balls";
    }
    res = vkBindImageMemory(device, dsimage, dsmemory, 0);
    if (res != VK_SUCCESS)
        Error() << "hate life";


    VkImageViewCreateInfo ivci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = dsimage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    res = vkCreateImageView( device, &ivci, nullptr, &dsimageview );
    if (res != VK_SUCCESS)
        Error() << "couldnt create image view";

    frames = std::vector<VkFramebuffer>(images.size());
    for (uint32_t i = 0; i < frames.size(); ++i){
        VkImageView views[2] = {
            *(images.data() + i),
            dsimageview
        };
        VkFramebufferCreateInfo fci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = rp,
            .attachmentCount = 2,
            .pAttachments = views,
            .width = ex.width,
            .height = ex.height,
            .layers = 1
        };

        vkCreateFramebuffer(device, &fci, nullptr, frames.data() + i);
    }

    return 0;
}
int32_t GraphicsWindow::CreateUniformDescriptors( void ){
    VkDescriptorSetLayoutBinding dslbUB = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo ulciUB = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &dslbUB
    };

    VkDescriptorSetLayoutBinding dslbS = {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo ulciS = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &dslbS
    };

    VkDescriptorSetLayoutBinding dslbS2 = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo ulciS2 = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &dslbS2
    };

    if (vkCreateDescriptorSetLayout(device, &ulciUB, nullptr, &uniformLayout) != VK_SUCCESS){
        Error() << "Couldn't create descriptor set layout! (1)";
    }
    if (vkCreateDescriptorSetLayout(device, &ulciS, nullptr, &textureLayout) != VK_SUCCESS){
        Error() << "Couldn't create descriptor set layout! (2)";
    }
    if (vkCreateDescriptorSetLayout(device, &ulciS2, nullptr, &uitextureLayout) != VK_SUCCESS){
        Error() << "Couldn't create descriptor set layout! (3)";
    }


    VkDescriptorPoolSize poolsizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1
        }
    };
    VkDescriptorPoolCreateInfo poolCI = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 3,
        .poolSizeCount = 3,
        .pPoolSizes = poolsizes,
    };
    if (vkCreateDescriptorPool(device, &poolCI, nullptr, &uniformPool) != VK_SUCCESS){
        Error() << "Couldn't create descriptor pool!";
    }
    return 0;
}
int32_t GraphicsWindow::CreatePipeline( void ){
    std::vector<char> shaderSources[4];
    {
        std::ifstream vfile( "vert.spirv", std::ios::ate | std::ios::binary );
        if (!vfile.is_open())
            Error() << "Couldnt open vertex shader !!";
        shaderSources[0] = std::vector<char>(vfile.tellg());
        vfile.seekg(0);
        vfile.read(shaderSources[0].data(), shaderSources[0].size());
        vfile.close();

        std::ifstream ffile( "frag.spirv", std::ios::ate | std::ios::binary );
        if (!ffile.is_open())
            Error() << "Couldnt open fragment shader !!";
        shaderSources[1] = std::vector<char>(ffile.tellg());
        ffile.seekg(0);
        ffile.read(shaderSources[1].data(), shaderSources[1].size());
        ffile.close();


        std::ifstream vuifile( "vertui.spirv", std::ios::ate | std::ios::binary );
        if (!vuifile.is_open())
            Error() << "Couldnt open vertex ui shader !!";
        shaderSources[2] = std::vector<char>(vuifile.tellg());
        vuifile.seekg(0);
        vuifile.read(shaderSources[2].data(), shaderSources[2].size());
        vuifile.close();

        std::ifstream fuifile( "fragui.spirv", std::ios::ate | std::ios::binary );
        if (!fuifile.is_open())
            Error() << "Couldnt open fragment ui shader !!";
        shaderSources[3] = std::vector<char>(fuifile.tellg());
        fuifile.seekg(0);
        fuifile.read(shaderSources[3].data(), shaderSources[3].size());
        fuifile.close();
    }

    VkShaderModule shaderModules[4];
    {
        VkShaderModuleCreateInfo shaderModulesCI[4] = {
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shaderSources[0].size(),
                .pCode = (const uint32_t*)shaderSources[0].data()
            },
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shaderSources[1].size(),
                .pCode = (const uint32_t*)shaderSources[1].data()
            },
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shaderSources[2].size(),
                .pCode = (const uint32_t*)shaderSources[2].data()
            },
            {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shaderSources[3].size(),
                .pCode = (const uint32_t*)shaderSources[3].data()
            }
        };

        vkCreateShaderModule(device, shaderModulesCI + 0, nullptr, shaderModules + 0);
        vkCreateShaderModule(device, shaderModulesCI + 1, nullptr, shaderModules + 1);
        vkCreateShaderModule(device, shaderModulesCI + 2, nullptr, shaderModules + 2);
        vkCreateShaderModule(device, shaderModulesCI + 3, nullptr, shaderModules + 3);

    }

    VkPipelineShaderStageCreateInfo shaderCreateInfos[4] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shaderModules[0],
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shaderModules[1],
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shaderModules[2],
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shaderModules[3],
            .pName = "main",
            .pSpecializationInfo = nullptr
        }
    };

    std::vector<VkVertexInputBindingDescription> bindingDesc = Vertex::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDesc = Vertex::GetAttributeDescription();
    VkPipelineVertexInputStateCreateInfo plVertexInputCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = (uint32_t)bindingDesc.size(),
        .pVertexBindingDescriptions = bindingDesc.data(), 
        .vertexAttributeDescriptionCount = (uint32_t)attributeDesc.size(),
        .pVertexAttributeDescriptions = attributeDesc.data()
    };

    std::vector<VkVertexInputBindingDescription> bindingDescui = VertexUI::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDescui = VertexUI::GetAttributeDescription();
    VkPipelineVertexInputStateCreateInfo uiplVertexInputCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = (uint32_t)bindingDescui.size(),
        .pVertexBindingDescriptions = bindingDescui.data(), 
        .vertexAttributeDescriptionCount = (uint32_t)attributeDescui.size(),
        .pVertexAttributeDescriptions = attributeDescui.data()
    };

    VkPipelineInputAssemblyStateCreateInfo plInputAssemblyCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    VkPipelineInputAssemblyStateCreateInfo uiplInputAssemblyCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)ex.width,
        .height = (float)ex.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = ex
    };
    VkPipelineViewportStateCreateInfo plViewportCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo plRasterizationCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f 
    };

    VkPipelineMultisampleStateCreateInfo multisampleCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo depthstencilCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_TRUE,
        .front = (VkStencilOpState){
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_REPLACE,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .compareMask = 0xff,
            .writeMask = 0xff,
            .reference = 1,
        },
        .back = {},
        .minDepthBounds = 0.f,
        .maxDepthBounds = 1.f
    };

    VkPipelineColorBlendAttachmentState colBlendAttachments[] = {
        {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = (
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT
            )
        }
    };
    VkPipelineColorBlendStateCreateInfo plColorBlendingCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_NO_OP,
        .attachmentCount = 1,
        .pAttachments = colBlendAttachments,
        .blendConstants = {}//{1.0f, 1.0f, 1.0f, 1.0f}
    };

    
    VkDescriptorSetLayout layouts[] = {
        uniformLayout, textureLayout
    };
    VkPushConstantRange pushconstants[] = {
        (VkPushConstantRange){
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = 64 // sizeof matrix (16 * sizeof(float))
        }
    };
    VkPipelineLayoutCreateInfo plci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 2,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = pushconstants
    };
    vkCreatePipelineLayout( device, &plci, nullptr, &pipelinelayouts[0] );

    VkDescriptorSetLayout layouts2[] = {
        uitextureLayout
    };
    VkPushConstantRange pushconstants2[] = {
        (VkPushConstantRange){
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(float) * 2
        }
    };
    VkPipelineLayoutCreateInfo plci2 = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = layouts2,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = pushconstants2
    };
    vkCreatePipelineLayout( device, &plci2, nullptr, &pipelinelayouts[1] );

    VkGraphicsPipelineCreateInfo pipelineCreateInfo[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2, // Vertex & fragment shaders
            .pStages = shaderCreateInfos,
            .pVertexInputState = &plVertexInputCI,
            .pInputAssemblyState = &plInputAssemblyCI,
            .pTessellationState = nullptr, // OPTIONAL
            .pViewportState = &plViewportCI,
            .pRasterizationState = &plRasterizationCI,
            .pMultisampleState = &multisampleCI,
            .pDepthStencilState = &depthstencilCI,
            .pColorBlendState = &plColorBlendingCI,
            .layout = pipelinelayouts[0],
            .renderPass = rp,
            .subpass = 0
        },
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2, // Vertex & fragment shaders
            .pStages = shaderCreateInfos+2,
            .pVertexInputState = &uiplVertexInputCI,
            .pInputAssemblyState = &uiplInputAssemblyCI,
            .pTessellationState = nullptr, // OPTIONAL
            .pViewportState = &plViewportCI,
            .pRasterizationState = &plRasterizationCI,
            .pMultisampleState = &multisampleCI,
            .pDepthStencilState = &depthstencilCI,
            .pColorBlendState = &plColorBlendingCI,
            .layout = pipelinelayouts[1],
            .renderPass = rp,
            .subpass = 0
        }
    };

    vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE,
        2, pipelineCreateInfo, nullptr, pipelines
    );

    return 0;
}
int32_t GraphicsWindow::CreateCommandPoolAndBuffer( void ){
    
    VkCommandPoolCreateInfo cpci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsQueueFamily
    };
    vkCreateCommandPool( device, &cpci, nullptr, &cmdpool );

    return 0;
}
int32_t GraphicsWindow::CreateSyncVariables( void ){
    VkSemaphoreCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    VkFenceCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    vkCreateSemaphore( device, &sci, nullptr, &semaphore_imageGrabbed );
    vkCreateSemaphore( device, &sci, nullptr, &semaphore_renderDone );
    vkCreateFence( device, &fci, nullptr, &fense_inFlight );

    return 0;
}

void GraphicsWindow::BeginRenderPassCommand( VkCommandBuffer presentbuffer, int32_t image ){
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer( presentbuffer, &cbbi );

    VkClearValue clearVal[] = {
        {.color{0, 0, 0, 1}},
        {.depthStencil{1.0, 0}}
    };
    VkRenderPassBeginInfo rpbi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = rp,
        .framebuffer = frames[image],
        .renderArea = {
            .offset = {0, 0},
            .extent = GetExtent2()
        },
        .clearValueCount = 2,
        .pClearValues = clearVal
    };
    vkCmdBeginRenderPass( presentbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE );
}
void GraphicsWindow::EndRenderPassCommand( VkCommandBuffer presentbuffer ){
    vkCmdEndRenderPass(presentbuffer);
    vkEndCommandBuffer(presentbuffer);
}

int32_t GraphicsWindow::DrawIndexed( std::vector<std::pair<VertexBuffer*, IndexBuffer*>> vibuffs, UIBuffer& uib, UniformBuffer& ub, VulkanImage& vi, Matrix vp ){
    vkWaitForFences( device, 1, &fense_inFlight, VK_TRUE, UINT64_MAX );
    vkResetFences( device, 1, &fense_inFlight );

    Command cmd( device, cmdpool );

    uint32_t imageIndex;
    vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, semaphore_imageGrabbed, VK_NULL_HANDLE, &imageIndex );

    BeginRenderPassCommand( cmd.GetCmd(), imageIndex );
    vkCmdBindPipeline( cmd.GetCmd(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[0] );
    vkCmdPushConstants( cmd.GetCmd(), pipelinelayouts[0], VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &vp);

    vi.Bind( cmd.GetCmd(), 0 );
    ub.BindBuffer( cmd.GetCmd() );
    
    for (uint64_t i = 0; i < vibuffs.size(); ++i){
        std::get<0>(vibuffs[i])->BindBuffer( cmd.GetCmd() );
        std::get<1>(vibuffs[i])->BindBuffer( cmd.GetCmd() );
        vkCmdDrawIndexed(cmd.GetCmd(), std::get<1>(vibuffs[i])->GetIndCount(), 1, 0, 0, 0);
    }

    vkCmdBindPipeline( cmd.GetCmd(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[1] );
    //float mousepos[2] = {20.0, 20.0};
    //vkCmdPushConstants( cmd.GetCmd(), pipelinelayouts[1], VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, mousepos);
    vi.Bind( cmd.GetCmd(), 1 );
    uib.BindBuffer(cmd.GetCmd());
    vkCmdDraw(cmd.GetCmd(), uib.GetVertCount(), 1, 0, 0);

    EndRenderPassCommand( cmd.GetCmd() );   

    VkSemaphore waitSemaphores[] = {semaphore_imageGrabbed};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {semaphore_renderDone};
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = cmd.GetPtr(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };
    vkQueueSubmit( graphicsQueue, 1, &submit, fense_inFlight );

    VkSwapchainKHR swapchains[] = {swapchain};
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &imageIndex,
    };

    if (vkQueuePresentKHR( presentQueue, &presentInfo ) != VK_SUCCESS){
        Error() << "Couldn't Present image :/";
    }

    return 0;
}


bool CheckSuitabilityOfDevice( VkPhysicalDevice dev, VkSurfaceKHR sur, uint32_t *graphics, uint32_t *present ){
    uint32_t g, p;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures   deviceFeatures;
    vkGetPhysicalDeviceProperties( dev, &deviceProperties );
    vkGetPhysicalDeviceFeatures( dev, &deviceFeatures );

        //std::cout << deviceProperties.deviceName << std::endl;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

    int32_t graphicsFound = false;
    VkBool32 presentFound = false;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i){
        std::cout << "queue " << i << std::endl;

        if (!graphicsFound){
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
                g = i;
                graphicsFound = true;
                    std::cout << "found gfx at " << i << std::endl;
            }
        }

        if (!presentFound){
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, sur, &presentFound);
            if (presentFound){
                p = i;
                    std::cout << "found prs at " << i << std::endl;
            }
        }

        if (graphicsFound && presentFound){
            break;
        }
    }
    if (!graphicsFound){
        return false;
    }
    if (presentFound == VK_FALSE){
        return false;
    }

    *graphics=g;
    *present =p;

    return true;
}