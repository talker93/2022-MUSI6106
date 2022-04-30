#include "MUSI6106Config.h"

#ifdef WITH_TESTS
#include "Synthesis.h"

#include "Vector.h"
#include "FastConv.h"

#include "gtest/gtest.h"
#include <random>


namespace fastconv_test {
    void CHECK_ARRAY_CLOSE(float* buffer1, float* buffer2, int iLength, float fTolerance)
    {
        for (int i = 0; i < iLength; i++)
        {
            EXPECT_NEAR(buffer1[i], buffer2[i], fTolerance);
        }
    }

class FastConv: public testing::Test
    {
    protected:
        void SetUp() override
        {
            m_pfInput = new float[m_iInputLength];
            m_pfIr = new float[m_iIRLength];
            m_pfOutput = new float[m_iInputLength + m_iIRLength];

            CVectorFloat::setZero(m_pfInput, m_iInputLength);
            m_pfInput[0] = 1;

            CSynthesis::generateNoise(m_pfIr, m_iIRLength);
            m_pfIr[0] = 1;

            CVectorFloat::setZero(m_pfOutput, m_iInputLength + m_iIRLength);

            m_pCFastConv = new CFastConv();
        }

        virtual void TearDown()
        {
            m_pCFastConv->reset();
            delete m_pCFastConv;

            delete[] m_pfIr;
            delete[] m_pfOutput;
            delete[] m_pfInput;
        }

        float *m_pfInput = 0;
        float *m_pfIr = 0;
        float *m_pfOutput = 0;

        int m_iInputLength = 83099;
        int m_iIRLength = 60001;

        CFastConv *m_pCFastConv = 0;
    };

    TEST_F(FastConv, Params)
    {
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(0, 1));
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 0));
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 10, -1));
        EXPECT_EQ(false, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 10, 7));
        EXPECT_EQ(true, Error_t::kNoError == m_pCFastConv->init(m_pfIr, 10, 4));
        EXPECT_EQ(true, Error_t::kNoError == m_pCFastConv->reset());
    }

    TEST_F(FastConv, Impulse)
    {
        // impulse with impulse
        int iBlockLength = 4;
        m_pCFastConv->init(m_pfIr, 1, iBlockLength);

        for (auto i = 0; i < 500; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        EXPECT_NEAR(1.F, m_pfOutput[iBlockLength], 1e-6F);
        EXPECT_NEAR(0.F, m_pfOutput[iBlockLength+1], 1e-6F);
        EXPECT_NEAR(0.F, m_pfOutput[iBlockLength+2], 1e-6F);
        EXPECT_NEAR(0.F, m_pfOutput[iBlockLength+3], 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMin(m_pfOutput, m_iInputLength), 1e-6F);
        EXPECT_NEAR(1.F, CVectorFloat::getMax(m_pfOutput, m_iInputLength), 1e-6F);

        // impulse with dc
        for (auto i = 0; i < 4; i++)
            m_pfOutput[i] = 1;
        iBlockLength = 8;
        m_pCFastConv->init(m_pfOutput, 4, iBlockLength);

        for (auto i = 0; i < 500; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        EXPECT_NEAR(0.F, CVectorFloat::getMean(m_pfOutput, 8), 1e-6F);
        EXPECT_NEAR(1.F, CVectorFloat::getMean(&m_pfOutput[8], 4), 1e-6F);
        EXPECT_NEAR(0.F, CVectorFloat::getMean(&m_pfOutput[12], 400), 1e-6F);

        // impulse with noise
        iBlockLength = 8;
        m_pCFastConv->init(m_pfIr, 27, iBlockLength);

        for (auto i = 0; i < m_iInputLength; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfInput[i], 1);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], 27, 1e-6F);
        CHECK_ARRAY_CLOSE(&m_pfInput[1], &m_pfOutput[iBlockLength + 27], 10, 1e-6F);

        // noise with impulse
        iBlockLength = 8;
        m_pCFastConv->init(m_pfInput, 27, iBlockLength);

        for (auto i = 0; i < m_iIRLength; i++)
            m_pCFastConv->process(&m_pfOutput[i], &m_pfIr[i], 1);

        CHECK_ARRAY_CLOSE(m_pfIr, &m_pfOutput[iBlockLength], m_iIRLength - iBlockLength, 1e-6F);
    }


    class FastConvTime: public testing::Test
    {
    protected:
        void SetUp() override
        {
            
            m_pCFastConv = new CFastConv();
            IRLength = 51;
            //BlockLength = 64;
            IR = new float[IRLength];
            std::default_random_engine generator(0);
            std::uniform_real_distribution<float> uniformRealDistribution(-1.0,1.0);
            
            for(int i = 0; i < IRLength; i++)
                IR[i] = uniformRealDistribution(generator);
            
            m_pCFastConv->init(IR, IRLength, IRLength, CFastConv::kTimeDomain);
            
        }

        virtual void TearDown()
        {
            m_pCFastConv->reset();
            delete[] IR;
        }
        
        int IRLength = 0;
        float* IR = nullptr;
        //int BlockLength = 0;
        CFastConv *m_pCFastConv = 0;
    };

    TEST_F(FastConvTime, identity){
        
        //input
        int InputLength = 10;
        float* input = new float[InputLength];
        //for(int i = 0; i < InputLength; i++)
        //    input[i] = 0.0;
        memset(input, 0, sizeof(float) * InputLength);
        input[3] = 1.0;
        //output
        float* output = new float[InputLength];
        memset(output, 0, sizeof(float) * InputLength);
        
        m_pCFastConv->process(output, input, InputLength);
        
        //check
        for(int i = 0; i + 3 < InputLength; i++){
            EXPECT_NEAR(IR[i], output[i + 3], 1e-8);
        }
        
        for (int i = 0; i < 3; i++) {
                EXPECT_EQ(output[i], 0);
        }
        
        delete[] input;
        delete[] output;
        
    }

    TEST_F(FastConvTime, flushbuffer){
        
        //input
        int InputLength = 10;
        float* input = new float[InputLength];
        //for(int i = 0; i < InputLength; i++)
        //    input[i] = 0.0;
        memset(input, 0, sizeof(float) * InputLength);
        input[3] = 1.0;
        //output
        float* output = new float[IRLength];
        memset(output, 0, sizeof(float) * IRLength);
        
        m_pCFastConv->process(output, input, InputLength);
        memset(output, 0, sizeof(float) * IRLength);
        
        //flush
        m_pCFastConv->flushBuffer(output);
        
        //check
        for(int i = 0; i + InputLength - 3 < IRLength; i++){
            EXPECT_NEAR(IR[i + InputLength - 3], output[i], 1e-8);
        }
        
        delete[] input;
        delete[] output;
        
    }

    //identity with a succession of different input/output block sizes
    TEST_F(FastConvTime, varyingBlockSize){
        
        //input
        int InputLength = 10000;
        float* input = new float[InputLength];
        //for(int i = 0; i < InLength; i++)
        //    input[i] = 0.0;
        memset(input, 0, sizeof(float) * InputLength);
        input[3] = 1.0;
        //output
        float* output = new float[InputLength];
        memset(output, 0, sizeof(float) * InputLength);
        
        int blockLengths[] = {1, 13, 1023, 2048, 1, 17, 5000, 1897};
        
        
        for(int i = 0, start = 0; i < 8; start += blockLengths[i++])
            m_pCFastConv->process(output + start, input + start, blockLengths[i]);
        
        //check output shiif IR
        for (int i = 0; i < IRLength && i + 3 < InputLength; i++) {
            EXPECT_NEAR(IR[i], output[i + 3], 1e-8);
        }
        //check tail
        for (int i = IRLength + 3; i < InputLength; i++) {
            //EXPECT_NEAR(output[i], 0, 1e-8);
            EXPECT_EQ(output[i], 0);
        }
    
    }



    //TEST_F(FastConv, EmptyTest)
    //{
    //}

    class FastConvFreq: public testing::Test
    {
    protected:
        void SetUp() override
        {
            
            m_pCFastConv = new CFastConv();
            IRLength = 51;
            IR = new float[IRLength];
            std::default_random_engine generator(0);
            std::uniform_real_distribution<float> uniformRealDistribution(-1.0,1.0);
            
            for(int i = 0; i < IRLength; i++)
                IR[i] = uniformRealDistribution(generator);
            
            m_pCFastConv->init(IR, IRLength, IRLength, CFastConv::kFreqDomain);
            
        }

        virtual void TearDown()
        {
            m_pCFastConv->reset();
            delete[] IR;
        }
        
        int IRLength = 0;
        float* IR = nullptr;
        CFastConv *m_pCFastConv = 0;
    };

    TEST_F(FastConvFreq, identity){
        
        //input
        int InputLength = 10;
        float* input = new float[InputLength];
        //for(int i = 0; i < InputLength; i++)
        //    input[i] = 0.0;
        memset(input, 0, sizeof(float) * InputLength);
        input[3] = 1.0;
        //output
        float* output = new float[InputLength];
        memset(output, 0, sizeof(float) * InputLength);
        
        m_pCFastConv->process(output, input, InputLength);
        
        //check
        for(int i = 0; i + 3 < InputLength; i++){
            EXPECT_NEAR(IR[i], output[i + 3], 1e-4);
        }
        
        for (int i = 0; i < 3; i++) {
                EXPECT_EQ(output[i], 0);
        }
        
        delete[] input;
        delete[] output;
        
    }

    TEST_F(FastConvFreq, flushbuffer){
        
        //input
        int InputLength = 10;
        float* input = new float[InputLength];
        //for(int i = 0; i < InputLength; i++)
        //    input[i] = 0.0;
        memset(input, 0, sizeof(float) * InputLength);
        input[3] = 1.0;
        //output
        float* output = new float[IRLength];
        memset(output, 0, sizeof(float) * IRLength);
        
        m_pCFastConv->process(output, input, InputLength);
        memset(output, 0, sizeof(float) * IRLength);
        
        //flush
        m_pCFastConv->flushBuffer(output);
        
        //check
        for(int i = 0; i + InputLength - 3 < IRLength; i++){
            EXPECT_NEAR(IR[i + InputLength - 3], output[i], 1e-4);
        }
        
        delete[] input;
        delete[] output;
        
    }

    //identity with a succession of different input/output block sizes
    TEST_F(FastConvFreq, varyingBlockSize){
        
        //input
        int InputLength = 10000;
        float* input = new float[InputLength];
        //for(int i = 0; i < InLength; i++)
        //    input[i] = 0.0;
        memset(input, 0, sizeof(float) * InputLength);
        input[3] = 1.0;
        //output
        float* output = new float[InputLength];
        memset(output, 0, sizeof(float) * InputLength);
        
        int blockLengths[] = {1, 13, 1023, 2048, 1, 17, 5000, 1897};
        
        
        for(int i = 0, start = 0; i < 8; start += blockLengths[i++])
        {
            m_pCFastConv->process(output + start, input + start, blockLengths[i]);
        }
        
        //check output shiif IR
        for (int i = 0; i < IRLength && i + 3 < InputLength; i++) {
            EXPECT_NEAR(IR[i], output[i + 3], 1e-4);
        }
        //check tail
        for (int i = IRLength + 3; i < InputLength; i++) {
            //EXPECT_NEAR(output[i], 0, 1e-8);
            EXPECT_EQ(output[i], 0);
        }

    }





}

#endif //WITH_TESTS
