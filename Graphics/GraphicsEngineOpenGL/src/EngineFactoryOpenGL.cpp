/*     Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence), 
 *  contract, or otherwise, unless required by applicable law (such as deliberate 
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental, 
 *  or consequential damages of any character arising as a result of this License or 
 *  out of the use or inability to use the software (including but not limited to damages 
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and 
 *  all other commercial damages or losses), even if such Contributor has been advised 
 *  of the possibility of such damages.
 */

/// \file
/// Routines that initialize OpenGL/GLES-based engine implementation

#include "pch.h"
#include "EngineFactoryOpenGL.h"
#include "RenderDeviceGLImpl.h"
#include "DeviceContextGLImpl.h"
#include "EngineMemory.h"
#include "HLSL2GLSLConverterObject.h"
#include "EngineFactoryBase.h"

#if PLATFORM_IOS
#   include "SwapChainGLIOS.h"
#else
#   include "SwapChainGLImpl.h"
#endif

#if PLATFORM_ANDROID
#   include "RenderDeviceGLESImpl.h"
#endif

namespace Diligent
{

#if PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS || PLATFORM_LINUX || PLATFORM_MACOS
    typedef RenderDeviceGLImpl TRenderDeviceGLImpl;
    typedef SwapChainGLImpl TSwapChain;
#elif PLATFORM_ANDROID
    typedef RenderDeviceGLESImpl TRenderDeviceGLImpl;
    typedef SwapChainGLImpl TSwapChain;
#elif PLATFORM_IOS
    typedef RenderDeviceGLImpl TRenderDeviceGLImpl;
    typedef SwapChainGLIOS TSwapChain;
#else
#   error Unsupported platform
#endif

/// Engine factory for OpenGL implementation
class EngineFactoryOpenGLImpl : public EngineFactoryBase<IEngineFactoryOpenGL>
{
public:

    static EngineFactoryOpenGLImpl* GetInstance()
    {
        static EngineFactoryOpenGLImpl TheFactory;
        return &TheFactory;
    }

    using TBase = EngineFactoryBase<IEngineFactoryOpenGL>;
    EngineFactoryOpenGLImpl() :
        TBase(IID_EngineFactoryOpenGL)
    {}

    virtual void CreateDeviceAndSwapChainGL(const EngineGLCreateInfo& EngineCI,
                                            IRenderDevice**           ppDevice,
                                            IDeviceContext**          ppImmediateContext,
                                            const SwapChainDesc&      SCDesc, 
                                            ISwapChain**              ppSwapChain )override final;

    virtual void CreateHLSL2GLSLConverter(IHLSL2GLSLConverter** ppConverter)override final;

    virtual void AttachToActiveGLContext(const EngineGLCreateInfo& EngineCI,
                                         IRenderDevice**           ppDevice,
                                         IDeviceContext**          ppImmediateContext )override final;
};



/// Creates render device, device context and swap chain for OpenGL/GLES-based engine implementation

/// \param [in] EngineCI - Engine creation attributes.
/// \param [out] ppDevice - Address of the memory location where pointer to 
///                         the created device will be written.
/// \param [out] ppImmediateContext - Address of the memory location where pointers to 
///                                   the immediate context will be written.
/// \param [in] SCDesc - Swap chain description.
/// \param [out] ppSwapChain    - Address of the memory location where pointer to the new 
///                               swap chain will be written.
void EngineFactoryOpenGLImpl::CreateDeviceAndSwapChainGL(const EngineGLCreateInfo& EngineCI,
                                                         IRenderDevice**           ppDevice,
                                                         IDeviceContext**          ppImmediateContext,
                                                         const SwapChainDesc&      SCDesc, 
                                                         ISwapChain**              ppSwapChain)
{
    if (EngineCI.DebugMessageCallback != nullptr)
        SetDebugMessageCallback(EngineCI.DebugMessageCallback);

    VERIFY( ppDevice && ppImmediateContext && ppSwapChain, "Null pointer provided" );
    if( !ppDevice || !ppImmediateContext || !ppSwapChain )
        return;

    if (EngineCI.NumDeferredContexts > 0)
    {
        LOG_WARNING_MESSAGE("OpenGL back-end does not support deferred contexts");
    }

    *ppDevice = nullptr;
    *ppImmediateContext = nullptr;
    *ppSwapChain = nullptr;

    try
    {
        SetRawAllocator(EngineCI.pRawMemAllocator);
        auto &RawMemAllocator = GetRawAllocator();

        RenderDeviceGLImpl *pRenderDeviceOpenGL( NEW_RC_OBJ(RawMemAllocator, "TRenderDeviceGLImpl instance", TRenderDeviceGLImpl)(RawMemAllocator, this, EngineCI, &SCDesc) );
        pRenderDeviceOpenGL->QueryInterface(IID_RenderDevice, reinterpret_cast<IObject**>(ppDevice) );

        DeviceContextGLImpl* pDeviceContextOpenGL( NEW_RC_OBJ(RawMemAllocator, "DeviceContextGLImpl instance", DeviceContextGLImpl)(pRenderDeviceOpenGL, false ) );
        // We must call AddRef() (implicitly through QueryInterface()) because pRenderDeviceOpenGL will
        // keep a weak reference to the context
        pDeviceContextOpenGL->QueryInterface(IID_DeviceContext, reinterpret_cast<IObject**>(ppImmediateContext) );
        pRenderDeviceOpenGL->SetImmediateContext(pDeviceContextOpenGL);

        TSwapChain *pSwapChainGL = NEW_RC_OBJ(RawMemAllocator, "SwapChainGLImpl instance", TSwapChain)(EngineCI, SCDesc, pRenderDeviceOpenGL, pDeviceContextOpenGL );
        pSwapChainGL->QueryInterface(IID_SwapChain, reinterpret_cast<IObject**>(ppSwapChain) );

        pDeviceContextOpenGL->SetSwapChain(pSwapChainGL);
        // Bind default framebuffer and viewport
        pDeviceContextOpenGL->SetRenderTargets( 0, nullptr, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
        pDeviceContextOpenGL->SetViewports( 1, nullptr, 0, 0 );
    }
    catch( const std::runtime_error & )
    {
        if( *ppDevice )
        {
            (*ppDevice)->Release();
            *ppDevice = nullptr;
        }

        if( *ppImmediateContext )
        {
            (*ppImmediateContext)->Release();
            *ppImmediateContext = nullptr;
        }

        if( *ppSwapChain )
        {
            (*ppSwapChain)->Release();
            *ppSwapChain = nullptr;
        }

        LOG_ERROR( "Failed to initialize OpenGL-based render device" );
    }
}


/// Creates render device, device context and attaches to existing GL context

/// \param [in] EngineCI - Engine creation attributes.
/// \param [out] ppDevice - Address of the memory location where pointer to 
///                         the created device will be written.
/// \param [out] ppImmediateContext - Address of the memory location where pointers to 
///                                   the immediate context will be written.
void EngineFactoryOpenGLImpl::AttachToActiveGLContext(const EngineGLCreateInfo& EngineCI,
                                                      IRenderDevice**           ppDevice,
                                                      IDeviceContext**          ppImmediateContext )
{
    if (EngineCI.DebugMessageCallback != nullptr)
        SetDebugMessageCallback(EngineCI.DebugMessageCallback);

    VERIFY( ppDevice && ppImmediateContext, "Null pointer provided" );
    if( !ppDevice || !ppImmediateContext )
        return;

    if (EngineCI.NumDeferredContexts > 0)
    {
        LOG_WARNING_MESSAGE("OpenGL back-end does not support deferred contexts");
    }

    *ppDevice = nullptr;
    *ppImmediateContext = nullptr;

    try
    {
        SetRawAllocator(EngineCI.pRawMemAllocator);
        auto &RawMemAllocator = GetRawAllocator();

        RenderDeviceGLImpl *pRenderDeviceOpenGL( NEW_RC_OBJ(RawMemAllocator, "TRenderDeviceGLImpl instance", TRenderDeviceGLImpl)(RawMemAllocator, this, EngineCI) );
        pRenderDeviceOpenGL->QueryInterface(IID_RenderDevice, reinterpret_cast<IObject**>(ppDevice) );

        DeviceContextGLImpl* pDeviceContextOpenGL( NEW_RC_OBJ(RawMemAllocator, "DeviceContextGLImpl instance", DeviceContextGLImpl)(pRenderDeviceOpenGL, false ) );
        // We must call AddRef() (implicitly through QueryInterface()) because pRenderDeviceOpenGL will
        // keep a weak reference to the context
        pDeviceContextOpenGL->QueryInterface(IID_DeviceContext, reinterpret_cast<IObject**>(ppImmediateContext) );
        pRenderDeviceOpenGL->SetImmediateContext(pDeviceContextOpenGL);
    }
    catch( const std::runtime_error & )
    {
        if( *ppDevice )
        {
            (*ppDevice)->Release();
            *ppDevice = nullptr;
        }

        if( *ppImmediateContext )
        {
            (*ppImmediateContext)->Release();
            *ppImmediateContext = nullptr;
        }

        LOG_ERROR( "Failed to initialize OpenGL-based render device" );
    }
}

#ifdef DOXYGEN
/// Loads OpenGL-based engine implementation and exports factory functions
/// \param [out] GetFactoryFunc - Pointer to the function that returns pointer to the factory for 
///                               the OpenGL engine implementation
///                               See EngineFactoryOpenGLImpl::CreateDeviceAndSwapChainGL().
/// \remarks Depending on the configuration and platform, the function loads different dll:
/// Platform\\Configuration    |           Debug              |         Release
/// --------------------------|------------------------------|----------------------------
///   Win32/x86               | GraphicsEngineOpenGL_32d.dll | GraphicsEngineOpenGL_32r.dll
///   Win32/x64               | GraphicsEngineOpenGL_64d.dll | GraphicsEngineOpenGL_64r.dll
///
/// To load the library on Android, it is necessary to call System.loadLibrary("GraphicsEngineOpenGL") from Java.
void LoadGraphicsEngineOpenGL(GetEngineFactoryOpenGLType &GetFactoryFunc)
{
    // This function is only required because DoxyGen refuses to generate documentation for a static function when SHOW_FILES==NO
    #error This function must never be compiled;    
}
#endif

void EngineFactoryOpenGLImpl::CreateHLSL2GLSLConverter(IHLSL2GLSLConverter** ppConverter)
{
    HLSL2GLSLConverterObject *pConverter( NEW_RC_OBJ(GetRawAllocator(), "HLSL2GLSLConverterObject instance", HLSL2GLSLConverterObject)() );
    pConverter->QueryInterface( IID_HLSL2GLSLConverter, reinterpret_cast<IObject**>(ppConverter) );
}

API_QUALIFIER
Diligent::IEngineFactoryOpenGL* GetEngineFactoryOpenGL()
{
    return Diligent::EngineFactoryOpenGLImpl::GetInstance();
}

}
