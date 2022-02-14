//
//  CombFilter.cpp
//  AudioFileIO
//
//  Created by 江山 on 2/13/22.
//
#include "MUSI6106Config.h"

#include <cassert>
#include <iostream>
#include <string.h>
#include <vector>
#include <fstream>

#include "Util.h"
#include "CombFilter.h"
#include "ErrorDef.h"

//#ifdef WITH_SNDLIB
//#include "sndlib.h"
//#endif //WITH_SNDLIB

static const char*  kCMyProjectBuildDate = __DATE__;


CCombFilter::CCombFilter () :
    m_bIsInitialized(false),
    m_pCCombFilter(0),
    m_fSampleRate(0)
{
    // this should never hurt
    this->reset ();
}


CCombFilter::~CCombFilter ()
{
    this->reset ();
}

const int  CCombFilter::getVersion (const Version_t eVersionIdx)
{
    int iVersion = 0;

    switch (eVersionIdx)
    {
    case kMajor:
        iVersion    = MUSI6106_VERSION_MAJOR;
        break;
    case kMinor:
        iVersion    = MUSI6106_VERSION_MINOR;
        break;
    case kPatch:
        iVersion    = MUSI6106_VERSION_PATCH;
        break;
    case kNumVersionInts:
        iVersion    = -1;
        break;
    }

    return iVersion;
}
const char*  CCombFilter::getBuildDate ()
{
    return kCMyProjectBuildDate;
}

Error_t CCombFilter::create (CCombFilter*& pCCombFilter)
{
    return Error_t::kNoError;
}

Error_t CCombFilter::destroy (CCombFilter*& pCCombFilter)
{
    return Error_t::kNoError;
}

Error_t CCombFilter::init (CombFilterType_t eFilterType, float fMaxDelayLengthInS, float fSampleRateInHz, int iNumChannels)
{
    m_filterType = eFilterType;
    m_fSampleRate = fSampleRateInHz;
    return Error_t::kNoError;
}

Error_t CCombFilter::reset ()
{
    return Error_t::kNoError;
}

Error_t CCombFilter::process (float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames)
{
    return Error_t::kNoError;
}

Error_t CCombFilter::setParam (FilterParam_t eParam, float fParamValue)
{
    if (eParam == 'kParamGain')
    {
        m_gain = fParamValue;
    }
    else if (eParam == 'kParamDelay')
    {
        m_delay = fParamValue;
    }
    return Error_t::kNoError;
}

float CCombFilter::getParam (FilterParam_t eParam) const
{
    return 0;
}
