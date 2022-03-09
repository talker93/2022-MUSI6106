#include "MUSI6106Config.h"
#include "ErrorDef.h"
#include "RingBuffer.h"
#include "Vibrato.h"
#include "Lfo.h"

CVibrato::CVibrato () {
    m_bIsInitialized = false;
    m_pCVibrato = 0;
    m_pCLfo = 0;
    m_fSampleRateInHz = 0;
    m_iNumChannels = 0;
    m_fDelayInS = 0;
    m_fModFreqInHz = 0;
    m_iNumberOfFrames = 0;
    m_fModWidthInS = 0;
    this -> reset ();
}

CVibrato::~CVibrato () {
    this -> reset();
}

Error_t CVibrato::create(CVibrato*& pCMyProject) {
    pCMyProject = new CVibrato ();
    if (!pCMyProject) {
        return Error_t::kUnknownError;
    }
    return Error_t::kNoError;
}

Error_t CVibrato::destroy(CVibrato*& pCMyProject) {
    if (!pCMyProject) {
        return Error_t::kUnknownError;
    }
    
    pCMyProject -> reset();
    
    delete pCMyProject;
    pCMyProject = 0;
    
    return Error_t::kNoError;
}

Error_t CVibrato::init(float fDelayInS, float fSampleRateInHz, float fModWidthInS, float fModFreqInHz, int iNumChannels) {
    reset();
    
    if (fDelayInS <= 0 ||
        fSampleRateInHz <= 0 ||
        fModWidthInS <= 0 || fModWidthInS > fDelayInS ||
        fModFreqInHz <= 0 || fModFreqInHz > fSampleRateInHz ||
        iNumChannels <=0) {
        return Error_t::kFunctionInvalidArgsError;
    }
    
    m_bIsInitialized = true;
    m_fDelayInS = fDelayInS;
    m_fSampleRateInHz = fSampleRateInHz;
    m_fModWidthInS = fModWidthInS;
    m_fModFreqInHz = fModFreqInHz;
    m_iNumChannels = iNumChannels;
    m_pCLfo = new CLfo((m_fModFreqInHz/m_fSampleRateInHz), m_fSampleRateInHz);
    m_pCLfo -> process();
    
    m_ppCRingBuffer = new CRingBuffer<float>* [m_iNumChannels];
    for (int i = 0; i < m_iNumChannels; i++) {
        m_ppCRingBuffer[i] = new CRingBuffer<float>(int(2 + m_fDelayInS * m_fSampleRateInHz + m_fModWidthInS * 2 * m_fSampleRateInHz));
    }

    
    return Error_t::kNoError;
}

Error_t CVibrato::reset () {
    delete m_pCVibrato;
    m_pCVibrato = 0;
    delete m_pCLfo;
    m_pCLfo = 0;
    if (m_bIsInitialized == true) {
        for (int i = 0; i < m_iNumChannels; i++) {
            delete m_ppCRingBuffer[i];
        }
        delete [] m_ppCRingBuffer;
        m_ppCRingBuffer = 0;
    }
    m_fDelayInS = 0;
    m_fSampleRateInHz = 0;
    m_fModWidthInS = 0;
    m_fModFreqInHz = 0;
    m_iNumChannels = 0;
    m_bIsInitialized = false;
    return Error_t::kNoError;
}

Error_t CVibrato::process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames) {    float fModAmp, fDelay = 0;
    for(int i = 0; i < m_iNumChannels; i++) {
        for(int j = 0; j < iNumberOfFrames; j++) {
            fModAmp = m_pCLfo -> returnLfoVal();
            fDelay = 1 + m_fDelayInS * m_fSampleRateInHz + m_fModWidthInS * m_fSampleRateInHz * fModAmp;
            m_ppCRingBuffer[i] -> putPostInc(ppfInputBuffer[i][j]);
            ppfOutputBuffer[i][j] = m_ppCRingBuffer[i] -> get(fDelay);
            m_ppCRingBuffer[i] -> getPostInc();
        }
    }
    
    return Error_t::kNoError;
    
}
