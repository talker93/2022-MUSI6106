
#if !defined(__FastConv_HEADER_INCLUDED__)
#define __FastConv_HEADER_INCLUDED__

#pragma once

#include "ErrorDef.h"
#include "RingBuffer.h"
#include "Fft.h"

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
    static Error_t create(CFastConv*& pCFastConv);
    
    static Error_t destroy(CFastConv*& pCFastConv);
    
    Error_t init(float* pfImpulseResponse, int iLengthOfIr, int iBlockLength = 8192, ConvCompMode_t eCompMode = kFreqDomain);

    /*! resets all internal class members
    \return Error_t
    */
    Error_t reset ();

    /*! computes the output with reverb
    \param pfOutputBuffer (mono)
    \param pfInputBuffer (mono)
    \param iLengthOfBuffers can be anything from 1 sample to 10000000 samples
    \return Error_t
    */
    Error_t process(float* pfOutputBuffer, const float* pfInputBuffer, int iLengthOfBuffers);

    /*! return the 'tail' after processing has finished (identical to feeding in zeros
    \param pfOutputBuffer (mono)
    \return Error_t
    */
    Error_t flushBuffer(float* pfOutputBuffer);
    
    /*! return the FFT results with length: m_iDataLength
     \param pfInput (mono)
     \return Error_t
    */
    Error_t getRealAndImag(float* pfOutReal, float* pfOutImag, float* pfInput);
    
    /*! return the FFT multiplication results with length: m_iDataLength
     \param pfOut (mono), pfMul_1 (mono), pfMul_2 (mono)
     \return Error_t
    */
    Error_t fftMul(float* pfOut, float* pfMul_1, float* pfMul_2);
    
    /*! return the FFT multiplication results and store in buffer
     \param buffer: iDataLength
     */
    Error_t fftBlock(float* buffer, float* block1, float* block2, int blockLen_1, int blockLen_2);

private:
    int m_iBlockLength;
    int m_iOverlapLength;
    int m_iDataLength;
    int m_iIRLength;
    int m_iFftLength;
    float** m_ppfImpulseBlock;
    float** m_ppfInputBlock;
    float** m_ppfBuffer;
    CRingBuffer<float>* m_pCRingBuff = 0;
    float* m_pfIR;
    bool m_bIsInitialized;
    /* 0-> timeDomain, 1-> freqDomain*/
    int m_iCompType;
    int m_iDivNums;
    int m_iCurDivNum = 0;
    CFastConv* m_pCFastConv = 0;
    Error_t checkData(const float* pfData, int dataLength);
    Error_t checkData(float*& pfData, int dataLength, bool init = false);
};


#endif
