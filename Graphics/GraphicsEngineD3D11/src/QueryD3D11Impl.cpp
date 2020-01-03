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

#include "pch.h"

#include "QueryD3D11Impl.h"
#include "DeviceContextD3D11Impl.h"

namespace Diligent
{

QueryD3D11Impl::QueryD3D11Impl(IReferenceCounters*    pRefCounters,
                               RenderDeviceD3D11Impl* pDevice,
                               const QueryDesc&       Desc) :
    TQueryBase{pRefCounters, pDevice, Desc}
{
    D3D11_QUERY_DESC d3d11QueryDesc = {};
    switch (Desc.Type)
    {
        case QUERY_TYPE_UNDEFINED:
            LOG_ERROR_AND_THROW("Query type is undefined");

        case QUERY_TYPE_OCCLUSION:
            d3d11QueryDesc.Query = D3D11_QUERY_OCCLUSION;
            break;

        case QUERY_TYPE_BINARY_OCCLUSION:
            d3d11QueryDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
            break;

        case QUERY_TYPE_TIMESTAMP:
            d3d11QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
            break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
            d3d11QueryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
            break;

        default:
            UNEXPECTED("Unexpected query type");
    }
    auto* pd3d11Device = pDevice->GetD3D11Device();

    auto hr = pd3d11Device->CreateQuery(&d3d11QueryDesc, &m_pd3d11Query);
    CHECK_D3D_RESULT_THROW(hr, "Failed to create D3D11 query object");
    VERIFY_EXPR(m_pd3d11Query != nullptr);
}

QueryD3D11Impl::~QueryD3D11Impl()
{
}

bool QueryD3D11Impl::GetData(void* pData, Uint32 DataSize)
{
    if (!TQueryBase::CheckQueryDataPtr(pData, DataSize))
        return false;

    auto* pCtxD3D11Impl = m_pContext.RawPtr<DeviceContextD3D11Impl>();
    VERIFY_EXPR(!pCtxD3D11Impl->IsDeferred());

    auto* pd3d11Ctx = pCtxD3D11Impl->GetD3D11DeviceContext();

    bool DataReady = false;
    switch (m_Desc.Type)
    {
        case QUERY_TYPE_OCCLUSION:
        {
            UINT64 NumSamples;
            DataReady = pd3d11Ctx->GetData(m_pd3d11Query, &NumSamples, sizeof(NumSamples), 0) == S_OK;
            if (DataReady)
            {
                auto& QueryData      = *reinterpret_cast<QueryDataOcclusion*>(pData);
                QueryData.NumSamples = NumSamples;
            }
        }
        break;

        case QUERY_TYPE_BINARY_OCCLUSION:
        {
            BOOL AnySamplePassed;
            DataReady = pd3d11Ctx->GetData(m_pd3d11Query, &AnySamplePassed, sizeof(AnySamplePassed), 0) == S_OK;
            if (DataReady)
            {
                auto& QueryData           = *reinterpret_cast<QueryDataBinaryOcclusion*>(pData);
                QueryData.AnySamplePassed = AnySamplePassed != FALSE;
            }
        }
        break;

        case QUERY_TYPE_TIMESTAMP:
        {
            VERIFY_EXPR(m_DisjointQuery);
            
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointQueryData;
            DataReady = pd3d11Ctx->GetData(m_DisjointQuery->pd3d11Query, &DisjointQueryData, sizeof(DisjointQueryData), 0) == S_OK;
            
            UINT64 NumTicks;
            if (DataReady)
            {
                DataReady = pd3d11Ctx->GetData(m_pd3d11Query, &NumTicks, sizeof(NumTicks), 0) == S_OK;
            }
            if (DataReady)
            {
                auto& QueryData     = *reinterpret_cast<QueryDataTimestamp*>(pData);
                QueryData.NumTicks  = NumTicks;
                // The timestamp returned by ID3D11DeviceContext::GetData for a timestamp query is only reliable if Disjoint is FALSE.
                QueryData.Frequency = DisjointQueryData.Disjoint ? 0 : DisjointQueryData.Frequency;
            }
        }
        break;

        case QUERY_TYPE_PIPELINE_STATISTICS:
        {
            D3D11_QUERY_DATA_PIPELINE_STATISTICS d3d11QueryData;
            DataReady = pd3d11Ctx->GetData(m_pd3d11Query, &d3d11QueryData, sizeof(d3d11QueryData), 0) == S_OK;
            if (DataReady)
            {
                auto& QueryData = *reinterpret_cast<QueryDataPipelineStatistics*>(pData);

                QueryData.InputVertices       = d3d11QueryData.IAVertices;
                QueryData.InputPrimitives     = d3d11QueryData.IAPrimitives;
                QueryData.GSPrimitives        = d3d11QueryData.GSPrimitives;
                QueryData.ClippingInvocations = d3d11QueryData.CInvocations;
                QueryData.ClippingPrimitives  = d3d11QueryData.CPrimitives;
                QueryData.VSInvocations       = d3d11QueryData.VSInvocations;
                QueryData.GSInvocations       = d3d11QueryData.GSInvocations;
                QueryData.PSInvocations       = d3d11QueryData.PSInvocations;
                QueryData.HSInvocations       = d3d11QueryData.HSInvocations;
                QueryData.DSInvocations       = d3d11QueryData.DSInvocations;
                QueryData.CSInvocations       = d3d11QueryData.CSInvocations;
            }
        }
        break;

        default:
            UNEXPECTED("Unexpected query type");
    }

    return DataReady;
}

} // namespace Diligent
