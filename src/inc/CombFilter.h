#if !defined(__CombFilter_hdr__)
#define __CombFilter_hdr__

#include "ErrorDef.h"
#include "CombFilterIf.h"
#include "RingBuffer.h"

class CCombFilterBase {
public:
    CCombFilterBase();
    virtual ~CCombFilterBase();
    explicit CCombFilterBase(float fMaxDelayInSamples, int iNumChannels);
    virtual Error_t process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames) = 0;
    Error_t setParam(CCombFilterIf::FilterParam_t eParam, float fParamValue, float fSampleRate);
    float getParam(CCombFilterIf::FilterParam_t eParam) const;
    
    CRingBuffer<float> **m_ppRingBuffer;
    float m_fGain;
    int m_iDelay;
    int m_fMaxDelayInSamples;
    int m_iMaxDelayInSamples;
    int m_iNumChannels;
};

class CCombFilterFIR : public CCombFilterBase {
public:
    CCombFilterFIR();
    CCombFilterFIR(float fMaxDelayInSamples, int iNumChannels);
    ~CCombFilterFIR();
    Error_t process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames);
};

class CCombFilterIIR : public CCombFilterBase {
public:
    CCombFilterIIR();
    CCombFilterIIR(float fMaxDelayInSamples, int iNumChannels);
    ~CCombFilterIIR();
    Error_t process(float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames);
};



#endif // #if !defined(__CombFilter_hdr__)

