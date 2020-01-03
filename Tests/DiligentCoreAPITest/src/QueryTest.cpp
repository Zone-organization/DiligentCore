/*
 *  Copyright 2019-2020 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#include <sstream>
#include <array>

#include "TestingEnvironment.h"

#include "gtest/gtest.h"

using namespace Diligent;
using namespace Diligent::Testing;

namespace
{

// clang-format off
const std::string QueryTest_ProceduralQuadVS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
};

void main(in uint VertId : SV_VertexID,
          out PSInput PSIn) 
{
    float HalfTexel = 0.5 / 512.0;
    float size = 0.25;
    float4 Pos[4];

    Pos[0] = float4(-size-HalfTexel, -size-HalfTexel, 0.0, 1.0);
    Pos[1] = float4(-size-HalfTexel, +size-HalfTexel, 0.0, 1.0);
    Pos[2] = float4(+size-HalfTexel, -size-HalfTexel, 0.0, 1.0);
    Pos[3] = float4(+size-HalfTexel, +size-HalfTexel, 0.0, 1.0);

    PSIn.Pos = Pos[VertId];
}
)"
};

const std::string QueryTest_PS{
R"(
struct PSInput 
{ 
    float4 Pos   : SV_POSITION; 
};

float4 main(in PSInput PSIn) : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
)"
};
// clang-format on


class QueryTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        auto* pEnv    = TestingEnvironment::GetInstance();
        auto* pDevice = pEnv->GetDevice();

        TextureDesc TexDesc;
        TexDesc.Name      = "Mips generation test texture";
        TexDesc.Type      = RESOURCE_DIM_TEX_2D;
        TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
        TexDesc.Width     = sm_TextureSize;
        TexDesc.Height    = sm_TextureSize;
        TexDesc.BindFlags = BIND_RENDER_TARGET;
        TexDesc.MipLevels = 1;
        TexDesc.Usage     = USAGE_DEFAULT;
        RefCntAutoPtr<ITexture> pRenderTarget;
        pDevice->CreateTexture(TexDesc, nullptr, &pRenderTarget);
        ASSERT_NE(pRenderTarget, nullptr) << "TexDesc:\n"
                                          << TexDesc;
        sm_pRTV = pRenderTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
        ASSERT_NE(sm_pRTV, nullptr);

        PipelineStateDesc PSODesc;
        PSODesc.Name = "Query command test - procedural quad";

        PSODesc.IsComputePipeline                             = false;
        PSODesc.GraphicsPipeline.NumRenderTargets             = 1;
        PSODesc.GraphicsPipeline.RTVFormats[0]                = TexDesc.Format;
        PSODesc.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        PSODesc.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;
        PSODesc.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage             = SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = true;

        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Query test vertex shader";
            ShaderCI.Source          = QueryTest_ProceduralQuadVS.c_str();
            pDevice->CreateShader(ShaderCI, &pVS);
            ASSERT_NE(pVS, nullptr);
        }

        RefCntAutoPtr<IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint      = "main";
            ShaderCI.Desc.Name       = "Query test pixel shader";
            ShaderCI.Source          = QueryTest_PS.c_str();
            pDevice->CreateShader(ShaderCI, &pPS);
            ASSERT_NE(pVS, nullptr);
        }

        PSODesc.GraphicsPipeline.pVS = pVS;
        PSODesc.GraphicsPipeline.pPS = pPS;
        pDevice->CreatePipelineState(PSODesc, &sm_pPSO);
        ASSERT_NE(sm_pPSO, nullptr);
    }

    static void TearDownTestSuite()
    {
        sm_pPSO.Release();
        sm_pRTV.Release();

        auto* pEnv = TestingEnvironment::GetInstance();
        pEnv->Reset();
    }

    static void DrawQuad()
    {
        auto* pEnv     = TestingEnvironment::GetInstance();
        auto* pContext = pEnv->GetDeviceContext();

        ITextureView* pRTVs[] = {sm_pRTV};
        pContext->SetRenderTargets(1, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        const float ClearColor[] = {0.f, 0.f, 0.f, 0.0f};
        pContext->ClearRenderTarget(pRTVs[0], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        pContext->SetPipelineState(sm_pPSO);
        // Commit shader resources. We don't really have any resources, but this call also sets the shaders in OpenGL backend.
        pContext->CommitShaderResources(nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawAttribs drawAttrs{4, DRAW_FLAG_VERIFY_ALL};
        pContext->Draw(drawAttrs);
    }

    static constexpr Uint32 sm_NumTestQueries = 3;
    void                    InitTestQueries(std::array<RefCntAutoPtr<IQuery>, sm_NumTestQueries>& Queries, const QueryDesc& queryDesc)
    {
        auto* pEnv     = TestingEnvironment::GetInstance();
        auto* pDevice  = pEnv->GetDevice();
        auto* pContext = pEnv->GetDeviceContext();

        for (auto& pQuery : Queries)
        {
            pDevice->CreateQuery(queryDesc, &pQuery);
            ASSERT_NE(pQuery, nullptr) << "Failed to create pipeline stats query";
        }

        pContext->BeginQuery(Queries[2]);
        DrawQuad();
        DrawQuad();
        DrawQuad();
        pContext->BeginQuery(Queries[1]);
        DrawQuad();
        DrawQuad();
        pContext->BeginQuery(Queries[0]);
        DrawQuad();
        pContext->EndQuery(Queries[2]);
        pContext->EndQuery(Queries[1]);
        pContext->EndQuery(Queries[0]);

        pContext->WaitForIdle();
    }

    static constexpr Uint32 sm_TextureSize = 512;

    static RefCntAutoPtr<ITextureView>   sm_pRTV;
    static RefCntAutoPtr<IPipelineState> sm_pPSO;
};

RefCntAutoPtr<ITextureView>   QueryTest::sm_pRTV;
RefCntAutoPtr<IPipelineState> QueryTest::sm_pPSO;

TEST_F(QueryTest, PipelineStats)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Pipeline stats query";
    queryDesc.Type = QUERY_TYPE_PIPELINE_STATISTICS;

    std::array<RefCntAutoPtr<IQuery>, sm_NumTestQueries> Queries;
    InitTestQueries(Queries, queryDesc);

    auto DrawCounter = 0;
    for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
    {
        DrawCounter += 1 + i;

        QueryDataPipelineStatistics QueryData;
        auto                        QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
        EXPECT_GE(QueryData.InputVertices, 4 * DrawCounter);
        EXPECT_GE(QueryData.InputPrimitives, 2 * DrawCounter);
        EXPECT_GE(QueryData.ClippingInvocations, 2 * DrawCounter);
        EXPECT_GE(QueryData.ClippingPrimitives, 2 * DrawCounter);
        EXPECT_GE(QueryData.VSInvocations, 4 * DrawCounter);
        auto NumPixels = sm_TextureSize * sm_TextureSize / 16;
        EXPECT_GE(QueryData.PSInvocations, NumPixels * DrawCounter);
    }
}

TEST_F(QueryTest, Occlusion)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Occlusion query";
    queryDesc.Type = QUERY_TYPE_OCCLUSION;

    std::array<RefCntAutoPtr<IQuery>, sm_NumTestQueries> Queries;
    InitTestQueries(Queries, queryDesc);

    auto DrawCounter = 0;
    for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
    {
        DrawCounter += 1 + i;
        QueryDataOcclusion QueryData;

        auto QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
        auto NumPixels = sm_TextureSize * sm_TextureSize / 16;
        EXPECT_GE(QueryData.NumSamples, NumPixels * DrawCounter);
    }
}

TEST_F(QueryTest, BinaryOcclusion)
{
    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Binary occlusion query";
    queryDesc.Type = QUERY_TYPE_BINARY_OCCLUSION;

    std::array<RefCntAutoPtr<IQuery>, sm_NumTestQueries> Queries;
    InitTestQueries(Queries, queryDesc);

    for (Uint32 i = 0; i < sm_NumTestQueries; ++i)
    {
        QueryDataBinaryOcclusion QueryData;

        auto QueryReady = Queries[i]->GetData(&QueryData, sizeof(QueryData));
        ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
        EXPECT_TRUE(QueryData.AnySamplePassed);
    }
}

TEST_F(QueryTest, Timestamp)
{
    auto* pEnv     = TestingEnvironment::GetInstance();
    auto* pDevice  = pEnv->GetDevice();
    auto* pContext = pEnv->GetDeviceContext();

    TestingEnvironment::ScopedReset EnvironmentAutoReset;

    QueryDesc queryDesc;
    queryDesc.Name = "Timestamp query";
    queryDesc.Type = QUERY_TYPE_TIMESTAMP;

    RefCntAutoPtr<IQuery> pQueryStart;
    pDevice->CreateQuery(queryDesc, &pQueryStart);
    ASSERT_NE(pQueryStart, nullptr) << "Failed to create tiemstamp query";

    RefCntAutoPtr<IQuery> pQueryEnd;
    pDevice->CreateQuery(queryDesc, &pQueryEnd);
    ASSERT_NE(pQueryEnd, nullptr) << "Failed to create tiemstamp query";

    pContext->EndQuery(pQueryStart);
    DrawQuad();
    pContext->EndQuery(pQueryEnd);

    pContext->FinishFrame();
    pContext->WaitForIdle();
    QueryDataTimestamp QueryStartData, QueryEndData;

    auto QueryReady = pQueryStart->GetData(&QueryStartData, sizeof(QueryStartData));
    ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
    QueryReady = pQueryEnd->GetData(&QueryEndData, sizeof(QueryEndData));
    ASSERT_TRUE(QueryReady) << "Query data must be available after idling the context";
    EXPECT_TRUE(QueryStartData.Frequency == 0 || QueryEndData.Frequency == 0 || QueryEndData.NumTicks > QueryStartData.NumTicks);
}

} // namespace
