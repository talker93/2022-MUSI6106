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
            
            m_pCFastConv = new CFastConv();
            IRLength = 51;
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
        CFastConv *m_pCFastConv = 0;
    };

    TEST_F(FastConv, identity){
        
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

    TEST_F(FastConv, flushbuffer){
        
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
    TEST_F(FastConv, varyingBlockSize){
        
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



    TEST_F(FastConv, EmptyTest)
    {
    }
}

#endif //WITH_TESTS

