
#include "FastConv.h"
#include <iostream>
#include <cmath>
using namespace std;

CFastConv::CFastConv( void ) :
    m_iBlockLength(0),
    m_iOverlapLength(0),
    m_iDataLength(0),
    m_iIRLength(0),
    m_iFftLength(0),
    m_pCRingBuff(0),
    m_ppCRingBuffFft(0),
    m_pfIR(0),
    m_ppfIRFft(0),
    m_bIsInitialized(false),
    m_iCompType(0),
    m_iDivNums(0),
    m_iCurBlockIdx(0),
    m_ppfMulBuffer(0),
    m_ppfMulSplitBuffer(0),
    m_pfOutputBufer(0),
    m_pCFft(0)
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
            m_iDataLength = iBlockLength + 1 - iLengthOfIr;
            m_pCRingBuff = new CRingBuffer<float>(iBlockLength);
            m_pCRingBuff->setReadIdx(0);
            m_pCRingBuff->setWriteIdx(0);
            m_pfIR = pfImpulseResponse;
            break;
            
        case(kFreqDomain):
            m_iBlockLength = iBlockLength;
            m_iCompType = 1;
            m_iIRLength = iLengthOfIr;
            m_iDivNums = ceil(static_cast<float>(iLengthOfIr)/static_cast<float>(iBlockLength));

            // find the nearest FFT length
            for( int i = 1; i < 20; i++)
            {
                if(pow(2, i) > m_iBlockLength)
                {
                    m_iFftLength = pow(2, i);
                    break;
                }
            }
            
            // blocking impluse response
            m_ppfIRFft = new float * [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
            {
                m_ppfIRFft[i] = new float [m_iBlockLength]();
                copy(pfImpulseResponse+i*m_iBlockLength, pfImpulseResponse+(i+1)*m_iBlockLength, m_ppfIRFft[i]);
            }
            
            // create buffer
            m_ppCRingBuffFft = new CRingBuffer<float>* [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
                m_ppCRingBuffFft[i] = new CRingBuffer<float> (m_iBlockLength*2*m_iDivNums);
            
            m_pfOutputBufer = new float [m_iFftLength]();
            
            m_ppfMulBuffer = new float* [4];
            for(int i = 0; i < 4; i++)
                m_ppfMulBuffer[i] = new float [m_iFftLength]();
            
            m_ppfMulSplitBuffer = new float* [6];
            for(int i = 0; i < 6; i++)
                m_ppfMulSplitBuffer[i] = new float [(m_iFftLength/2)+1]();
            
            m_pCFft->createInstance(m_pCFft);
            m_pCFft->initInstance(m_iFftLength);
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
        delete [] m_ppCRingBuffFft;
        m_ppCRingBuffFft = 0;
        delete m_pfIR;
        m_pfIR = 0;
        delete [] m_ppfIRFft;
        m_ppfIRFft = 0;
        delete [] m_ppfMulBuffer;
        m_ppfMulBuffer = 0;
        delete[] m_ppfMulSplitBuffer;
        m_ppfMulSplitBuffer = 0;
        delete m_pfOutputBufer;
        m_pfOutputBufer = 0;
        m_pCFft->resetInstance();
        m_pCFft = 0;
    }
    m_iBlockLength = 0;
    m_iOverlapLength = 0;
    m_iDataLength = 0;
    m_iIRLength = 0;
    m_iFftLength = 0;
    m_pCRingBuff = 0;
    m_ppCRingBuffFft = 0;
    m_pfIR = 0;
    m_ppfIRFft = 0;
    m_iCompType = 0;
    m_iDivNums = 0;
    m_iCurBlockIdx = 0;
    m_ppfMulBuffer = 0;
    m_ppfMulSplitBuffer = 0;
    m_pfOutputBufer = 0;
    m_pCFft = 0;
    m_bIsInitialized = false;
    
    return Error_t::kNoError;
}

Error_t CFastConv::fftMul(float *pfMulOut, const float *pfMul1, const float *pfMul2, int pfMulLength1, int pfMulLength2)
{
    copy(pfMul1, pfMul1+pfMulLength1, m_ppfMulBuffer[0]);
    copy(pfMul2, pfMul2+pfMulLength2, m_ppfMulBuffer[1]);
    
    // m_ppfMulBuffer: 2, 3 -> output; 0, 1 -> input
    m_pCFft->doFft(m_ppfMulBuffer[2], m_ppfMulBuffer[0]);
    m_pCFft->doFft(m_ppfMulBuffer[3], m_ppfMulBuffer[1]);
    
    // m_ppfMulSplitBuffer: 0, 2 -> Real; 1, 3 -> Imag
    m_pCFft->splitRealImag(m_ppfMulSplitBuffer[0], m_ppfMulSplitBuffer[1], m_ppfMulBuffer[2]);
    m_pCFft->splitRealImag(m_ppfMulSplitBuffer[2], m_ppfMulSplitBuffer[3], m_ppfMulBuffer[3]);

    // m_ppfMulSplitBuffer: 4 -> OutputReal; 5 -> OutputImag
    for(int i = 0; i < m_iBlockLength+1; i++)
    {
        m_ppfMulSplitBuffer[4][i] = m_ppfMulSplitBuffer[0][i] * m_ppfMulSplitBuffer[2][i]
                                    + m_ppfMulSplitBuffer[1][i] * m_ppfMulSplitBuffer[3][i];
        m_ppfMulSplitBuffer[5][i] = m_ppfMulSplitBuffer[0][i] * m_ppfMulSplitBuffer[3][i]
                                    + m_ppfMulSplitBuffer[1][i] * m_ppfMulSplitBuffer[2][i];
    }
    
    m_pCFft->mergeRealImag(pfMulOut, m_ppfMulSplitBuffer[4], m_ppfMulSplitBuffer[5]);
    
    return Error_t::kNoError;
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
{
    
    if(m_iCompType == 1)
    {
        int tailBufferIdx;
        int tailPosition;
        int headerBufferIdx;
        int headerPosition;
        
        // tail addition
        // all ring buffer has length of 2 * m_iBlockLength
        // the tail stores in the 2nd half of ring buffer
        tailBufferIdx = m_iCurBlockIdx;
        for ( int i = 0; i < m_iDivNums; i++)
        {
            tailPosition = m_iDivNums - 1 - i;
            m_ppCRingBuffFft[tailBufferIdx] -> setReadIdx((tailPosition*2+1)*m_iBlockLength);
            for (int j = 0; j < m_iBlockLength; j++)
                pfOutputBuffer[j] += m_ppCRingBuffFft[tailBufferIdx] -> getPostInc();
            tailBufferIdx = (tailBufferIdx+1) % m_iDivNums;
        }
        
        // calculation and store in ringBuffer
        // data length is m_iFftLength
        for(int i = 0; i < m_iDivNums; i++)
        {
            fftMul(m_pfOutputBufer, pfInputBuffer, m_ppfIRFft[i], m_iBlockLength, m_iBlockLength);
            m_ppCRingBuffFft[m_iCurBlockIdx] -> put(m_pfOutputBufer, m_iFftLength);
            m_ppCRingBuffFft[m_iCurBlockIdx] -> setWriteIdx(2*m_iBlockLength*(i+1));
        }
        
        // header addition
        // the header stores in the 1st half of ring buffer
        headerBufferIdx = (m_iCurBlockIdx + 1) % m_iDivNums;
        for ( int i = 0; i < m_iDivNums; i++)
        {
            headerPosition = m_iDivNums - 1 - i;
            m_ppCRingBuffFft[headerBufferIdx] -> setReadIdx(headerPosition*2*m_iBlockLength);
            for (int j = 0; j < m_iBlockLength; j++)
                pfOutputBuffer[j] += m_ppCRingBuffFft[headerBufferIdx] -> getPostInc();
            headerBufferIdx = (headerBufferIdx+1) % m_iDivNums;
        }
        m_iCurBlockIdx = (m_iCurBlockIdx+1) % m_iDivNums;
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
/*
Error_t CFastConv::flushBuffer(float* pfOutputBuffer)
{
    switch (m_iCompType) {
        case 0:
            for (int i = 0; i < m_iIRLength; ++i) {
                m_pCRingBuff->putPostInc(0.F);
                for (int j = 0; j < m_iIRLength; ++j) {
                    // sorry for comment this block, we will discuss about it.
//                    pfOutputBuffer[i] += m_pCRingBuff->get(m_iIRLength-j) * m_pfIR[j];
                }
                m_pCRingBuff->getPostInc();
            }
            return Error_t::kNoError;
            break;
            
        default:
            break;
    }
}
*/
 
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
