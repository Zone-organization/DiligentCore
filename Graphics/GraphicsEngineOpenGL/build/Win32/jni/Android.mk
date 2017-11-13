# Android NDK project makefile autogenerated by Premake

# Preamble
DEPENDENCY_PATH := $(call my-dir)
LOCAL_PATH := $(abspath $(DEPENDENCY_PATH))
include $(CLEAR_VARS)


# Project configuration
LOCAL_MODULE := GraphicsEngineOpenGL
LOCAL_CFLAGS := -std=c++11 -DUSE_GL3_STUB=0 -DENGINE_DLL -DBUILDING_DLL -fvisibility=hidden
LOCAL_CPP_FEATURES := exceptions rtti
LOCAL_STATIC_LIBRARIES += GraphicsEngine-prebuilt HLSL2GLSLConverterLib-prebuilt AndroidPlatform-prebuilt BasicPlatform-prebuilt GraphicsTools-prebuilt Common-prebuilt
LOCAL_LDLIBS := -lGLESv3 -lEGL -llog -landroid

# Include paths
PROJECT_ROOT := $(LOCAL_PATH)/../../..
SOLUTION_ROOT := $(PROJECT_ROOT)/../..
LOCAL_C_INCLUDES := $(PROJECT_ROOT)/include $(PROJECT_ROOT)/interface $(SOLUTION_ROOT)/External

# Source files
#VisualGDBAndroid: AutoUpdateSourcesInNextLine
LOCAL_SRC_FILES := ../../../src/BufferGLImpl.cpp ../../../src/BufferViewGLImpl.cpp ../../../src/DeviceContextGLImpl.cpp ../../../src/DLLMain.cpp ../../../src/FBOCache.cpp ../../../src/GLContextAndroid.cpp ../../../src/GLContextState.cpp ../../../src/GLContextWindows.cpp ../../../src/GLObjectWrapper.cpp ../../../src/GLProgram.cpp ../../../src/GLProgramResources.cpp ../../../src/GLStubs.cpp ../../../src/GLTypeConversions.cpp ../../../src/PipelineStateGLImpl.cpp ../../../src/RenderDeviceGLImpl.cpp ../../../src/RenderDeviceGLESImpl.cpp ../../../src/SamplerGLImpl.cpp ../../../src/ShaderResourceBindingGLImpl.cpp ../../../src/SwapChainGLImpl.cpp ../../../src/TexRegionRender.cpp ../../../src/Texture1DArray_OGL.cpp ../../../src/Texture1D_OGL.cpp ../../../src/Texture2DArray_OGL.cpp ../../../src/Texture2D_OGL.cpp ../../../src/Texture3D_OGL.cpp ../../../src/TextureBaseGL.cpp ../../../src/TextureCubeArray_OGL.cpp ../../../src/TextureCube_OGL.cpp ../../../src/TextureViewGLImpl.cpp ../../../src/VAOCache.cpp ../../../src/RenderDeviceFactoryOpenGL.cpp ../../../src/ShaderGLImpl.cpp ../../../src/pch.cpp

#VisualGDBAndroid: VSExcludeListLocation
VISUALGDB_VS_EXCLUDED_FILES_Release := ../../../src/DLLMain.cpp ../../../src/GLContextWindows.cpp
VISUALGDB_VS_EXCLUDED_FILES_Debug := ../../../src/DLLMain.cpp ../../../src/GLContextWindows.cpp
LOCAL_SRC_FILES := $(filter-out $(VISUALGDB_VS_EXCLUDED_FILES_$(VGDB_VSCONFIG)),$(LOCAL_SRC_FILES))
include $(BUILD_SHARED_LIBRARY)


LOCAL_MODULE := GraphicsEngineOpenGL_static
LOCAL_MODULE_FILENAME := libGraphicsEngineOpenGL
LOCAL_CFLAGS := -std=c++11 -DUSE_GL3_STUB=0
LOCAL_LDLIBS :=
include $(BUILD_STATIC_LIBRARY)



include $(CLEAR_VARS)
# Declare pre-built Common library
LOCAL_MODULE := Common-prebuilt
LOCAL_EXPORT_C_INCLUDES := $(SOLUTION_ROOT)/Common/include $(SOLUTION_ROOT)/Common/interface
LOCAL_SRC_FILES := $(SOLUTION_ROOT)/Common/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libCommon.a
include $(PREBUILT_STATIC_LIBRARY)


include $(CLEAR_VARS)
# Declare pre-built AndroidPlatform library
LOCAL_MODULE := AndroidPlatform-prebuilt
LOCAL_SRC_FILES := $(SOLUTION_ROOT)/Platforms/Android/build/Windows/obj/local/$(TARGET_ARCH_ABI)/libAndroidPlatform.a
LOCAL_EXPORT_C_INCLUDES := $(SOLUTION_ROOT)/Platforms/interface
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
# Declare pre-built BasicPlatform library
LOCAL_MODULE := BasicPlatform-prebuilt
LOCAL_SRC_FILES := $(SOLUTION_ROOT)/Platforms/Basic/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libBasicPlatform.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
# Declare pre-built GraphicsEngine library
LOCAL_MODULE := GraphicsEngine-prebuilt
LOCAL_SRC_FILES := $(SOLUTION_ROOT)/Graphics/GraphicsEngine/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libGraphicsEngine.a
LOCAL_EXPORT_C_INCLUDES := $(SOLUTION_ROOT)/Graphics/GraphicsEngine/include $(SOLUTION_ROOT)/Graphics/GraphicsEngine/interface
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
# Declare pre-built Graphics Tools library
LOCAL_MODULE := GraphicsTools-prebuilt
LOCAL_SRC_FILES := $(SOLUTION_ROOT)/Graphics/GraphicsTools/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libGraphicsTools.a
LOCAL_EXPORT_C_INCLUDES := $(SOLUTION_ROOT)/Graphics/GraphicsTools/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
# Declare pre-built HLSL2GLSLConverterLib library
LOCAL_MODULE := HLSL2GLSLConverterLib-prebuilt
LOCAL_SRC_FILES := $(SOLUTION_ROOT)/Graphics/HLSL2GLSLConverterLib/build/Win32/obj/local/$(TARGET_ARCH_ABI)/libHLSL2GLSLConverterLib.a
LOCAL_EXPORT_C_INCLUDES := $(SOLUTION_ROOT)/Graphics/HLSL2GLSLConverterLib/include $(SOLUTION_ROOT)/Graphics/HLSL2GLSLConverterLib/interface
include $(PREBUILT_STATIC_LIBRARY)
