
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
    reset();
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
            m_ppfInputBlock = new float*[m_iDivNums+1];
            for(int i = 0; i < m_iDivNums+1; i++)
                m_ppfInputBlock[i] = new float [m_iBlockLength]();
            m_ppfImpulseBlock = new float*[m_iDivNums];
            for (int i = 0; i < m_iDivNums; i++)
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

Error_t CFastConv::fftBlock(float *buffer, float *block1, float *block2, int blockLen_1, int blockLen_2)
{
    if(blockLen_1 > m_iBlockLength || blockLen_2 > m_iBlockLength)
        assert("something wrong with your input block number");
    
    memset(buffer, 0, m_iDataLength*sizeof(float));
    
    // get the minimum FFT number
    for( int i = 0; i < 100; i++)
    {
        if (m_iDataLength < pow(2, i+1) && m_iDataLength > pow(2, i))
        {
            m_iFftLength = pow(2, i+1);
            break;
        }
    }

    float* blockBuffer1 = new float [static_cast<int>(m_iFftLength)]();
    memcpy(blockBuffer1, block1, blockLen_1*sizeof(float));

    float* blockBuffer2 = new float [static_cast<int>(m_iFftLength)]();
    memcpy(blockBuffer2, block2, blockLen_2*sizeof(float));

    fftMul(buffer, blockBuffer1, blockBuffer2);
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
{
    if(m_iCompType == 1)
    {
        m_iCurDivNum = m_iCurDivNum % (m_iDivNums+1);
        memset(pfOutputBuffer, 0, iLengthOfBuffers);
        
        // get new input block
        memcpy(m_ppfInputBlock[m_iCurDivNum], pfInputBuffer, iLengthOfBuffers*sizeof(float));
        
        int num = 0;
        int k = m_iDivNums-1;
        for ( int n = 0; n < m_iDivNums; n++)
        {
            fftBlock(m_ppfBuffer[num], m_ppfInputBlock[n], m_ppfImpulseBlock[k], m_iBlockLength, m_iBlockLength);
            k--;
            num++;
        }
        k = m_iDivNums-1;
        for ( int n = 1; n < m_iDivNums; n++)
        {
            fftBlock(m_ppfBuffer[num], m_ppfInputBlock[n], m_ppfImpulseBlock[k], m_iBlockLength, m_iBlockLength);
            k--;
            num++;
        }
        
        // get tail
        for (int i = 0; i < num/2; i++)
        {
            for (int j = m_iBlockLength; j < m_iDataLength; j++)
            {
                pfOutputBuffer[j-m_iBlockLength] += m_ppfBuffer[i][j];
            }
        }
        // get head
        for (int i = 0; (i >= num/2)&&(i<num); i++)
        {
            for (int j = 0; j < m_iBlockLength; j++)
            {
                pfOutputBuffer[j] += m_ppfBuffer[i][j];
            }
        }
        
        m_iCurDivNum++;

    }
    else if(m_iCompType == 0)
    {
    // reset outputbuffer
        for (int i = 0; i < iLengthOfBuffers; i++)
            pfOutputBuffer[i] = 0;
        
        
        for (int i = 0; i < iLengthOfBuffers; ++i) {
            m_pCRingBuff->putPostInc(pfInputBuffer[i]);
            for (int j = 0; j < m_iIRLength; ++j) {
                pfOutputBuffer[i] += m_pCRingBuff->get(m_iIRLength-j) * m_pfIR[j];
            }
            m_pCRingBuff->getPostInc();
        }
    }
    
    return Error_t::kNoError;
}

Error_t CFastConv::flushBuffer(float* pfOutputBuffer)
{
    for (int i = 0; i < m_iIRLength; ++i) {
        m_pCRingBuff->putPostInc(0.F);
        for (int j = 0; j < m_iIRLength; ++j) {
            pfOutputBuffer[i] += m_pCRingBuff->get(m_iIRLength-j) * m_pfIR[j];
        }
        m_pCRingBuff->getPostInc();
    }
    return Error_t::kNoError;
}
