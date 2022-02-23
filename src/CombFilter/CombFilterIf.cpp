#include "ErrorDef.h"
#include "Util.h"
#include "CombFilterIf.h"
#include "CombFilter.h"

CCombFilterIf::CCombFilterIf() {
    m_bIsInitialized = false;
    m_pCCombFilter = 0;
    m_fSampleRate = 0;
}

CCombFilterIf::~CCombFilterIf() {
}

Error_t CCombFilterIf::create (CCombFilterIf*& pCCombFilterIf) {
    pCCombFilterIf = new CCombFilterIf();
    if(!pCCombFilterIf) {
        return Error_t::kMemError;
    }
    return Error_t::kNoError;
}

Error_t CCombFilterIf::destroy(CCombFilterIf *&pCCombFilterIf) {
    delete pCCombFilterIf;
    pCCombFilterIf = 0;
    return Error_t::kNoError;
}

Error_t CCombFilterIf::init(CombFilterType_t eFilterType, float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels) {
    m_bIsInitialized = true;
    m_fSampleRate = fSampleRateInHz;
    m_fMaxDelayInS = fMaxDelayLengthInS;
    m_fMaxDelayInSamples = (fMaxDelayLengthInS * fSampleRateInHz);
    m_iNumChannels = iNumChannels;
    m_eFilterType = eFilterType;
    if(m_eFilterType == kCombFIR) {
        m_pCCombFilter = new CCombFilterFIR(m_fMaxDelayInSamples, m_iNumChannels);
    } else {
        m_pCCombFilter = new CCombFilterIIR(m_fMaxDelayInSamples, m_iNumChannels);
    }
    return Error_t::kNoError;
}

Error_t CCombFilterIf::reset() {
    m_bIsInitialized = false;
    m_fSampleRate = 48000;
    m_fMaxDelayInS = 1;
    m_fMaxDelayInSamples = (m_fSampleRate * m_fMaxDelayInS);
    m_iNumChannels = 1;
    m_eFilterType = kCombFIR;
    return Error_t::kNoError;
}

Error_t CCombFilterIf::setParam(FilterParam_t eParam, float fParamValue) {
    m_pCCombFilter->setParam(eParam, fParamValue, m_fSampleRate);
}

float CCombFilterIf::getParam(FilterParam_t eParam) const {
    m_pCCombFilter->getParam(eParam);
}

Error_t CCombFilterIf::process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames) {
    m_pCCombFilter->process(ppfInputBuffer, ppfOutputBuffer, iNumberOfFrames);
    return Error_t::kNoError;
}










