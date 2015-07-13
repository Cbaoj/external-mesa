//
// File: vk_wsi_display.h
//
/*
** Copyright (c) 2014 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef __VK_WSI_LUNARG_H__
#define __VK_WSI_LUNARG_H__

#include "vulkan.h"

#define VK_WSI_LUNARG_REVISION             3
#define VK_WSI_LUNARG_EXTENSION_NUMBER     1

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// This macro defines INT_MAX in enumerations to force compilers to use 32 bits
// to represent them. This may or may not be necessary on some compilers. The
// option to compile it out may allow compilers that warn about missing enumerants
// in switch statements to be silenced.
// Using this macro is not needed for flag bit enums because those aren't used
// as storage type anywhere.
#define VK_MAX_ENUM(Prefix) VK_##Prefix##_MAX_ENUM = 0x7FFFFFFF

// This macro defines the BEGIN_RANGE, END_RANGE, NUM, and MAX_ENUM constants for
// the enumerations.
#define VK_ENUM_RANGE(Prefix, First, Last) \
    VK_##Prefix##_BEGIN_RANGE                               = VK_##Prefix##_##First, \
    VK_##Prefix##_END_RANGE                                 = VK_##Prefix##_##Last, \
    VK_NUM_##Prefix                                         = (VK_##Prefix##_END_RANGE - VK_##Prefix##_BEGIN_RANGE + 1), \
    VK_MAX_ENUM(Prefix)

// This is a helper macro to define the value of flag bit enum values.
#define VK_BIT(bit)     (1 << (bit))

// ------------------------------------------------------------------------------------------------
// Objects

VK_DEFINE_DISP_SUBCLASS_HANDLE(VkDisplayWSI, VkObject)
VK_DEFINE_DISP_SUBCLASS_HANDLE(VkSwapChainWSI, VkObject)

// ------------------------------------------------------------------------------------------------
// Enumeration constants

#define VK_WSI_LUNARG_ENUM(type,id)    ((type)(VK_WSI_LUNARG_EXTENSION_NUMBER * -1000 + (id)))

// Extend VkPhysicalDeviceInfoType enum with extension specific constants
#define VK_PHYSICAL_DEVICE_INFO_TYPE_DISPLAY_PROPERTIES_WSI         VK_WSI_LUNARG_ENUM(VkPhysicalDeviceInfoType, 0)
#define VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PRESENT_PROPERTIES_WSI   VK_WSI_LUNARG_ENUM(VkPhysicalDeviceInfoType, 1)

// Extend VkStructureType enum with extension specific constants
#define VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI                VK_WSI_LUNARG_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_PRESENT_INFO_WSI                          VK_WSI_LUNARG_ENUM(VkStructureType, 1)

// Extend VkImageLayout enum with extension specific constants
#define VK_IMAGE_LAYOUT_PRESENT_SOURCE_WSI                          VK_WSI_LUNARG_ENUM(VkImageLayout, 0)

// Extend VkObjectType enum for new objects
#define VK_OBJECT_TYPE_DISPLAY_WSI                                  VK_WSI_LUNARG_ENUM(VkObjectType, 0)
#define VK_OBJECT_TYPE_SWAP_CHAIN_WSI                               VK_WSI_LUNARG_ENUM(VkObjectType, 1)

// ------------------------------------------------------------------------------------------------
// Enumerations

typedef enum VkDisplayInfoTypeWSI_
{
    // Info type for vkGetDisplayInfo()
    VK_DISPLAY_INFO_TYPE_FORMAT_PROPERTIES_WSI              = 0x00000003,   // Return the VkFormat(s) supported for swap chains with the display

    VK_ENUM_RANGE(DISPLAY_INFO_TYPE, FORMAT_PROPERTIES_WSI, FORMAT_PROPERTIES_WSI)
} VkDisplayInfoTypeWSI;

typedef enum VkSwapChainInfoTypeWSI_
{
    // Info type for vkGetSwapChainInfo()
    VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI           = 0x00000000,   // Return information about the persistent images of the swapchain

    VK_ENUM_RANGE(SWAP_CHAIN_INFO_TYPE, PERSISTENT_IMAGES_WSI, PERSISTENT_IMAGES_WSI)
} VkSwapChainInfoTypeWSI;

// ------------------------------------------------------------------------------------------------
// Flags

typedef VkFlags VkSwapModeFlagsWSI;
typedef enum VkSwapModeFlagBitsWSI_
{
    VK_SWAP_MODE_FLIP_BIT_WSI                               = VK_BIT(0),
    VK_SWAP_MODE_BLIT_BIT_WSI                               = VK_BIT(1),
} VkSwapModeFlagBitsWSI;

// ------------------------------------------------------------------------------------------------
// Structures

typedef struct VkDisplayPropertiesWSI_
{
    VkDisplayWSI                            display;            // Handle of the display object
    VkExtent2D                              physicalResolution; // Max resolution for CRT?
} VkDisplayPropertiesWSI;

typedef struct VkDisplayFormatPropertiesWSI_
{
    VkFormat                                swapChainFormat;    // Format of the images of the swap chain
} VkDisplayFormatPropertiesWSI;

typedef struct VkSwapChainCreateInfoWSI_
{
    VkStructureType                         sType;              // Must be VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI
    const void*                             pNext;              // Pointer to next structure

    // TBD: It is not yet clear what the use will be for the following two
    // values.  It seems to be needed for more-global window-system handles
    // (e.g. X11 display).  If not needed for the SDK, we will drop it from
    // this extension, and from a future version of this header.
    const void*                             pNativeWindowSystemHandle; // Pointer to native window system handle
    const void*                             pNativeWindowHandle; // Pointer to native window handle

    uint32_t                                displayCount;       // Number of displays the swap chain is created for
    const VkDisplayWSI*                     pDisplays;          // displayCount number of display objects the swap chain is created for

    uint32_t                                imageCount;         // Number of images in the swap chain

    VkFormat                                imageFormat;        // Format of the images of the swap chain
    VkExtent2D                              imageExtent;        // Width and height of the images of the swap chain
    uint32_t                                imageArraySize;     // Number of layers of the images of the swap chain (needed for multi-view rendering)
    VkFlags                                 imageUsageFlags;    // Usage flags for the images of the swap chain (see VkImageUsageFlags)

    VkFlags                                 swapModeFlags;      // Allowed swap modes (see VkSwapModeFlagsWSI)
} VkSwapChainCreateInfoWSI;

typedef struct VkSwapChainImageInfoWSI_
{
    VkImage                                 image;              // Persistent swap chain image handle
    VkDeviceMemory                          memory;             // Persistent swap chain image's memory handle
} VkSwapChainImageInfoWSI;

typedef struct VkPhysicalDeviceQueuePresentPropertiesWSI_
{
    VkBool32                                supportsPresent;    // Tells whether the queue supports presenting
} VkPhysicalDeviceQueuePresentPropertiesWSI;

typedef struct VkPresentInfoWSI_
{
    VkStructureType                         sType;              // Must be VK_STRUCTURE_TYPE_PRESENT_INFO_WSI
    const void*                             pNext;              // Pointer to next structure
    VkImage                                 image;              // Image to present
    uint32_t                                flipInterval;       // Flip interval
} VkPresentInfoWSI;

// ------------------------------------------------------------------------------------------------
// Function types

typedef VkResult (VKAPI *PFN_vkGetDisplayInfoWSI)(VkDisplayWSI display, VkDisplayInfoTypeWSI infoType, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkCreateSwapChainWSI)(VkDevice device, const VkSwapChainCreateInfoWSI* pCreateInfo, VkSwapChainWSI* pSwapChain);
typedef VkResult (VKAPI *PFN_vkDestroySwapChainWSI)(VkSwapChainWSI swapChain);
typedef VkResult (VKAPI *PFN_vkGetSwapChainInfoWSI)(VkSwapChainWSI swapChain, VkSwapChainInfoTypeWSI infoType, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkQueuePresentWSI)(VkQueue queue, const VkPresentInfoWSI* pPresentInfo);

// ------------------------------------------------------------------------------------------------
// Function prototypes

#ifdef VK_PROTOTYPES

VkResult VKAPI vkGetDisplayInfoWSI(
    VkDisplayWSI                            display,
    VkDisplayInfoTypeWSI                    infoType,
    size_t*                                 pDataSize,
    void*                                   pData);

VkResult VKAPI vkCreateSwapChainWSI(
    VkDevice                                device,
    const VkSwapChainCreateInfoWSI*         pCreateInfo,
    VkSwapChainWSI*                         pSwapChain);

VkResult VKAPI vkDestroySwapChainWSI(
    VkSwapChainWSI                          swapChain);

VkResult VKAPI vkGetSwapChainInfoWSI(
    VkSwapChainWSI                          swapChain,
    VkSwapChainInfoTypeWSI                  infoType,
    size_t*                                 pDataSize,
    void*                                   pData);

VkResult VKAPI vkQueuePresentWSI(
    VkQueue                                 queue,
    const VkPresentInfoWSI*                 pPresentInfo);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VK_WSI_LUNARG_H__
