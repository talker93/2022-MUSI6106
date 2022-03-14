#include "MUSI6106Config.h"

#ifdef WITH_TESTS
#include <cassert>
#include <cstdio>
#include <algorithm>

#include "gtest/gtest.h"

#include "Synthesis.h"
#include "Vector.h"
#include "ErrorDef.h"

#include "Vibrato.h"
#include "RingBuffer.h"

namespace vibrato_test {
    void CHECK_ARRAY_CLOSE(float* buffer1, float* buffer2, int iLength, float fTolerance)
    {
        for (int i = 0; i < iLength; i++)
        {
            EXPECT_NEAR(buffer1[i], buffer2[i], fTolerance);
        }
    }
    
class CTestVibrato : public testing::Test
{
public:
    CVibrato            *pVibrato = 0;
    float**             ppfAudioData = 0;
    float**             ppfOutputData = 0;
        
    float**             ppfTempIn = 0;
    float**             ppfTempOut = 0;
        
    float               fMaxWidthSecs = 0.1;
    float               fSampleRate = 44100;
    int                 iNumChannels = 2;
    int                 iBlockSize = 1024;
    float               fModFreq = 10;
    float               fModWidth = 0.05;
    float               fDelay = 0.05;
    
    void SetUp() override
    {
        CVibrato::create(pVibrato);

        pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);
        
        ppfAudioData  = new float*[iNumChannels];
        ppfOutputData = new float*[iNumChannels];
        
        ppfTempIn  = new float*[iNumChannels];
        ppfTempOut = new float*[iNumChannels];
        
        float fTestSigFreq = 440.F;
        
        for (int i = 0; i < iNumChannels; i++)
        {
            ppfAudioData[i] = new float[iBlockSize];
            ppfOutputData[i] = new float[iBlockSize];
            
            ppfTempIn[i]  = new float[iBlockSize];
            ppfTempOut[i] = new float[iBlockSize];

            CSynthesis::generateSine(ppfAudioData[i], fTestSigFreq, fSampleRate, iBlockSize);
        }
    }
    
    virtual void TearDown() override
    {
        for (int i = 0; i < iNumChannels; i++)
        {
            delete[] ppfAudioData[i];
            delete[] ppfOutputData[i];
            
            delete[] ppfTempIn[i];
            delete[] ppfTempOut[i];
        }
        delete[] ppfAudioData;
        delete[] ppfOutputData;
        
        delete[] ppfTempIn;
        delete[] ppfTempOut;
        
        CVibrato::destroy(pVibrato);
    }
    
    void testProcess(float** ppfOutputData, int iBlockSize, int iStartSample = 0)
    {
        for (int i = 0; i < iNumChannels; i++)
        {
            for (int j = 0; j < iBlockSize; j++)
            {
                ppfTempIn[i][j] = ppfAudioData[i][iStartSample + j];
            }
        }

        pVibrato->process(ppfTempIn, ppfTempOut, iBlockSize);

        for (int i = 0; i < iNumChannels; i++)
        {
            for (int j = 0; j < iBlockSize; j++)
            {
                ppfOutputData[i][iStartSample + j] = ppfTempOut[i][j];
            }
        }
    }
};

// Output equals delayed input when modulation amplitude is 0.
TEST_F(CTestVibrato, ModAmpZero)
{
    // e.g., you can use the "VibratoData" contents
    pVibrato -> reset();

    fModWidth = 0;

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);

    testProcess(ppfOutputData, iBlockSize);

    int iDelayInSamples = (int)(2 + (fMaxWidthSecs * 2 * fSampleRate));

    for (int i = 0; i < iNumChannels; i++)
    {
        for (int j = iDelayInSamples; j < iBlockSize; j++)
        {
            EXPECT_NEAR(ppfAudioData[i][j - iDelayInSamples], ppfOutputData[i][j], 1e-3F);
        }
    }
}

// DC input stays DC ouput regardless of parametrization.
TEST_F(CTestVibrato, DCEqual)
{
    for (int i = 0; i < iNumChannels; i++)
    {
        CSynthesis::generateDc(ppfAudioData[i], iBlockSize);
    }

    pVibrato -> reset();

    fModWidth = 0.05;

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);

    testProcess(ppfOutputData, iBlockSize);

    int iDelayInSamples = (int)(2 + (fMaxWidthSecs * 2 * fSampleRate));

    for (int i = 0; i < iNumChannels; i++)
    {
        for (int j = iDelayInSamples; j < iBlockSize; j++)
        {
            EXPECT_NEAR(ppfAudioData[i][j - iDelayInSamples], ppfOutputData[i][j], 1e-3F);
        }
    }
}



// Varying input block size.
TEST_F(CTestVibrato, VaryBlockSize)
{
    int testBlockSize = 459;
    
    float** ppfOutBlock  = new float*[iNumChannels];
    
    for (int i = 0; i < iNumChannels; i++)
    {
        ppfOutBlock[i] = new float[iBlockSize];
    }
    
    pVibrato -> reset();
    
    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);
    
    testProcess(ppfOutputData, iBlockSize);
    
    pVibrato -> reset();

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);
    testProcess(ppfOutBlock, testBlockSize);
    
    for (int i = 0; i < iNumChannels; i++)
    {
        CHECK_ARRAY_CLOSE(ppfOutBlock[i], ppfOutputData[i], testBlockSize, 1e-3F);
        delete[] ppfOutBlock[i];
    }
    delete[] ppfOutBlock;
    
    // Second block size
    
    testBlockSize = 277;
    
    ppfOutBlock  = new float*[iNumChannels];

    for (int i = 0; i < iNumChannels; i++)
    {
        ppfOutBlock[i] = new float[iBlockSize];
    }

    pVibrato -> reset();

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);

    testProcess(ppfOutputData, iBlockSize);

    pVibrato -> reset();

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);
    
    testProcess(ppfOutBlock, testBlockSize);

    for (int i = 0; i < iNumChannels; i++)
    {
        CHECK_ARRAY_CLOSE(ppfOutBlock[i], ppfOutputData[i], testBlockSize, 1e-3F);
        delete[] ppfOutBlock[i];
    }

    delete[] ppfOutBlock;
    
    // Third block size
    
    testBlockSize = 883;
    
    ppfOutBlock  = new float*[iNumChannels];

    for (int i = 0; i < iNumChannels; i++)
    {
        ppfOutBlock[i] = new float[iBlockSize];
    }

    pVibrato -> reset();

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);

    testProcess(ppfOutputData, iBlockSize);

    pVibrato -> reset();

    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);
    
    testProcess(ppfOutBlock, testBlockSize);

    for (int i = 0; i < iNumChannels; i++)
    {
        CHECK_ARRAY_CLOSE(ppfOutBlock[i], ppfOutputData[i], testBlockSize, 1e-3F);
        delete[] ppfOutBlock[i];
    }

    delete[] ppfOutBlock;
    
}

// Zero input signal.
TEST_F(CTestVibrato, ZeroInput)
{
    for (int i = 0; i < iNumChannels; i++)
    {
        CSynthesis::generateDc(ppfAudioData[i], iBlockSize, 0);
    }
    
    pVibrato -> reset();
    
    fModWidth = 0.05;
    
    pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels);
    
    testProcess(ppfOutputData, iBlockSize);
    
    for (int i = 0; i < iNumChannels; i++)
    {
        CHECK_ARRAY_CLOSE(ppfAudioData[i], ppfOutputData[i], iBlockSize, 1e-3F);
    }
}

// Zero input signal.
TEST_F(CTestVibrato, InvalidInputs)
{
    
    pVibrato -> reset();
    
    // negative inputs
    fModWidth = -0.05;
    fModFreq = -10;
    
    EXPECT_EQ(pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels), Error_t::kFunctionInvalidArgsError);
    
    // mod width larger than max width
    fModWidth = 1;
    fModFreq = 10;

    EXPECT_EQ(pVibrato -> CVibrato::init(fDelay, fSampleRate, fModWidth, fModFreq, iNumChannels), Error_t::kFunctionInvalidArgsError);
}

class CTestRingBuffer : public testing::Test
{
public:
    CRingBuffer<float>* pRingBuffer = 0;
    int iBufferLengthInSamples = 10;
    
    void SetUp() override
    {
        pRingBuffer = new CRingBuffer<float>(iBufferLengthInSamples);

        // init ring buffer with values 1-10
        for (int i = 1; i < iBufferLengthInSamples + 1; i++)
        {
            pRingBuffer -> putPostInc(i);
        }
    }
    
    virtual void TearDown() override
    {
        pRingBuffer -> reset();
        delete pRingBuffer;
    }
    
    void fillBuffer(int iBufferLengthInSamples)
    {
        for (int i = 1; i < iBufferLengthInSamples + 1; i++)
        {
            pRingBuffer -> putPostInc(i);
        }
    }
};

// Test put/get and initial read/write from initialized buffer
TEST_F(CTestRingBuffer, InitBuffer)
{
    EXPECT_EQ(pRingBuffer -> get(), 1);
    EXPECT_EQ(pRingBuffer -> getReadIdx(), 0);
    EXPECT_EQ(pRingBuffer -> getWriteIdx(), 0);
}

// Test interpolation
TEST_F(CTestRingBuffer, Interp)
{
    EXPECT_EQ(pRingBuffer -> get(1.5), 2.5);
}

// Test getPostInc and readIdx wrap around
TEST_F(CTestRingBuffer, GetWrapAround)
{
    for (int i = 1; i < iBufferLengthInSamples + 1; i++)
    {
        EXPECT_EQ(pRingBuffer -> getPostInc(), i);
    }
    EXPECT_EQ(pRingBuffer -> getReadIdx(), 0);
}

// Test get number of values in ring buffer
TEST_F(CTestRingBuffer, NumValues)
{
    pRingBuffer -> getPostInc();
    EXPECT_EQ(pRingBuffer -> getNumValuesInBuffer(), iBufferLengthInSamples-1);
    pRingBuffer -> reset();
    EXPECT_EQ(pRingBuffer -> getNumValuesInBuffer(), 0);
}

}

#endif //WITH_TESTS
