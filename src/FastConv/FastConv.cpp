
#include "FastConv.h"

CFastConv::CFastConv( void ) :
    m_iBlockLength(0),
    m_iOverlapLength(0),
    m_iDataLength(0),
    m_iIRLength(0),
    m_iFftLength(0),
    m_pCRingBuff(0),
    m_pCRingBuffOut(0),
    m_pfIR(0),
    m_ppfIRFft(0),
    m_bIsInitialized(false),
    m_eCompType(kNumConvCompModes),
    m_iDivNums(0),
    m_ppfMulBuffer(0),
    m_ppfMulSplitBuffer(0),
    m_ppfMulFftBuffer(0),
    m_pfLayerBuffer(0),
    m_pfFftOutBuffer(0),
    m_pCFft(0),
    m_iCurBlock(0), 
    m_iLatency(0),
    m_pfOutputBuffer(0),
    m_pfInputBuffer(0),
    m_pfBlockBuffer(0),
    m_bBoundryIsMet(false)
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
    if(iLengthOfIr == 0)
        return Error_t::kFunctionInvalidArgsError;

    if(pfImpulseResponse == NULL)
        return Error_t::kFunctionInvalidArgsError;

    if(iBlockLength < 1)
        return Error_t::kFunctionInvalidArgsError;

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
            if (!CUtil::isPowOf2(iBlockLength))
                return Error_t::kFunctionInvalidArgsError;
            m_iBlockLength = iBlockLength;
            m_eCompType = CFastConv::kFreqDomain;
            m_iIRLength = iLengthOfIr;
            m_iDivNums = ceil(static_cast<float>(iLengthOfIr)/static_cast<float>(iBlockLength));

            m_iFftLength = 2 * m_iBlockLength;
            
            // create buffer
            m_pCRingBuffIn = new CRingBuffer<float> (m_iBlockLength);
            m_pCRingBuffOut = new CRingBuffer<float> (m_iBlockLength);
            
            m_pfLayerBuffer = new float [m_iBlockLength*(m_iDivNums+1)]();
            m_pfOutputBuffer = new float [m_iBlockLength*(m_iDivNums+1)]();
            m_pfInputBuffer = new float [m_iBlockLength]();
            m_pfBlockBuffer = new float [m_iFftLength]();
            
            m_pfFftOutBuffer = new float [m_iFftLength]();
            
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
            m_pCFft->initInstance(m_iFftLength, 1, CFft::kWindowHann, CFft::kNoWindow);
            
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
            
            // caculating FFT results of impulse response
            m_ppf_H = new CFft::complex_t* [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
                m_ppf_H[i] = new CFft::complex_t [m_iFftLength]();
            
            m_ppf_H_Real = new float* [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
                m_ppf_H_Real[i] = new float [m_iFftLength/2 + 1]();
            
            m_ppf_H_Imag = new float* [m_iDivNums];
            for(int i = 0; i < m_iDivNums; i++)
                m_ppf_H_Imag[i] = new float [m_iFftLength/2 + 1]();
            
            for(int i = 0; i < m_iDivNums; i++)
            {
                CVectorFloat::mulC_I(m_ppfIRFft[i], static_cast<float>(m_iFftLength), m_iBlockLength);
                m_pCFft->doFft(m_ppf_H[i], m_ppfIRFft[i]);
                m_pCFft->splitRealImag(m_ppf_H_Real[i], m_ppf_H_Imag[i], m_ppf_H[i]);
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
        delete m_pCRingBuffOut;
        m_pCRingBuffOut = 0;
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
        delete m_pfLayerBuffer;
        m_pfLayerBuffer = 0;
        delete m_pfFftOutBuffer;
        m_pfFftOutBuffer =0;
        m_pCFft->resetInstance();
        m_pCFft = 0;
    }
    m_iBlockLength = 0;
    m_iOverlapLength = 0;
    m_iDataLength = 0;
    m_iIRLength = 0;
    m_iFftLength = 0;
    m_pCRingBuff = 0;
    m_pfIR = 0;
    m_ppfIRFft = 0;
    m_eCompType = kNumConvCompModes;
    m_iDivNums = 0;
    m_ppfMulBuffer = 0;
    m_ppfMulSplitBuffer = 0;
    m_pfLayerBuffer = 0;
    m_pfFftOutBuffer = 0;
    m_pCFft = 0;
    m_iCurBlock = 0;
    m_iLatency = 0;
    m_pfOutputBuffer = 0;
    m_pfInputBuffer = 0;
    m_pfBlockBuffer = 0;
    m_bBoundryIsMet = false;
    m_bIsInitialized = false;
    
    return Error_t::kNoError;
}

Error_t CFastConv::fftMul(float *pfMulOut, const float *pfMul, int H_index, int iLenOfBuffer)
{
    copy(pfMul, pfMul+iLenOfBuffer, m_ppfMulBuffer[0]);
    
    //pre scaling
    CVectorFloat::mulC_I(m_ppfMulBuffer[0], static_cast<float>(m_iFftLength), iLenOfBuffer);
    
    // m_ppfMulBuffer: 0 -> output; 0 -> input
    m_pCFft->doFft(m_ppfMulFftBuffer[0], m_ppfMulBuffer[0]);
    
    // m_ppfMulSplitBuffer: 0 -> Real; 1 -> Imag
    m_pCFft->splitRealImag(m_ppfMulSplitBuffer[0], m_ppfMulSplitBuffer[1], m_ppfMulFftBuffer[0]);

    // m_ppfMulSplitBuffer: 4 -> OutputReal; 5 -> OutputImag
    for(int i = 0; i < m_iFftLength/2+1; i++)
    {
        m_ppfMulSplitBuffer[4][i] = m_ppfMulSplitBuffer[0][i] * m_ppf_H_Real[H_index][i]
                                    - m_ppfMulSplitBuffer[1][i] * m_ppf_H_Imag[H_index][i];
        m_ppfMulSplitBuffer[5][i] = m_ppfMulSplitBuffer[0][i] * m_ppf_H_Imag[H_index][i]
                                    + m_ppfMulSplitBuffer[1][i] * m_ppf_H_Real[H_index][i];
    }
    
    m_pCFft->mergeRealImag(m_ppfMulFftBuffer[2], m_ppfMulSplitBuffer[4], m_ppfMulSplitBuffer[5]);
    
    m_pCFft->doInvFft(pfMulOut, m_ppfMulFftBuffer[2]);
    
    //post scaling
    CVectorFloat::mulC_I(pfMulOut, 1/static_cast<float>(m_iFftLength), m_iFftLength);
    
    return Error_t::kNoError;
}

Error_t CFastConv::process (float* pfOutputBuffer, const float *pfInputBuffer, int iLengthOfBuffers)
{
    
    if(m_eCompType == kFreqDomain)
    {
        // when the boundry met
        if(iLengthOfBuffers+m_pCRingBuffIn -> getNumValuesInBuffer() >= m_iBlockLength)
        {
            m_bBoundryIsMet = true;
            
            // 1.before calculation
            // 1.1. split current frame and fill up the input ring buffer
            int value2fill = m_iBlockLength - m_pCRingBuffIn -> getNumValuesInBuffer();
            m_pCRingBuffIn->putPostInc(pfInputBuffer, value2fill);
            // 1.2. write out the rest output ring buffer
            m_pCRingBuffOut->getPostInc(pfOutputBuffer, value2fill);
            
            // 2. FFT and multiplication
            // 2.1 overlap blocks for one layer
            m_pCRingBuffIn -> getPostInc(m_pfInputBuffer, m_iBlockLength);
            float* remaining = new float [m_iBlockLength]();
            CVectorFloat::setZero(m_pfLayerBuffer, (m_iDivNums+1)*m_iBlockLength);
            for(int i = 0; i < m_iDivNums; i++)
            {
                fftMul(m_pfBlockBuffer, m_pfInputBuffer, i, m_iBlockLength);
                CVectorFloat::add_I(&m_pfLayerBuffer[i*m_iBlockLength], m_pfBlockBuffer, m_iFftLength);
            }
            
            // 2.2 overlap layers
            CVectorFloat::add_I(m_pfLayerBuffer, &m_pfOutputBuffer[m_iBlockLength], m_iDivNums*m_iBlockLength);
            
            // 2.3 send to buffer
            CVectorFloat::copy(m_pfOutputBuffer, m_pfLayerBuffer, m_iBlockLength*(m_iDivNums+1));
            
            // 2.4 send to ring buffer
            m_pCRingBuffOut -> putPostInc(m_pfOutputBuffer, m_iBlockLength);
//            checkData(m_pfOutputBuffer, (m_iDivNums+1)*m_iBlockLength);
            
            // 3. after calculation
            // 3.1 copy the 2nd part of input into ring buffer
            m_pCRingBuffIn->putPostInc(&pfInputBuffer[value2fill], iLengthOfBuffers-value2fill);
            
            // 3.2 write out the initial values from each FFT results
            m_pCRingBuffOut -> getPostInc(&pfOutputBuffer[value2fill], iLengthOfBuffers-value2fill);
        }
        
        // copy value from input into ring buffer
        // bypass this function when boundries have been met
        if(!m_bBoundryIsMet)
            m_pCRingBuffIn->putPostInc(pfInputBuffer, iLengthOfBuffers);
        
        // copy value from ring buffer into output
        // add latency when boundries have been met
        if(!m_bBoundryIsMet)
            m_pCRingBuffOut -> getPostInc(pfOutputBuffer, iLengthOfBuffers);
        
        checkData(pfOutputBuffer, iLengthOfBuffers);
//        m_iCurBlock++;
//        cout << m_iCurBlock << endl;
        m_bBoundryIsMet = false;
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
    float* pfFlushInputBuffer = new float[m_iBlockLength + m_iIRLength - 1];
    memset(pfFlushInputBuffer, 0, sizeof(float) * (m_iBlockLength + m_iIRLength - 1));
    
    switch (m_eCompType) {
        case 0:
            for (int i = 0; i < m_iIRLength; ++i) {
                m_pCRingBuff->putPostInc(0.F);
                for (int j = 0; j < m_iIRLength; ++j) {
                    pfOutputBuffer[i] += m_pCRingBuff->get(m_iIRLength-j) * m_pfIR[j];
                }
                m_pCRingBuff->getPostInc();
            }
            break;
            
        case 1:
            
            process(pfOutputBuffer, pfFlushInputBuffer, m_iBlockLength + m_iIRLength - 1);
//            for (int i = 0; i < m_iIRLength; ++i) {
//                //m_pCRingBuff->putPostInc(0.F);
//                for (int j = 0; j < m_iIRLength; ++j) {
//                    pfOutputBuffer[i] += pfFlushInputBuffer[m_iIRLength-j] * m_pfIR[j];
//                }
//                m_pCRingBuff->getPostInc();
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

Error_t CFastConv::checkData(float*& pfData, int dataLength, bool reset /*= false*/)
{
    cout << "-----------------data inspector-----------" << endl;
    cout << "Data until " << dataLength << " : "<< endl;
    for(int i = 0; i < dataLength; i++)
        cout << pfData[i] << ", ";
    cout << "-----------------inspector ends------------" << endl;
    if(reset == true)
    {
        delete [] pfData;
        pfData = 0;
    }
}
