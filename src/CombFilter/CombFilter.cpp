#include "Util.h"
#include "CombFilterIf.h"
#include "CombFilter.h"

CCombFilterBase::CCombFilterBase() {
    m_fGain = 0;
    m_iDelay = 0;
    m_iMaxDelayInSamples = 0;
    m_iNumChannels = 0;
    m_ppRingBuffer = 0;
}

CCombFilterBase::CCombFilterBase(float fMaxDelayInSamples, int iNumChannels) {
    m_ppRingBuffer = new CRingBuffer<float>*[iNumChannels];
    for(int i=0; i<iNumChannels; i++) {
        m_ppRingBuffer[i] = new CRingBuffer<float>(fMaxDelayInSamples);
    }
    m_iMaxDelayInSamples = static_cast<int>(fMaxDelayInSamples);
    m_iNumChannels = iNumChannels;
    m_fGain = 1;
    m_iDelay = 1;
}

CCombFilterBase::~CCombFilterBase() {
    
}

Error_t CCombFilterBase::setParam(CCombFilterIf::FilterParam_t eParam, float fParamValue, float fSampleRate) {
    if(eParam == CCombFilterIf::kParamGain) {
        m_fGain = fParamValue;
    } else {
        m_iDelay = static_cast<int>(fParamValue * fSampleRate);
    }
    return Error_t::kNoError;
}

float CCombFilterBase::getParam(CCombFilterIf::FilterParam_t eParam) const {
    if(eParam == CCombFilterIf::kParamGain) {
        return m_fGain;
    } else {
        return m_iDelay;
    }
}


// get values from parent class
CCombFilterFIR::CCombFilterFIR(float fMaxDelayInSamples, int iNumChannels) : CCombFilterBase(fMaxDelayInSamples, iNumChannels) {}

CCombFilterIIR::CCombFilterIIR(float fMaxDelayInSamples, int iNumChannels) : CCombFilterBase(fMaxDelayInSamples, iNumChannels) {}

CCombFilterFIR::~CCombFilterFIR() {}
CCombFilterIIR::~CCombFilterIIR() {}

Error_t CCombFilterFIR::process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames) {
    for(int i=0; i<m_iNumChannels; i++) {
        m_ppRingBuffer[i]->setWriteIdx(m_iDelay);
        for(int j=0; j<iNumberOfFrames; j++) {
            ppfOutputBuffer[i][j] = ppfInputBuffer[i][j] + m_fGain * (m_ppRingBuffer[i]->getPostInc());
            m_ppRingBuffer[i]->putPostInc(ppfInputBuffer[i][j]);
        }
    }
}

Error_t CCombFilterIIR::process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames) {
    for(int i=0; i<m_iNumChannels; i++) {
        m_ppRingBuffer[i]->setWriteIdx(m_iDelay);
        for(int j=0; j<iNumberOfFrames; j++) {
            ppfOutputBuffer[i][j] = ppfInputBuffer[i][j] + m_fGain * (m_ppRingBuffer[i]->getPostInc());
            m_ppRingBuffer[i]->putPostInc(ppfOutputBuffer[i][j]);
        }
    }
}


