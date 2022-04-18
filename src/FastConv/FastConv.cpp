
#include "FastConv.h"
#include <iostream>

CFastConv::CFastConv( void ) :
    m_bIsInitialized(false),
    m_iBlockLength(0),
    m_iOverlapLength(0),
    m_iDataLength(0),
    m_iIRLength(0),
    m_pCRingBuff(0),
    m_pfIR(0),
    m_iCompType(0)
{
    this -> reset();
}

CFastConv::~CFastConv( void )
{
    this -> reset();
}

Error_t CFastConv::create(CFastConv*& pCFastConv)
{
    pCFastConv = new CFastConv;
    if (!pCFastConv)
        return Error_t::kMemError;
    return Error_t::kNoError;
}

Error_t CFastConv::destroy(CFastConv *&pCFastConv)
{
    if (!pCFastConv)
        return Error_t::kUnknownError;
    
    pCFastConv->reset();
    
    delete pCFastConv;
    pCFastConv = 0;
    
    return Error_t::kNoError;
}

Error_t CFastConv::init(float *pfImpulseResponse, int iLengthOfIr, int iBlockLength /*= 8192*/, ConvCompMode_t eCompMode /*= kFreqDomain*/)
{
    switch(eCompMode)
    {
        case(kTimeDomain):
            m_iBlockLength = iBlockLength;
            m_iIRLength = iLengthOfIr;
            m_iOverlapLength = iLengthOfIr - 1;
            m_iDataLength = iBlockLength + 1 - iLengthOfIr;
            m_pCRingBuff = new CRingBuffer<float>(iBlockLength*2);
            m_pCRingBuff->setReadIdx(0);
            m_pCRingBuff->setWriteIdx(0);
            m_pfIR = pfImpulseResponse;
            break;
            
        case(kFreqDomain):
            m_iBlockLength = iBlockLength;
            m_iIRLength = iLengthOfIr;
            m_iOverlapLength = iLengthOfIr - 1;
            m_iDataLength = iBlockLength + iLengthOfIr - 1;
            m_pCRingBuff = new CRingBuffer<float>(m_iDataLength);
            m_pCRingBuff->setReadIdx(0);
            m_pCRingBuff->setWriteIdx(0);
            m_pfIR = pfImpulseResponse;
            m_iCompType = 1;
            break;

        case kNumConvCompModes:
            return Error_t::kInvalidString;
    }
    return Error_t::kNoError;
}

Error_t CFastConv::reset()
{
    if(m_bIsInitialized)
    {
        delete m_pCRingBuff;
        m_pCRingBuff = 0;
        
        delete m_pfIR;
        m_pfIR = 0;
    }
    
    m_iBlockLength = 0;
    m_iIRLength = 0;
    m_iOverlapLength = 0;
    m_iDataLength = 0;
    m_iCompType = 0;
    
    return Error_t::kNoError;
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
{
    
    // here is the input check
    std::cout << "--------input" << std::endl;
    for (int i = 0; i < iLengthOfBuffers; i++)
        std::cout << pfInputBuffer[i] << ", ";
    std::cout << std::endl;
    // impulse response
    std::cout << "---------impulse response" << std::endl;
    for (int i = 0; i < m_iIRLength; i++)
        std::cout << m_pfIR[i] << std::endl;
    
    
    if(m_iCompType == 0)
    {
        for (int n = 0; n < iLengthOfBuffers; n++) {
            //y[n] = 0;
            m_pCRingBuff->putPostInc(pfInputBuffer[n]);
            for (int k = 0; k < m_iDataLength; k++) {
                // To right shift the impulse
                if ((n - k) >= 0 && (n - k) < m_iIRLength) {
                    // Main calculation
                    pfOutputBuffer[n] += m_pCRingBuff->get(k) * m_pfIR[n - k];
                }
            }
        }
    }
    else if(m_iCompType == 1)
    {
        // reset outputbuffer
        for (int i = 0; i < iLengthOfBuffers; i++)
            pfOutputBuffer[i] = 0;
        
        m_pCRingBuff -> putPostInc(pfInputBuffer, iLengthOfBuffers);
        
        std::cout << "-------------bufer" << std::endl;
        for (int i = 0; i < m_pCRingBuff->getLength(); i++)
        {
            std::cout << m_pCRingBuff->get(i) << ", ";
        }
        std::cout << std::endl;
        
        for (int n = 0; n < iLengthOfBuffers + m_iIRLength - 1; n++)
        {
            for (int k = 0; k < iLengthOfBuffers; k++)
            {
                if((n-k) >= 0 && (n-k) < m_iIRLength && n < iLengthOfBuffers)
                {
                    pfOutputBuffer[n] += m_pCRingBuff->get(k) * m_pfIR[n - k];
                    if(n < m_iOverlapLength)
                        pfOutputBuffer[n] += m_pCRingBuff->get(k+iLengthOfBuffers);
                }
                else if((n-k) >= 0 && (n-k) < m_iIRLength && n >= iLengthOfBuffers)
                {
                    m_pCRingBuff->putPostInc(m_pCRingBuff->get(k)*m_pfIR[n-k]);
                }
            }
        }
        
        std::cout << m_pCRingBuff->getLength() << std::endl;
        m_pCRingBuff->setWriteIdx(0);
    }
    
    // here is the output check
    std::cout << "---------------------output" << std::endl;
    for (int i = 0; i < iLengthOfBuffers; i++)
        std::cout << pfOutputBuffer[i] << ", ";
    std::cout << std::endl;
    std::cout << std::endl;

    return Error_t::kNoError;
}

Error_t CFastConv::flushBuffer(float* pfOutputBuffer)
{
    return Error_t::kNoError;
}
