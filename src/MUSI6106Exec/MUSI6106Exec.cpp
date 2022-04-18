
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
//#include "Vibrato.h"
#include "FastConv.h"


using std::cout;
using std::endl;

// local function declarations
void    showClInfo();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{

    std::string             sInputFilePath,                 //!< file paths
        sOutputFilePath;

    static const int            kBlockSize = 1024;
    long long                   iNumFrames = kBlockSize;
    //int                         iNumChannels;

    //float                       fModFrequencyInHz;
    //float                       fModWidthInSec;
    int iIRLength = 10;

    clock_t                     time = 0;

    float** ppfInputAudio = 0;
    float** ppfOutputAudio = 0;
    
    float* pfImpulseResponse=0;

    CAudioFileIf* phAudioFile = 0;
    CAudioFileIf* phAudioFileOut = 0;

    CAudioFileIf::FileSpec_t    stFileSpec;

    CFastConv* pCFastConv = 0;

    showClInfo();


    // command line args
    if (argc < 2)
    {
        cout << "Incorrect number of arguments!" << endl;
        return -1;
    }
    sInputFilePath = argv[1];
    sOutputFilePath = argv[2];
    pfImpulseResponse = new float[iIRLength];
    for(int i = 0; i < 10; i++)
        pfImpulseResponse[i] = 1;
        
    //fModFrequencyInHz = atof(argv[3]);
    //fModWidthInSec = atof(argv[4]);

    ///////////////////////////////////////////////////////////////////////////
    // open the input wave file
    CAudioFileIf::create(phAudioFile);
    phAudioFile->openFile(sInputFilePath, CAudioFileIf::kFileRead);
    if (!phAudioFile->isOpen())
    {
        cout << "Wave file open error!";
        CAudioFileIf::destroy(phAudioFile);
        return -1;
    }
    phAudioFile->getFileSpec(stFileSpec);
    
    //create output file
    CAudioFileIf::create(phAudioFileOut);
    phAudioFileOut->openFile(sOutputFilePath, CAudioFileIf::kFileWrite, &stFileSpec);
    if (!phAudioFileOut->isOpen())
    {
        cout << "Wave file open error!";
        CAudioFileIf::destroy(phAudioFileOut);
        return -1;
    }
    
    // allocate memory
    ppfInputAudio = new float*[stFileSpec.iNumChannels];
    for (int i = 0; i < stFileSpec.iNumChannels; i++)
        ppfInputAudio[i] = new float[kBlockSize];

    ppfOutputAudio = new float*[stFileSpec.iNumChannels];
    for (int i = 0; i < stFileSpec.iNumChannels; i++)
        ppfOutputAudio[i] = new float[kBlockSize];
    
    ////////////////////////////////////////////////////////////////////////////
    //CVibrato::create(pCVibrato);
    //pCVibrato->init(fModWidthInSec, stFileSpec.fSampleRateInHz, iNumChannels);
    CFastConv::create(pCFastConv);
    pCFastConv->init(pfImpulseResponse, iIRLength, iNumFrames, CFastConv::kFreqDomain);
    
    
    // Set parameters of vibrato
    

    // processing
    while (!phAudioFile->isEof())
    {
        phAudioFile->readData(ppfInputAudio, iNumFrames);
        pCFastConv->process(ppfOutputAudio[0], ppfInputAudio[0], iNumFrames);
        phAudioFileOut->writeData(ppfOutputAudio, iNumFrames);
    }
    phAudioFile->getFileSpec(stFileSpec);


    cout << "\nreading/writing done in: \t" << (clock() - time) * 1.F / CLOCKS_PER_SEC << " seconds." << endl;

    //////////////////////////////////////////////////////////////////////////////
    // clean-up
    CAudioFileIf::destroy(phAudioFile);
    CAudioFileIf::destroy(phAudioFileOut);
    //CFastConv::reset(pCFastConv);

    
    
    delete[] ppfInputAudio;
    delete[] ppfOutputAudio;
    ppfInputAudio = 0;
    ppfOutputAudio = 0;

    // all done
    return 0;

}


void     showClInfo()
{
    cout << "MUSI6106 Assignment Executable" << endl;
    cout << "(c) 2014-2022 by Alexander Lerch" << endl;
    cout << endl;

    return;
}

