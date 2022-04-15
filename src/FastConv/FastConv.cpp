
#include "FastConv.h"

CFastConv::CFastConv( void )
{
}

CFastConv::~CFastConv( void )
{
    reset();
}

Error_t CFastConv::create(CFastConv*& pCFastConv)
{
    pCFastConv = new CFastConv;
    if (!pCFastConv)
        return Error_t::kMemError;
    return Error_t::kNoError;
}


Error_t CFastConv::init(float *pfImpulseResponse, int iLengthOfIr, int iBlockLength /*= 8192*/, ConvCompMode_t eCompMode /*= kFreqDomain*/)
{
    
    m_iBlockLength = iBlockLength;
    m_iIRLength = iLengthOfIr;
    m_iOverlapLength = iLengthOfIr - 1;
    m_iDataLength = iBlockLength + 1 - iLengthOfIr;
    m_pCRingBuff = new CRingBuffer<float>(iBlockLength*2);
    m_pCRingBuff->setReadIdx(0);
    m_pCRingBuff->setWriteIdx(0);
    m_pfIR = pfImpulseResponse;

    
    
    return Error_t::kNoError;
}

Error_t CFastConv::reset()
{
    delete m_pCRingBuff;
    m_pCRingBuff = 0;
    
    return Error_t::kNoError;
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
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
    /*
    for (int i = 0; i < bufferLength; ++i) {
            m_buffer->putPostInc(input[i]);
            for (int j = 0; j < m_IRLength; ++j) {
                output[i] += m_buffer->get(m_IRLength-j) * (m_IR[j] * m_wetGain);
            }
            m_buffer->getPostInc();
        }
        return Error_t::kNoError;
    */
    return Error_t::kNoError;
}

Error_t CFastConv::flushBuffer(float* pfOutputBuffer)
{
    return Error_t::kNoError;
}
