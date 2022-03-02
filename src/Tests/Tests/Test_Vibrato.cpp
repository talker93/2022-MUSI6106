#include "MUSI6106Config.h"

#ifdef WITH_TESTS
#include <cassert>
#include <cstdio>
#include <algorithm>

#include "UnitTest++.h"

#include "Synthesis.h"
#include "Vector.h"
#include "ErrorDef.h"

#include "Vibrato.h"

namespace vibrato_test {
    void CHECK_ARRAY_CLOSE(float* buffer1, float* buffer2, int iLength, float fTolerance)
    {
        for (int i = 0; i < iLength; i++)
        {
            EXPECT_NEAR(buffer1[i], buffer2[i], fTolerance);
        }
    }

}

SUITE(Vibrato)
{
    struct VibratoData
    {
        VibratoData():
            // setup
            // e.g., allocate a vibrato object and test signal (newly created for each test case)
            
            pVibrato(0),
            ppfAudioData(0),
            ppfOutputData(0),
            
            ppfTempIn(0),
            ppfTempOut(0),
            
            fMaxWidthSecs(0.1),
            fSampleRate(44100),
            iNumChannels(2),
            iBlockSize(1024),
            fModFreq(10),
            fModWidth(0.05)
            
            {
                CVibrato::create(pVibrato);
                pVibrato -> CVibrato::init(fMaxWidthInS, fSampleRateInHz, iNumChannels, fModFreq, fModWidth);
                
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

                    CSynthesis::generateSine(ppfAudioData[i], fTestSigFreq, iSampleRate, iBlockSize);
                
                }
            
            }

        ~VibratoData()
        {
            // teardown
            // e.g., deallocate the vibrato object and test signal
            
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

        // e.g., a reusable process() function
        void testProcess(float** ppfOutputData, int iBlockSize, int iStartSample = 0,)
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

        // e.g., a member vibrato object to be reused in each test
            
        CVibrato::Vibrato   *pVibrato;
        float**             ppfAudioData;
        float**             ppfOutputData;
            
        float**             ppfTempIn;
        float**             m_ppfTempOut;
            
        float               fMaxWidthSecs;
        float               fSampleRate;
        int                 iNumChannels;
        int                 iBlockSize;
        float               fModFreq;
        float               fModWidth;

    };

    TEST(MyTestWithNoFixture)
    {
        // e.g., allocate & deallocate local data for testing
    }

    // Output equals delayed input when modulation amplitude is 0.
    TEST_FIXTURE(VibratoData, ModAmpZero)
    {
        // e.g., you can use the "VibratoData" contents
        pVibrato -> reset();
        
        fMaxWidthSecs = 0.1;
        fModWidth = 0;
        
        pVibrato -> init(fMaxWidthSecs, fSampleRate, iNumChannels, fModFreq, fModWidth);
        
        testProcess(ppfOutputData, iBlockSize);
        
        int iDelayInSamples = (int)(ceil(fModWidth * sampleRate))
        
        for (int i = 0; i < iNumChannels; i++)
        {
           for (int j = iDelayinSamples; j < iBlockSize; j++)
           {
               CHECK_ARRAY_CLOSE(ppfAudioData[i][j - iDelayInSamples], outputData[i][j], 1, 1e-3F);
           }
        }
    }
       
    // DC input stays DC ouput regardless of parametrization.
    TEST_FIXTUREE(VibratoData, DCEqual)
    {
        for (int i = 0; i < iNumChannels; i++)
        {
            CSynthesis::generateDc(m_ppfAudioData[i], iBlockSize);
        }
        
        pVibrato -> reset();
        
        fMaxWidthSecs = 0.1;
        fModWidth = 0.05;
        
        pVibrato -> init(fMaxWidthSecs, fSampleRate, iNumChannels, fModFreq, fModWidth);
        
        testProcess(ppfOutputData, iBlockSize);
        
        for (int i = 0; i < iNumChannels; i++)
        {
            CHECK_ARRAY_CLOSE(ppfAudioData[i], outputData[i], iBlockSize, 1e-3F);
        }
    }
    
    // Varying input block size.
    TEST_FIXTURE(VibratoData, VaryBlockSize)
    {
        float** ppfOutBlock  = new float*[iNumChannels];
        
        for (int i = 0; i < iNumChannels; i++)
        {
            ppfOutBlock[i] = new float[iBlockSize];
        }
        
        pVibrato -> reset();
        
        pVibrato -> init(fMaxWidthSecs, fSampleRate, iNumChannels, fModFreq, fModWidth);
        
        testProcess(ppfOutputData, iBlockSize);
        testProcess(ppfOutBlock, 459);
        
        for (int i = 0; i < iNumChannels; i++)
        {
            CHECK_ARRAY_CLOSE(ppfOutBlock[i], outputData[i], iBlockSize, 1e-3F)
            delete[] ppfOutBlock[i];
        }
        
        delete[] ppfOutBlock;
    }
    
    // Zero input signal.
    TEST_FIXTURE(VibratoData, ZeroInput)
    {
        for (int i = 0; i < iNumChannels; i++)
        {
            CSynthesis::generateDc(m_ppfAudioData[i], iBlockSize, 0);
        }
        
        pVibrato -> reset();
        
        fMaxWidthSecs = 0.1;
        fModWidth = 0.05;
        
        pVibrato -> init(fMaxWidthSecs, fSampleRate, iNumChannels, fModFreq, fModWidth);
        
        testProcess(ppfOutputData, iBlockSize);
        
        for (int i = 0; i < iNumChannels; i++)
        {
            CHECK_ARRAY_CLOSE(ppfAudioData[i], outputData[i], iBlockSize, 1e-3F);
        }
    }
        
    // write additional test cases
    TEST_FIXTURE(VibratoData, )
};
    
    
SUITE(RingBuffer)
{
    struct RingBufferData
    {
        RingBufferData():
            pRingBuffer(0),
            iBufferLengthInSamples(10)

        {
            pRingBuffer = CRingBuffer<float>(iBufferLengthInSamples);
            
            // init ring buffer with values 1-10
            for (int i = 1; i < iBufferLengthInSamples + 1; i++)
            {
                pRingBuffer -> putPostInc(i, 1);
            }
        }
        
        ~RingBufferData()
        {
            pRingBuffer -> reset();
            delete pRingBuffer;
        }
        
        CRingBuffer<float>* pRingBuffer;
        int iBufferLength;
    }
    
    // Test put/get and initial read/write from initialized buffer
    TEST_FIXTURE(RingBufferData, InitBuffer)
    {
        EXPECT_EQ(pRingBuffer->get(), 1, 1e-3F);
        CHECK(pRingBuffer->getReadIdx() == 0);
        CHECK(pRingBuffer->getWriteIdx() == 0)
    }
    
    TEST_FIXTURE(RingBufferData, interp)
    {
        EXPECT_EQ(pRingBuffer->get(1.5), 2.5, 1e-3F);
    }
    
    
};

#endif //WITH_TESTS
