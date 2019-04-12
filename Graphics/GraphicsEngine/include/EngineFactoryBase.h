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

#pragma once

/// \file
/// Implementation of the Diligent::EngineFactoryBase template class

#include "Object.h"
#include "EngineFactory.h"
#include "DefaultShaderSourceStreamFactory.h"

namespace Diligent
{

const APIInfo& GetAPIInfo();

/// Template class implementing base functionality for an engine factory

/// \tparam BaseInterface - base interface that this class will inheret
///                         (Diligent::IEngineFactoryD3D11, Diligent::IEngineFactoryD3D12,
///                          Diligent::IEngineFactoryVk or Diligent::IEngineFactoryOpenGL).
template<class BaseInterface>
class EngineFactoryBase : public BaseInterface
{
public:
    using CounterValueType = IReferenceCounters::CounterValueType;

    EngineFactoryBase(const INTERFACE_ID& FactoryIID)noexcept :
        m_FactoryIID(FactoryIID),
        m_RefCounters(*this)
    {
    }

    virtual void QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)override final
    {
        if (ppInterface == nullptr)
            return;

        *ppInterface = nullptr;
        if (IID == IID_Unknown || IID == m_FactoryIID || IID == IID_EngineFactory)
        {
            *ppInterface = this;
            (*ppInterface)->AddRef();
        }
    }

    virtual CounterValueType AddRef()override final
    {
        return m_RefCounters.AddStrongRef();
    }

    virtual CounterValueType Release()override final
    {
        return m_RefCounters.ReleaseStrongRef();
    }

    virtual IReferenceCounters* GetReferenceCounters()const override final
    {
        return const_cast<IReferenceCounters*>(static_cast<const IReferenceCounters*>(&m_RefCounters));
    }

    virtual const APIInfo& GetAPIInfo() const override final
    {
        return Diligent::GetAPIInfo();
    }

    virtual void CreateDefaultShaderSourceStreamFactory(const Char*                       SearchDirectories, 
                                                        IShaderSourceInputStreamFactory** ppShaderSourceFactory)const override final
    {
        Diligent::CreateDefaultShaderSourceStreamFactory(SearchDirectories, ppShaderSourceFactory);
    }

private:
    class DummyReferenceCounters final : public IReferenceCounters
    {
    public:
        DummyReferenceCounters(EngineFactoryBase& Factory)noexcept :
            m_Factory(Factory)
        {
            m_lNumStrongReferences = 0;
            m_lNumWeakReferences   = 0;
        }

        using IReferenceCounters::CounterValueType;
        virtual CounterValueType AddStrongRef() override final
        {
            return Atomics::AtomicIncrement(m_lNumStrongReferences);
        }

        virtual CounterValueType ReleaseStrongRef()override final
        {
            return Atomics::AtomicDecrement(m_lNumStrongReferences);
        }

        virtual CounterValueType AddWeakRef()override final
        {
            return Atomics::AtomicIncrement(m_lNumWeakReferences);
        }
            
        virtual CounterValueType ReleaseWeakRef()override final
        {
            return Atomics::AtomicDecrement(m_lNumWeakReferences);
        }

        virtual void GetObject(IObject** ppObject)override final
        {
            if (ppObject != nullptr)
                m_Factory.QueryInterface(IID_Unknown, ppObject);
        }

        virtual CounterValueType GetNumStrongRefs()const override final
        {
            return m_lNumStrongReferences;
        }

        virtual CounterValueType GetNumWeakRefs()const override final
        {
            return m_lNumWeakReferences;
        }
    private:
        EngineFactoryBase& m_Factory;
        Atomics::AtomicLong m_lNumStrongReferences;
        Atomics::AtomicLong m_lNumWeakReferences;
    };

    const INTERFACE_ID m_FactoryIID;
    DummyReferenceCounters m_RefCounters;
};

}
