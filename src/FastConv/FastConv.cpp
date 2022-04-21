
#include "FastConv.h"
#include <iostream>
#include <cmath>
using namespace std;

CFastConv::CFastConv( void ) :
    m_bIsInitialized(false),
    m_iBlockLength(0),
    m_iOverlapLength(0),
    m_iDataLength(0),
    m_iIRLength(0),
    m_pCRingBuff(0),
    m_pfIR(0),
    m_iCompType(0)
{
    this -> reset();
}

CFastConv::~CFastConv( void )
{
    this -> reset();
}

Error_t CFastConv::create(CFastConv*& pCFastConv)
{
    pCFastConv = new CFastConv;
    if (!pCFastConv)
        return Error_t::kMemError;
    return Error_t::kNoError;
}

Error_t CFastConv::destroy(CFastConv *&pCFastConv)
{
    if (!pCFastConv)
        return Error_t::kUnknownError;
    
    pCFastConv->reset();
    
    delete pCFastConv;
    pCFastConv = 0;
    
    return Error_t::kNoError;
}

Error_t CFastConv::init(float *pfImpulseResponse, int iLengthOfIr, int iBlockLength /*= 8192*/, ConvCompMode_t eCompMode /*= kFreqDomain*/)
{
    switch(eCompMode)
    {
        case(kTimeDomain):
            m_iBlockLength = iBlockLength;
            m_iIRLength = iLengthOfIr;
            m_iOverlapLength = iLengthOfIr - 1;
            m_iDataLength = iBlockLength + iLengthOfIr - 1;
            m_pCRingBuff = new CRingBuffer<float>(m_iDataLength);
            m_pfIR = pfImpulseResponse;
            break;
            
        case(kFreqDomain):
            m_iBlockLength = iBlockLength;
            m_iIRLength = iLengthOfIr;
            m_iOverlapLength = iLengthOfIr - 1;
            m_iDataLength = iBlockLength + iLengthOfIr - 1;
            m_pCRingBuff = new CRingBuffer<float>(m_iDataLength);
            m_pfIR = pfImpulseResponse;
            m_iCompType = 1;
            m_ppfImpulseBlock = new float*[m_iDivDimes];
            for (int i = 0; i < m_iDivDimes; i++)
            {
                m_ppfImpulseBlock[i] = new float[m_iBlockLength];
                for (int j = 0; j < m_iBlockLength; j++)
                {
                    m_ppfImpulseBlock[i][j] = m_pfIR[i+j];
                }
            }
            
            
            break;

        case kNumConvCompModes:
            return Error_t::kInvalidString;
    }
    return Error_t::kNoError;
}

Error_t CFastConv::reset()
{
    if(m_bIsInitialized)
    {
        delete m_pCRingBuff;
        m_pCRingBuff = 0;
        
        delete m_pfIR;
        m_pfIR = 0;
    }
    
    m_iBlockLength = 0;
    m_iIRLength = 0;
    m_iOverlapLength = 0;
    m_iDataLength = 0;
    m_iCompType = 0;
    
    return Error_t::kNoError;
}

Error_t CFastConv::getRealAndImag (float* pfOutReal, float* pfOutImag, float* pfInput)
{
    float* pfSpec = new float [static_cast<int>(m_iFftLength)]();
    
    CFft* pCFft;
    CFft::createInstance(pCFft);
    // why am I getting wrong data here, if init(m_iDataLength) ?
    pCFft->initInstance(static_cast<int>(m_iFftLength));
    pCFft->doFft(pfSpec, pfInput);
    pCFft->splitRealImag(pfOutReal, pfOutImag, pfSpec);
    pCFft->resetInstance();
    CFft::destroyInstance(pCFft);
    
    return Error_t::kNoError;
}

Error_t CFastConv::fftMul(float *pfMulOut, float *pfMul_1, float* pfMul_2)
{
    
    float* pfMul_1_Real = new float [static_cast<int>(m_iFftLength/2+1)]();
    float* pfMul_1_Imag = new float [static_cast<int>(m_iFftLength/2+1)]();
    float* pfMul_2_Real = new float [static_cast<int>(m_iFftLength/2+1)]();
    float* pfMul_2_Imag = new float [static_cast<int>(m_iFftLength/2+1)]();
    float* pfOut_Real = new float [static_cast<int>(m_iFftLength/2+1)]();
    float* pfOut_Imag = new float [static_cast<int>(m_iFftLength/2+1)]();
    float* pfOut = new float [static_cast<int>(m_iFftLength)]();

    getRealAndImag(pfMul_1_Real, pfMul_1_Imag, pfMul_1);
    getRealAndImag(pfMul_2_Real, pfMul_2_Imag, pfMul_2);
    
    for(int i = 0; i < static_cast<int>(m_iFftLength); i++)
    {
        pfOut_Real[i] = pfMul_1_Real[i] * pfMul_2_Real[i] - pfMul_1_Imag[i] * pfMul_2_Imag[i];
        pfOut_Imag[i] = pfMul_1_Real[i] * pfMul_2_Imag[i] + pfMul_2_Real[i] * pfMul_1_Imag[i];
    }
    
    CFft* pCFft_Mul;
    CFft::createInstance(pCFft_Mul);
    pCFft_Mul->initInstance(static_cast<int>(m_iFftLength));
    pCFft_Mul->mergeRealImag(pfOut, pfOut_Real, pfOut_Imag);
    pCFft_Mul->doInvFft(pfMulOut, pfOut);
    pCFft_Mul->resetInstance();
    CFft::destroyInstance(pCFft_Mul);
    
    return Error_t::kNoError;
    
}

Error_t CFastConv::checkData(const float *pfData, int dataLength)
{
    cout << "-----------------data inspector-----------" << endl;
    cout << "Data until " << dataLength << " : "<< endl;
    for(int i = 0; i < dataLength; i++)
        cout << pfData[i] << ", ";
    cout << "-----------------inspector ends------------" << endl;
}

Error_t CFastConv::checkData(float*& pfData, int dataLength, bool init /*= false*/)
{
    cout << "-----------------data inspector-----------" << endl;
    cout << "Data until " << dataLength << " : "<< endl;
    for(int i = 0; i < dataLength; i++)
        cout << pfData[i] << ", ";
    cout << "-----------------inspector ends------------" << endl;
    if(init == true)
    {
        delete [] pfData;
        pfData = 0;
    }
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
{
    if(m_iCompType == 1)
    {

        
        // -------------------------------------


//        if(m_iCurDivNum % m_iDivDimes == 0) {
//
//        };
//
        float conv = 0;
        memset(pfOutputBuffer, 0, iLengthOfBuffers*sizeof(float));

        // tail adding
        for (int n = 0; n < m_iIRLength-1; n++)
            pfOutputBuffer[n] = m_pCRingBuff->get(n+iLengthOfBuffers);

        // buffer reset
        m_pCRingBuff->reset();

        // direct conv
//        for ( int n = 0; n < iLengthOfBuffers + m_iIRLength - 1; n++)
//        {
//            for ( int k = 0; k < iLengthOfBuffers; k++)
//            {
//                if((n-k) >= 0 && (n-k) < m_iIRLength)
//                {
//                    conv += pfInputBuffer[k] * m_pfIR[n-k];
//                }
//            }
//            m_pCRingBuff->putPostInc(conv);
//            conv = 0;
//        }
        
        // get the minimum FFT number
        for( int i = 0; i < 100; i++)
        {
            if (m_iDataLength < pow(2, i+1) && m_iDataLength > pow(2, i))
            {
                m_iFftLength = pow(2, i+1);
                break;
            }
        }

        float* pfIirBuffer = new float [static_cast<int>(m_iFftLength)]();
        memcpy(pfIirBuffer, m_pfIR, m_iIRLength*sizeof(float));

        float* pfInput = new float [static_cast<int>(m_iFftLength)]();
        memcpy(pfInput, pfInputBuffer, iLengthOfBuffers*sizeof(float));

        float* pfMulResult = new float [static_cast<int>(m_iFftLength)]();

        fftMul(pfMulResult, pfInput, pfIirBuffer);

        m_pCRingBuff->putPostInc(pfMulResult, m_iDataLength);
        
        // output
        for (int n = 0; n < iLengthOfBuffers; n++)
            pfOutputBuffer[n] += m_pCRingBuff->get(n);

    }
    else if(m_iCompType == 0)
    {
        // reset outputbuffer
        memset(pfOutputBuffer, 0, iLengthOfBuffers*sizeof(pfOutputBuffer[0]));
        
        float tail;
        
        m_pCRingBuff -> putPostInc(pfInputBuffer, iLengthOfBuffers);
        
        for (int n = 0; n < iLengthOfBuffers + m_iIRLength - 1; n++)
        {
            for (int k = 0; k < iLengthOfBuffers; k++)
            {
                if((n-k) >= 0 && (n-k) < m_iIRLength && n < iLengthOfBuffers)
                    pfOutputBuffer[n] += m_pCRingBuff->get(k) * m_pfIR[n - k];
                else if((n-k) >= 0 && (n-k) < m_iIRLength && n >= iLengthOfBuffers)
                    tail = (m_pCRingBuff->get(k) * m_pfIR[n-k] + tail);
            }
            // put buffer in the tail
            if(n >= iLengthOfBuffers)
            {
                m_pCRingBuff->putPostInc(tail);
                tail = 0;
            }
            // add buffer in begin
            if(n < m_iOverlapLength)
                pfOutputBuffer[n] += m_pCRingBuff->get(n+iLengthOfBuffers);
        }
        
        m_pCRingBuff->setWriteIdx(0);
    }
    

    return Error_t::kNoError;
}

Error_t CFastConv::flushBuffer(float* pfOutputBuffer)
{
    return Error_t::kNoError;
}
