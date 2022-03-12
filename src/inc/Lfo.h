#if !defined(__Lfo_hdr__)
#define __Lfo_hdr__

#include <math.h>
#include "ErrorDef.h"
#include "Synthesis.h"
#include "RingBuffer.h"

#include <iostream>
using namespace::std;


class CLfo {
public:
    static Error_t create (CLfo*& pCLfo) {
        pCLfo = new CLfo();
        if (!pCLfo) {
            return Error_t::kUnknownError;
        }
        return Error_t::kNoError;
    }
    
    static Error_t destroy (CLfo*& pCLfo) {
        if (!pCLfo) {
            return Error_t::kUnknownError;
        }
        pCLfo -> reset();
        delete pCLfo;
        pCLfo = 0;
        return Error_t::kNoError;
    }
    
    Error_t init (float fModFreqRatio, float fSampleRate) {
        this -> reset();
        if (fModFreqRatio <= 0 || fSampleRate <= 0) {
            return Error_t::kFunctionInvalidArgsError;
        }
        m_bIsInitialized = true;
        m_fModFreqRatio = fModFreqRatio;
        m_fSampleRate = fSampleRate;
        m_iWaveLength = static_cast<int>(1/fModFreqRatio);
        m_pSine = new float [m_iWaveLength];
        m_pRingBuffer = new CRingBuffer<float>(m_iWaveLength);
        return Error_t::kNoError;
    }
    
    Error_t reset () {
        if (m_bIsInitialized == true) {
            delete m_pRingBuffer;
            m_pRingBuffer = 0;
            delete [] m_pSine;
            m_pSine = 0;
        }
        m_fSampleRate = 0;
        m_fModFreqRatio = 0;
        m_bIsInitialized = false;
        return Error_t::kNoError;
    }
    
    Error_t generate () {
        CSynthesis::generateSine(m_pSine, m_fModFreqRatio * m_fSampleRate, m_fSampleRate, m_iWaveLength);
        m_pRingBuffer -> putPostInc(m_pSine, m_iWaveLength);
    }
    
    float getValue () {
        return m_pRingBuffer -> getPostInc();
    }
    
    CLfo () {
        m_bIsInitialized = false;
        m_pRingBuffer = 0;
        m_pSine = 0;
        m_fSampleRate = 0;
        m_fModFreqRatio = 0;
        this -> reset ();
    }
    
    virtual ~CLfo () {
        this -> reset();
    }
    
private:
    CRingBuffer<float>  *m_pRingBuffer;
    float               *m_pSine;
    bool                m_bIsInitialized;
    float               m_fSampleRate;
    float               m_fModFreqRatio;
    int                 m_iWaveLength;
};

#endif // __Lfo_hdr__
