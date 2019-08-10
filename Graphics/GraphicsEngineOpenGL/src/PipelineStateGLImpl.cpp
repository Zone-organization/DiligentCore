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

#include "pch.h"
#include "PipelineStateGLImpl.h"
#include "RenderDeviceGLImpl.h"
#include "ShaderGLImpl.h"
#include "ShaderResourceBindingGLImpl.h"
#include "EngineMemory.h"

namespace Diligent
{

PipelineStateGLImpl::PipelineStateGLImpl(IReferenceCounters*      pRefCounters,
                                         RenderDeviceGLImpl*      pDeviceGL,
                                         const PipelineStateDesc& PipelineDesc,
                                         bool                     bIsDeviceInternal) : 
    TPipelineStateBase(pRefCounters, pDeviceGL, PipelineDesc, bIsDeviceInternal),
    m_GLProgram(false)
{
    if (!m_Desc.IsComputePipeline && m_pPS == nullptr)
    {
        // Some OpenGL implementations fail if fragment shader is not present, so
        // create a dummy one.
        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_GLSL;
        ShaderCI.Source          = "void main(){}";
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.Desc.Name       = "Dummy fragment shader";
        pDeviceGL->CreateShader(ShaderCI, &m_pPS);
        m_Desc.GraphicsPipeline.pPS = m_pPS;
        m_ppShaders[m_NumShaders++] = m_pPS;
    }

    auto &DeviceCaps = pDeviceGL->GetDeviceCaps();
    VERIFY( DeviceCaps.DevType != DeviceType::Undefined, "Device caps are not initialized" );
    bool bIsProgramPipelineSupported = DeviceCaps.bSeparableProgramSupported;

    LinkGLProgram(bIsProgramPipelineSupported);
}

void PipelineStateGLImpl::LinkGLProgram(bool bIsProgramPipelineSupported)
{
    if (bIsProgramPipelineSupported)
    {
        // Program pipelines are not shared between GL contexts, so we cannot create
        // it now
        m_ShaderResourceLayoutHash = 0;
        m_StaticResources.resize(m_NumShaders);
        for (Uint32 Shader = 0; Shader < m_NumShaders; ++Shader)
        {
            auto* pShaderGL = GetShader<ShaderGLImpl>(Shader);
            const SHADER_RESOURCE_VARIABLE_TYPE StaticVars[] = {SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
            m_StaticResources[Shader].Clone(GetDevice(), *this, pShaderGL->GetGlProgram().GetResources(), m_Desc.ResourceLayout, StaticVars, _countof(StaticVars));

            const auto ShaderType = pShaderGL->GetDesc().ShaderType;
            const auto ShaderTypeInd = GetShaderTypeIndex(ShaderType);
            m_ResourceLayoutIndex[ShaderTypeInd] = static_cast<Int8>(Shader);

            HashCombine(m_ShaderResourceLayoutHash, pShaderGL->m_GlProgObj.GetResources().GetHash());
        }
    }
    else
    {
        // Create new progam
        m_GLProgram.Create();
        for (Uint32 Shader = 0; Shader < m_NumShaders; ++Shader)
        {
            auto* pCurrShader = GetShader<ShaderGLImpl>(Shader);
            glAttachShader(m_GLProgram, pCurrShader->m_GLShaderObj);
            CHECK_GL_ERROR("glAttachShader() failed");
        }
        glLinkProgram(m_GLProgram);
        CHECK_GL_ERROR("glLinkProgram() failed");
        int IsLinked = GL_FALSE;
        glGetProgramiv(m_GLProgram, GL_LINK_STATUS, &IsLinked);
        CHECK_GL_ERROR("glGetProgramiv() failed");
        if (!IsLinked)
        {
            int LengthWithNull = 0, Length = 0;
            // Notice that glGetProgramiv is used to get the length for a shader program, not glGetShaderiv.
            // The length of the info log includes a null terminator.
            glGetProgramiv(m_GLProgram, GL_INFO_LOG_LENGTH, &LengthWithNull);

            // The maxLength includes the NULL character
            std::vector<char> shaderProgramInfoLog(LengthWithNull);

            // Notice that glGetProgramInfoLog  is used, not glGetShaderInfoLog.
            glGetProgramInfoLog(m_GLProgram, LengthWithNull, &Length, shaderProgramInfoLog.data());
            VERIFY(Length == LengthWithNull-1, "Incorrect program info log len");
            LOG_ERROR_MESSAGE("Failed to link shader program:\n", shaderProgramInfoLog.data(), '\n');
            UNEXPECTED("glLinkProgram failed");
        }
            
        // Detach shaders from the program object
        for (Uint32 Shader = 0; Shader < m_NumShaders; ++Shader)
        {
            auto* pCurrShader = GetShader<ShaderGLImpl>(Shader);
            glDetachShader(m_GLProgram, pCurrShader->m_GLShaderObj);
            CHECK_GL_ERROR("glDetachShader() failed");
        }

        SHADER_TYPE ShaderStages = SHADER_TYPE_UNKNOWN;
        for (Uint32 Shader = 0; Shader < m_NumShaders; ++Shader)
        {
            auto* pCurrShader = GetShader<ShaderGLImpl>(Shader);
            const auto& Desc = pCurrShader->GetDesc();
            ShaderStages |= Desc.ShaderType;
        }

        auto pDeviceGL = GetDevice();
        m_GLProgram.InitResources(pDeviceGL, ShaderStages, *this);
        
        m_StaticResources.resize(1);
        const SHADER_RESOURCE_VARIABLE_TYPE StaticVars[] = {SHADER_RESOURCE_VARIABLE_TYPE_STATIC};
        m_StaticResources[0].Clone(GetDevice(), *this, m_GLProgram.GetResources(), m_Desc.ResourceLayout, StaticVars, _countof(StaticVars));

        m_ShaderResourceLayoutHash = m_GLProgram.GetResources().GetHash();
    }   
}


PipelineStateGLImpl::~PipelineStateGLImpl()
{
    GetDevice()->OnDestroyPSO(this);
}

IMPLEMENT_QUERY_INTERFACE( PipelineStateGLImpl, IID_PipelineStateGL, TPipelineStateBase )


void PipelineStateGLImpl::CreateShaderResourceBinding(IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
{
    auto* pRenderDeviceGL = GetDevice();
    auto& SRBAllocator = pRenderDeviceGL->GetSRBAllocator();
    auto pResBinding = NEW_RC_OBJ(SRBAllocator, "ShaderResourceBindingGLImpl instance", ShaderResourceBindingGLImpl)(this);
    if (InitStaticResources)
        pResBinding->InitializeStaticResources(this);
    pResBinding->QueryInterface(IID_ShaderResourceBinding, reinterpret_cast<IObject**>(ppShaderResourceBinding));
}

bool PipelineStateGLImpl::IsCompatibleWith(const IPipelineState* pPSO)const
{
    VERIFY_EXPR(pPSO != nullptr);

    if (pPSO == this)
        return true;

    const PipelineStateGLImpl* pPSOGL = ValidatedCast<const PipelineStateGLImpl>(pPSO);
    if (m_ShaderResourceLayoutHash != pPSOGL->m_ShaderResourceLayoutHash)
        return false;

    return m_GLProgram.GetResources().IsCompatibleWith(pPSOGL->m_GLProgram.GetResources());
}

GLObjectWrappers::GLPipelineObj& PipelineStateGLImpl::GetGLProgramPipeline(GLContext::NativeGLContextType Context)
{
    ThreadingTools::LockHelper Lock(m_ProgPipelineLockFlag);
    for(auto& ctx_pipeline : m_GLProgPipelines)
    {
        if (ctx_pipeline.first == Context)
            return ctx_pipeline.second;
    }

    // Create new progam pipeline
    m_GLProgPipelines.emplace_back(Context, true);
    auto& ctx_pipeline = m_GLProgPipelines.back();
    GLuint Pipeline = ctx_pipeline.second;
    for (Uint32 Shader = 0; Shader < m_NumShaders; ++Shader)
    {
        auto* pCurrShader = GetShader<ShaderGLImpl>(Shader);
        auto GLShaderBit = ShaderTypeToGLShaderBit(pCurrShader->GetDesc().ShaderType);
        // If the program has an active code for each stage mentioned in set flags,
        // then that code will be used by the pipeline. If program is 0, then the given
        // stages are cleared from the pipeline.
        glUseProgramStages(Pipeline, GLShaderBit, pCurrShader->m_GlProgObj);
        CHECK_GL_ERROR("glUseProgramStages() failed");
    }
    return ctx_pipeline.second;
}


void PipelineStateGLImpl::BindStaticResources(Uint32 ShaderFlags, IResourceMapping* pResourceMapping, Uint32 Flags)
{
    for(auto& StaticRes : m_StaticResources)
    {
        if ((StaticRes.GetShaderStages() & ShaderFlags)!=0)
            StaticRes.BindResources(pResourceMapping, Flags);
    }
}
    
Uint32 PipelineStateGLImpl::GetStaticVariableCount(SHADER_TYPE ShaderType) const
{
    if (m_GLProgram)
    {
        return (m_StaticResources[0].GetShaderStages() & ShaderType) != 0 ? m_StaticResources[0].GetVariableCount() : 0;
    }
    else
    {
        const auto LayoutInd = m_ResourceLayoutIndex[GetShaderTypeIndex(ShaderType)];
        return LayoutInd >= 0 ? m_StaticResources[LayoutInd].GetVariableCount() : 0;
    }
}

IShaderResourceVariable* PipelineStateGLImpl::GetStaticVariableByName(SHADER_TYPE ShaderType, const Char* Name)
{
    if (m_GLProgram)
    {
        return (m_StaticResources[0].GetShaderStages() & ShaderType) != 0 ? m_StaticResources[0].GetVariable(Name) : nullptr;
    }
    else
    {
        const auto LayoutInd = m_ResourceLayoutIndex[GetShaderTypeIndex(ShaderType)];
        return LayoutInd >= 0 ? m_StaticResources[LayoutInd].GetVariable(Name) : nullptr;
    }
}

IShaderResourceVariable* PipelineStateGLImpl::GetStaticVariableByIndex(SHADER_TYPE ShaderType, Uint32 Index)
{
    if (m_GLProgram)
    {
        return (m_StaticResources[0].GetShaderStages() & ShaderType) != 0 ? m_StaticResources[0].GetVariable(Index) : nullptr;
    }
    else
    {
        const auto LayoutInd = m_ResourceLayoutIndex[GetShaderTypeIndex(ShaderType)];
        return LayoutInd >= 0 ? m_StaticResources[LayoutInd].GetVariable(Index) : nullptr;
    }
}

}
