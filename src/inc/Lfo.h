#if !defined(__Lfo_hdr__)
#define __Lfo_hdr__

#define _USE_MATH_DEFINES
#include <math.h>

#include "ErrorDef.h"
#include "Synthesis.h"
#include "RingBuffer.h"

class CLfo
{
public:
    CLfo(float fModFreq, float fSampleRate)
    {
        m_pCRingBuffer = 0;
        m_fRead = 0;
        m_fModFreq = fModFreq;
        m_fSampleRate = fSampleRate;
        if (m_fModFreq == 0) {
            m_iBufferLength = 1;
        } else {
            m_iBufferLength = static_cast<int>(1 / m_fModFreq);
        }
        m_pCRingBuffer = new CRingBuffer<float>(m_iBufferLength);
    }
    
    ~CLfo()
    {
        delete m_pCRingBuffer;
        m_pCRingBuffer = 0;
    }
    
    Error_t setParam(float fParamValue)
    {
        m_fModFreq = fParamValue;
        float *pSine = new float [m_iBufferLength];
        
        if (m_fModFreq == 0) {
            m_iBufferLength = 1;
        }
        else {
            m_iBufferLength = static_cast<int>(1 / m_fModFreq);
        }
        
        CSynthesis::generateSine(pSine, m_fModFreq * m_fSampleRate, m_fSampleRate, m_iBufferLength);
        m_pCRingBuffer-> putPostInc(pSine, m_iBufferLength);
        delete [] pSine;
        
        return Error_t::kNoError;
    }
    
    //return current values
    float returnLfoVal()
    {
        float fLfo = m_pCRingBuffer->get(m_fRead);
        
        m_fRead = m_fRead + 1;
        
        if (m_fRead >= m_iBufferLength) {
            m_fRead -= m_iBufferLength;
        }
        
        return fLfo;
        
    }
    
    void process()
    {
        float *pSine = new float [m_iBufferLength];
        
        CSynthesis::generateSine(pSine, m_fModFreq * m_fSampleRate, m_fSampleRate, m_iBufferLength);
        
        m_pCRingBuffer-> putPostInc(pSine, m_iBufferLength);
        
        delete [] pSine;
    }
    
private:
    
    int                             m_iBufferLength;
    float                           m_fSampleRate;
    float                           m_fRead;
    float                           m_fModFreq;
    CRingBuffer<float>              *m_pCRingBuffer;
};

#endif // __Lfo_hdr__
