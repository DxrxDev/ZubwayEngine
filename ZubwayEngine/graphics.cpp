#include "graphics.hpp"
#include "error.hpp"

#include <vulkan/vulkan.h>

#include <optional>
#include <limits>
#include <fstream>
#include <cstring>


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

    for (int32_t i = 0; i < memProperties.memoryTypeCount; ++i){
        if ((filter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    Error() << "Couldn't find memory type for buffer :/";
    return 0;
}

VertexBuffer::VertexBuffer( GraphicsWindow *wnd, std::vector<Vertex>& verts ){
    window = wnd;
    verticies = verts;

    Buffer vertStageBuffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(verticies[0]) * verticies.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );

    void* mappeddata;
    vkMapMemory(window->GetDevice(), vertStageBuffer.GetMemory(), 0, vertStageBuffer.GetSizeOnGPU(), 0, &mappeddata);
    memcpy(mappeddata, verticies.data(), vertStageBuffer.GetSizeOnGPU());
    vkUnmapMemory(window->GetDevice(), vertStageBuffer.GetMemory());

    vertBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(verticies[0]) * verticies.size(),
        (
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
        ),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    VkCommandBuffer command;
    VkCommandBufferAllocateInfo cbAI = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = wnd->GetCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(window->GetDevice(), &cbAI, &command);

    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(command, &cbbi);

    VkBufferCopy cbcp = {
        0, 0,
        vertStageBuffer.GetSizeOnCPU(),
    };
    vkCmdCopyBuffer(
        command, 
        vertStageBuffer.GetBuffer(), vertBuffer->GetBuffer(),
        1, &cbcp
    );

    vkEndCommandBuffer(command);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &command,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(window->GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(window->GetGraphicsQueue());
}
VertexBuffer::~VertexBuffer( ){
    delete vertBuffer;
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

IndexBuffer::IndexBuffer( GraphicsWindow *wnd, uint16_t *data, size_t numI ) {
    window = wnd;
    dind = (uint16_t *)malloc( sizeof(uint16_t) * numI );
    nind = numI;
    memcpy(dind, data, sizeof(uint16_t) * numI);

    Buffer indStageBuffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(dind[0]) * nind,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );

    void* mappeddata;
    vkMapMemory(window->GetDevice(), indStageBuffer.GetMemory(), 0, indStageBuffer.GetSizeOnGPU(), 0, &mappeddata);
    memcpy(mappeddata, dind, indStageBuffer.GetSizeOnGPU());
    vkUnmapMemory(window->GetDevice(), indStageBuffer.GetMemory());

    iBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        sizeof(dind[0]) * nind,
        (
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
        ),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    VkCommandBuffer command;
    VkCommandBufferAllocateInfo cbAI = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = wnd->GetCommandPool(),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(window->GetDevice(), &cbAI, &command);

    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = 0,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = nullptr
    };
    vkBeginCommandBuffer(command, &cbbi);

    VkBufferCopy cbcp = {
        0, 0,
        indStageBuffer.GetSizeOnCPU(),
    };
    vkCmdCopyBuffer(
        command, 
        indStageBuffer.GetBuffer(), iBuffer->GetBuffer(),
        1, &cbcp
    );

    vkEndCommandBuffer(command);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &command,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(window->GetGraphicsQueue(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(window->GetGraphicsQueue());
}
IndexBuffer::~IndexBuffer(){
    free( dind );
}
int32_t IndexBuffer::BindBuffer( VkCommandBuffer cmd ){
    vkCmdBindIndexBuffer(cmd, iBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);
    return 0;
}
uint32_t IndexBuffer::GetIndCount( void ){
    return nind;
}


UniformBuffer1::UniformBuffer1( GraphicsWindow *wnd, MVP mvp ){
    window = wnd;
    this->mvp = mvp;

    uBuffer = new Buffer(
        wnd->GetDevice(), wnd->GetPhysicalDevice(),
        MVP::GetSize(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        (
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    );
    UploadMVP();

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
        Error() << "Couldn't allocate descriptor sets!";
    }

    VkDescriptorBufferInfo bufferInfo = {
        .buffer = uBuffer->GetBuffer(),
        .offset = 0,
        .range = MVP::GetSize()
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
UniformBuffer1::~UniformBuffer1( void ){
    delete uBuffer;
}
int32_t UniformBuffer1::BindBuffer( VkCommandBuffer cmd ){
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
void UniformBuffer1::UpdateMVP( MVP mvp ){
    this->mvp = mvp;
    UploadMVP( );
}
void UniformBuffer1::UploadMVP( void ){
    void* mappeddata;
    vkMapMemory(window->GetDevice(), uBuffer->GetMemory(), 0, uBuffer->GetSizeOnGPU(), 0, &mappeddata);
    memcpy(mappeddata, &mvp, uBuffer->GetSizeOnGPU());
    vkUnmapMemory(window->GetDevice(), uBuffer->GetMemory());
    // free( mappeddata );
}

UniformBuffer2::UniformBuffer2( GraphicsWindow *wnd, uint32_t capacity ){
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
        Error() << "Couldn't allocate descriptor sets!";
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
UniformBuffer2::~UniformBuffer2( void ){
    delete uBuffer;
}
int32_t UniformBuffer2::BindBuffer( VkCommandBuffer cmd ){
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
void UniformBuffer2::UpdateMVP( MVP *mvps, uint32_t nmvp, uint32_t offset ){
    void* mappeddata;
    vkMapMemory(window->GetDevice(), uBuffer->GetMemory(), 0, uBuffer->GetSizeOnGPU(), 0, &mappeddata);

    MVP *copyAt = (MVP *)(mappeddata) + offset;
    memcpy(copyAt, mvps, MVP::GetSize() * nmvp);
    
    vkUnmapMemory(window->GetDevice(), uBuffer->GetMemory());
    // free( mappeddata );
}
void UniformBuffer2::UploadMVP( void ){
}


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
        .queueCreateInfoCount = deviceQueueCreateInfos.size(),
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
    vkDestroyPipeline( device, pipeline, nullptr );

    vkDestroySurfaceKHR( vk_inst, surface, nullptr );
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
    return pipeline;
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
        .queueFamilyIndexCount = queueFamiliesUsed.size(),
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
    VkAttachmentDescription colorAttachment = {
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
    	colorAttachmentRef.attachment = 0, // Index of the color attachment
    	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

    VkSubpassDescription subpass = {
    	.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    	.colorAttachmentCount = 1,
    	.pColorAttachments = &colorAttachmentRef
	};

    VkSubpassDependency dependency = {
    	.srcSubpass = VK_SUBPASS_EXTERNAL,
    	.dstSubpass = 0,
    	.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    	.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    	.srcAccessMask = 0,
    	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

    VkRenderPassCreateInfo renderPassInfo = {
    	.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
    	.attachmentCount = 1,
    	.pAttachments = &colorAttachment,
    	.subpassCount = 1,
    	.pSubpasses = &subpass,
    	.dependencyCount = 1,
    	.pDependencies = &dependency
	};

    vkCreateRenderPass(device, &renderPassInfo, nullptr, &rp);

    frames = std::vector<VkFramebuffer>(images.size());
    for (uint32_t i = 0; i < frames.size(); ++i){
        VkFramebufferCreateInfo fci = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = rp,
            .attachmentCount = 1,
            .pAttachments = images.data() + i,
            .width = ex.width,
            .height = ex.height,
            .layers = 1
        };

        vkCreateFramebuffer(device, &fci, nullptr, frames.data() + i);
    }

    return 0;
}
int32_t GraphicsWindow::CreateUniformDescriptors( void ){
    VkDescriptorSetLayoutBinding dslb = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo ulCI = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &dslb
    };
    if (vkCreateDescriptorSetLayout(device, &ulCI, nullptr, &uniformLayout) != VK_SUCCESS){
        Error() << "Couldn't create descriptor set layout!";
    }

    VkDescriptorPoolSize poolsize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1
    };
    VkDescriptorPoolCreateInfo poolCI = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolsize,
    };
    if (vkCreateDescriptorPool(device, &poolCI, nullptr, &uniformPool) != VK_SUCCESS){
        Error() << "Couldn't create descriptor pool!";
    }
    return 0;
}
int32_t GraphicsWindow::CreatePipeline( void ){
    std::vector<char> shaderSources[2];
    {
        std::ifstream vfile( "vert.out", std::ios::ate | std::ios::binary );
        if (!vfile.is_open())
            Error() << "Couldnt open vertex shader !!";
        shaderSources[0] = std::vector<char>(vfile.tellg());
        vfile.seekg(0);
        vfile.read(shaderSources[0].data(), shaderSources[0].size());
        vfile.close();

        std::ifstream ffile( "frag.out", std::ios::ate | std::ios::binary );
        if (!ffile.is_open())
            Error() << "Couldnt open fragment shader !!";
        shaderSources[1] = std::vector<char>(ffile.tellg());
        ffile.seekg(0);
        ffile.read(shaderSources[1].data(), shaderSources[1].size());
        ffile.close();
    }

    VkShaderModule shaderModules[2];
    {
        VkShaderModuleCreateInfo shaderModulesCI[2] = {
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
            }
        };

        vkCreateShaderModule(device, shaderModulesCI + 0, nullptr, shaderModules + 0);
        vkCreateShaderModule(device, shaderModulesCI + 1, nullptr, shaderModules + 1);
    }

    VkPipelineShaderStageCreateInfo shaderCreateInfos[2] = {
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
        }
    };

    std::vector<VkVertexInputBindingDescription> bindingDesc = Vertex::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDesc = Vertex::GetAttributeDescription();
    VkPipelineVertexInputStateCreateInfo plVertexInputCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = bindingDesc.size(),
        .pVertexBindingDescriptions = bindingDesc.data(), 
        .vertexAttributeDescriptionCount = attributeDesc.size(),
        .pVertexAttributeDescriptions = attributeDesc.data()
    };

    VkPipelineInputAssemblyStateCreateInfo plInputAssemblyCI = {
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

    VkPipelineColorBlendAttachmentState colBlendAttachment = {
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
    };
    VkPipelineColorBlendStateCreateInfo plColorBlendingCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_NO_OP,
        .attachmentCount = 1,
        .pAttachments = &colBlendAttachment,
        .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f}
    };

    
    
    VkPipelineLayoutCreateInfo plci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &uniformLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    vkCreatePipelineLayout( device, &plci, nullptr, &pipelineLayout );


    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
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
        .pMultisampleState = &multisampleCI, // OPTIONAL
        .pDepthStencilState = nullptr, // OPTIONAL
        .pColorBlendState = &plColorBlendingCI,
        .layout = pipelineLayout,
        .renderPass = rp,
        .subpass = 0
    };

    vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE,
        1, &pipelineCreateInfo, nullptr, &pipeline
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

    VkClearValue clearVal = {{{0, 0, 0, 1}}};
    VkRenderPassBeginInfo rpbi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = rp,
        .framebuffer = frames[image],
        .renderArea = {
            .offset = {0, 0},
            .extent = GetExtent2()
        },
        .clearValueCount = 1,
        .pClearValues = &clearVal
    };
    vkCmdBeginRenderPass( presentbuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( presentbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
}
void GraphicsWindow::EndRenderPassCommand( VkCommandBuffer presentbuffer ){
    vkCmdEndRenderPass(presentbuffer);
    vkEndCommandBuffer(presentbuffer);
}

int32_t GraphicsWindow::Draw( VertexBuffer& vb ){
    vkWaitForFences( device, 1, &fense_inFlight, VK_TRUE, UINT64_MAX );
    vkResetFences( device, 1, &fense_inFlight );

    uint32_t imageIndex;
    vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, semaphore_imageGrabbed, VK_NULL_HANDLE, &imageIndex );

    Command cmd( device, cmdpool );

    BeginRenderPassCommand( cmd.GetCmd(), imageIndex );
        vb.BindBuffer( cmd.GetCmd() );
        vkCmdDraw(cmd.GetCmd(), vb.GetVertCount(), 1, 0, 0);
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
int32_t GraphicsWindow::DrawIndexed( VertexBuffer& vb, IndexBuffer& ib, std::vector<UniformBuffer1*> ubs ){
    vkWaitForFences( device, 1, &fense_inFlight, VK_TRUE, UINT64_MAX );
    vkResetFences( device, 1, &fense_inFlight );

    Command cmd( device, cmdpool );

    uint32_t imageIndex;
    vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, semaphore_imageGrabbed, VK_NULL_HANDLE, &imageIndex );

    BeginRenderPassCommand( cmd.GetCmd(), imageIndex );
    vb.BindBuffer( cmd.GetCmd() );
    ib.BindBuffer( cmd.GetCmd() );

    for (uint32_t i = 0; i < ubs.size(); ++i){
        ubs[i]->BindBuffer( cmd.GetCmd() );
        vkCmdDrawIndexed(cmd.GetCmd(), ib.GetIndCount(), 1, 0, 0, 0);
    }

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
int32_t GraphicsWindow::DrawIndexed2( VertexBuffer& vb, IndexBuffer& ib, UniformBuffer2& ub ){
    vkWaitForFences( device, 1, &fense_inFlight, VK_TRUE, UINT64_MAX );
    vkResetFences( device, 1, &fense_inFlight );

    Command cmd( device, cmdpool );

    uint32_t imageIndex;
    vkAcquireNextImageKHR( device, swapchain, UINT64_MAX, semaphore_imageGrabbed, VK_NULL_HANDLE, &imageIndex );

    BeginRenderPassCommand( cmd.GetCmd(), imageIndex );
    vb.BindBuffer( cmd.GetCmd() );
    ib.BindBuffer( cmd.GetCmd() );
    ub.BindBuffer( cmd.GetCmd() );

    vkCmdDrawIndexed(cmd.GetCmd(), ib.GetIndCount(), 1, 0, 0, 0);
    

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