
#include "FastConv.h"

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
    m_eCompType(kNumConvCompModes),
    m_iDivNums(0),
    m_iCurBlockIdx(0),
    m_ppfMulBuffer(0),
    m_ppfMulSplitBuffer(0),
    m_ppfMulFftBuffer(0),
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
            m_eCompType = CFastConv::kTimeDomain;
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
            m_eCompType = CFastConv::kFreqDomain;
            m_iIRLength = iLengthOfIr;
            m_iDivNums = ceil(static_cast<float>(iLengthOfIr)/static_cast<float>(iBlockLength));

            m_iFftLength = 2*m_iBlockLength;
            
            // blocking impluse response
            m_ppfIRFft = new float * [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
            {
                if(i < m_iDivNums - 1)
                {
                    m_ppfIRFft[i] = new float [m_iBlockLength]();
                    copy(pfImpulseResponse+i*m_iBlockLength, pfImpulseResponse+(i+1)*m_iBlockLength, m_ppfIRFft[i]);
                }
                else if(i == m_iDivNums -1)
                {
                    m_ppfIRFft[i] = new float [m_iBlockLength]();
                    copy(pfImpulseResponse+i*m_iBlockLength, pfImpulseResponse+i*m_iBlockLength+iLengthOfIr%m_iBlockLength, m_ppfIRFft[i]);
                }
            }
            
            // create buffer
            m_ppCRingBuffFft = new CRingBuffer<float>* [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
                m_ppCRingBuffFft[i] = new CRingBuffer<float> (m_iBlockLength*2*m_iDivNums);
            
            m_pfOutputBufer = new float [m_iFftLength]();
            
            m_ppfMulBuffer = new float* [2];
            for(int i = 0; i < 2; i++)
                m_ppfMulBuffer[i] = new float [m_iFftLength]();
            
            m_ppfMulFftBuffer = new CFft::complex_t* [3];
            for(int i = 0; i < 3; i++)
                m_ppfMulFftBuffer[i] = new CFft::complex_t [m_iFftLength]();
            
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
        delete [] m_ppfMulFftBuffer;
        m_ppfMulFftBuffer = 0;
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
    m_eCompType = kNumConvCompModes;
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
//    checkData(pfMul1, m_iBlockLength);
//    checkData(pfMul2, m_iBlockLength);
    
    //pre scaling
    CVectorFloat::mulC_I(m_ppfMulBuffer[0], m_iFftLength, m_iBlockLength);
    CVectorFloat::mulC_I(m_ppfMulBuffer[1], m_iFftLength, m_iBlockLength);
//    checkData(m_ppfMulBuffer[1], m_iFftLength);
    
    // m_ppfMulBuffer: 2, 3 -> output; 0, 1 -> input
    m_pCFft->doFft(m_ppfMulFftBuffer[0], m_ppfMulBuffer[0]);
    m_pCFft->doFft(m_ppfMulFftBuffer[1], m_ppfMulBuffer[1]);
//    checkData(m_ppfMulFftBuffer[0], m_iFftLength);
//    checkData(m_ppfMulFftBuffer[1], m_iFftLength);
    
    // m_ppfMulSplitBuffer: 0, 2 -> Real; 1, 3 -> Imag
    m_pCFft->splitRealImag(m_ppfMulSplitBuffer[0], m_ppfMulSplitBuffer[1], m_ppfMulFftBuffer[0]);
    m_pCFft->splitRealImag(m_ppfMulSplitBuffer[2], m_ppfMulSplitBuffer[3], m_ppfMulFftBuffer[1]);
//    checkData(m_ppfMulBuffer[3], m_iFftLength);

    // m_ppfMulSplitBuffer: 4 -> OutputReal; 5 -> OutputImag
    for(int i = 0; i < m_iFftLength/2+1; i++)
    {
        m_ppfMulSplitBuffer[4][i] = m_ppfMulSplitBuffer[0][i] * m_ppfMulSplitBuffer[2][i]
                                    - m_ppfMulSplitBuffer[1][i] * m_ppfMulSplitBuffer[3][i];
        m_ppfMulSplitBuffer[5][i] = m_ppfMulSplitBuffer[0][i] * m_ppfMulSplitBuffer[3][i]
                                    + m_ppfMulSplitBuffer[1][i] * m_ppfMulSplitBuffer[2][i];
    }
    
    m_pCFft->mergeRealImag(m_ppfMulFftBuffer[2], m_ppfMulSplitBuffer[4], m_ppfMulSplitBuffer[5]);
    
    m_pCFft->doInvFft(pfMulOut, m_ppfMulFftBuffer[2]);
    
    //post scaling
    CVectorFloat::mulC_I(pfMulOut, 1/static_cast<float>(m_iFftLength), m_iFftLength);
    
    checkData(pfMul1, m_iBlockLength);

    checkData(m_ppfMulSplitBuffer[0], m_iFftLength/2+1);

    checkData(m_ppfMulSplitBuffer[4], m_iFftLength/2+1);

    checkData(pfMulOut, m_iFftLength);
    
    return Error_t::kNoError;
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers )
{
    
    if(m_eCompType == kFreqDomain)
    {
        int tailBufferIdx;
        int tailPosition;
        int headerBufferIdx;
        int headerPosition;
        
        // tail addition
        // there are m_iDivNums ring buffer in total
        // each ring buffer has length of 2 * m_iBlockLength * m_iDivNums
        // the tail stores in the 2nd half of a correspondant part in the ring buffer
        tailBufferIdx = m_iCurBlockIdx;
        for ( int i = 0; i < m_iDivNums; i++)
        {
            tailPosition = m_iDivNums - 1 - i;
            m_ppCRingBuffFft[tailBufferIdx] -> setReadIdx((tailPosition*2+1)*m_iBlockLength);
            for (int j = 0; j < m_iBlockLength; j++)
                pfOutputBuffer[j] += m_ppCRingBuffFft[tailBufferIdx] -> getPostInc();
            tailBufferIdx = (tailBufferIdx+1) % m_iDivNums;
        }
        
        // calculate and store in ringBuffer
        // data length is m_iFftLength
        for(int i = 0; i < m_iDivNums; i++)
        {
            fftMul(m_pfOutputBufer, pfInputBuffer, m_ppfIRFft[i], m_iBlockLength, m_iBlockLength);
            checkData(m_pfOutputBufer, m_iBlockLength);
            m_ppCRingBuffFft[m_iCurBlockIdx] -> put(m_pfOutputBufer, m_iFftLength);
            m_ppCRingBuffFft[m_iCurBlockIdx] -> setWriteIdx(2*m_iBlockLength*(i+1));
        }
        
        // header addition
        // the header stores in the 1st half of a correspondant part in the ring buffer
        headerBufferIdx = (m_iCurBlockIdx + 1) % m_iDivNums;
        for ( int i = 0; i < m_iDivNums; i++)
        {
            headerPosition = m_iDivNums - 1 - i;
            m_ppCRingBuffFft[headerBufferIdx] -> setReadIdx(headerPosition*2*m_iBlockLength);
            for (int j = 0; j < m_iBlockLength; j++)
                pfOutputBuffer[j] += m_ppCRingBuffFft[headerBufferIdx] -> getPostInc();
            headerBufferIdx = (headerBufferIdx+1) % m_iDivNums;
        }
//        cout << "intput" << endl;
//        checkData(pfInputBuffer, m_iBlockLength);
//        cout << "output" << endl;
        checkData(pfOutputBuffer, m_iBlockLength);
        m_iCurBlockIdx = (m_iCurBlockIdx+1) % m_iDivNums;
    }
     else if(m_eCompType == kTimeDomain)
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
    switch (m_eCompType) {
        case 0:
            for (int i = 0; i < m_iIRLength; ++i) {
                m_pCRingBuff->putPostInc(0.F);
                for (int j = 0; j < m_iIRLength; ++j) {
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
    switch (m_eCompType) {
        case 0:
//            for (int i = 0; i < m_iIRLength; ++i) {
//                m_pCRingBuff->putPostInc(0.F);
//                for (int j = 0; j < m_iIRLength; ++j) {
//                    pfOutputBuffer[i] += m_pCRingBuff->get(m_iIRLength-j) * m_pfIR[j];
//                }
//                m_pCRingBuff->getPostInc();
//            }
            break;
            
        case 1:
//            for (int i = 0; i < m_iIRLength; ++i) {
//                m_ppCRingBuffFft[0]->putPostInc(0.F);
//                for (int j = 0; j < m_iIRLength; ++j) {
//                    pfOutputBuffer[i] += m_ppCRingBuffFft[0]->get(m_iIRLength-j) * m_ppfIRFft[0][j];
//                }
//                m_ppCRingBuffFft[0]->getPostInc();
//            }
            break;
            
        default:
            break;
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
