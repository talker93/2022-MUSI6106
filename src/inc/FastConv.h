
#if !defined(__FastConv_HEADER_INCLUDED__)
#define __FastConv_HEADER_INCLUDED__

#pragma once

#include "ErrorDef.h"
#include "RingBuffer.h"
#include "Fft.h"
#include "Vector.h"
#include "Util.h"
#include <math.h>
#include <iostream>
using namespace std;

/*! \brief interface for fast convolution
*/
class CFastConv
{
public:
    enum ConvCompMode_t
    {
        kTimeDomain,
        kFreqDomain,

        kNumConvCompModes
    };

    CFastConv(void);
    virtual ~CFastConv(void);

    /*! initializes the class with the impulse response and the block length
    \param pfImpulseResponse impulse response samples (mono only)
    \param iLengthOfIr length of impulse response
    \param iBlockLength processing block size
    \return Error_t
    */
    static Error_t create(CFastConv*& CFastConv);
    
    static Error_t destroy(CFastConv*& pCFastConv);
    
    Error_t init(float* pfImpulseResponse, int iLengthOfIr, int iBlockLength = 8192, ConvCompMode_t eCompMode = kFreqDomain);

    /*! resets all internal class members
    \return Error_t
    */
    Error_t reset ();

    /*! return the 'tail' after processing has finished (identical to feeding in zeros
    \param pfOutputBuffer (mono)
    \return Error_t
    */
    Error_t flushBuffer(float* pfOutputBuffer);
    
    /*! computes the output with reverb
    \param pfOutputBuffer (mono)
    \param pfInputBuffer (mono)
    \param iLengthOfBuffers can be anything from 1 sample to 10000000 samples
    \return Error_t
    */
    Error_t process(float* pfOutputBuffer, const float* pfInputBuffer, int iLengthOfBuffers);
    
    /*!
     \param pfMulOut: iFftLength
     \param pfMul1: m_iBlockLength
     \param pfMul2: m_iBlockLength
     */
    Error_t fftMul(float* pfMulOut, const float* pfMul1, const float* pfMul2, int pfMulLength1, int pfMulLength2);

private:
    int m_iBlockLength;
    int m_iOverlapLength;
    int m_iDataLength;
    int m_iIRLength;
    int m_iFftLength;
    CRingBuffer<float>* m_pCRingBuff = 0;
    CRingBuffer<float>** m_ppCRingBuffFft = 0;
    float* m_pfIR;
    float** m_ppfIRFft;
    bool m_bIsInitialized;
    CFastConv::ConvCompMode_t m_eCompType;
    int m_iDivNums;
    int m_iCurBlockIdx;
    float** m_ppfMulBuffer;
    CFft::complex_t** m_ppfMulFftBuffer;
    float** m_ppfMulSplitBuffer;
    float* m_pfOutputBufer;
    CFft* m_pCFft = 0;
    Error_t checkData(const float* pfData, int dataLength);
    Error_t checkData(float*& pfData, int dataLength, bool init = false);
    Error_t checkRingBuffer();
    
};


#endif
