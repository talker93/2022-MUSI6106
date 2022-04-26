
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "FastConv.h"


using std::cout;
using std::endl;

// local function declarations
void    showClInfo();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{

    std::string sInputFilePath, sOutputFilePath;
    
    std::string sInputIrPath;

    static const int            kBlockSize = 512;
    static const int            kConvBlockSize = 2048;
    long long                   iNumFrames = kBlockSize;
    long long                   iIrLength = 0;
    //int                         iNumChannels;


    clock_t                     time = 0;
    clock_t time_begin = 0;
    clock_t time_end = 0;

    float** ppfInputAudio = 0;
    float** ppfOutputAudio = 0;
    float** ppfIrAudio = 0;
    float* pfImpulseResponse=0;

    CAudioFileIf* phAudioFile = 0;
    CAudioFileIf* phAudioFileOut = 0;
    CAudioFileIf* phAudioIr = 0;

    CAudioFileIf::FileSpec_t    stFileSpec;
    CAudioFileIf::FileSpec_t    stIrSpec;
    CFastConv::ConvCompMode_t   eCompMode;

    CFastConv* pCFastConv = 0;
    int mode = 0;

    showClInfo();


    // command line args
    if (argc < 5)
    {
        cout << "Incorrect number of arguments!" << endl;
        return -1;
    }
    sInputFilePath = argv[1];
    sOutputFilePath = argv[2];
    sInputIrPath = argv[3];
    mode = atoi(argv[4]);
    switch (mode) {
        case 0:
            eCompMode = CFastConv::kTimeDomain;
            break;
        case 1:
            eCompMode = CFastConv::kFreqDomain;
        default:
            break;
    }

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
    
    // open the impulse response file
    CAudioFileIf::create(phAudioIr);
    phAudioIr->openFile(sInputIrPath, CAudioFileIf::kFileRead);
    if (!phAudioIr->isOpen())
    {
        cout << "Impulse response open error!";
        CAudioFileIf::destroy(phAudioIr);
        return -1;
    }
    phAudioIr->getFileSpec(stIrSpec);
    
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
    
    phAudioIr->getLength(iIrLength);
    ppfIrAudio = new float*[stIrSpec.iNumChannels];
    for (int i = 0; i < stIrSpec.iNumChannels; i++)
        ppfIrAudio[i] = new float[iIrLength];
    cout << iIrLength << endl;
    if (ppfInputAudio == 0)
    {
        CAudioFileIf::destroy(phAudioFile);
        //hTestOutputFile.close();
        return -1;
    }
    if (ppfOutputAudio[0] == 0)
    {
        CAudioFileIf::destroy(phAudioFileOut);
        //hTestOutputFile.close();
        return -1;
    }
    
    if (ppfIrAudio[0] == 0)
    {
        CAudioFileIf::destroy(phAudioIr);
        return -1;
    }
    
    // Load impluse response from file
    phAudioIr->readData(ppfIrAudio, iIrLength);
    
    ////////////////////////////////////////////////////////////////////////////
    CFastConv::create(pCFastConv);
    pCFastConv->init(ppfIrAudio[0], iIrLength, kConvBlockSize, eCompMode);
    
    // processing
    time_begin = clock();
    while (!phAudioFile->isEof())
    {
        phAudioFile->readData(ppfInputAudio, iNumFrames);
        pCFastConv->process(ppfOutputAudio[0], ppfInputAudio[0], iNumFrames);
        phAudioFileOut->writeData(ppfOutputAudio, iNumFrames);
    }
    
    
    phAudioFile->getFileSpec(stFileSpec);

    //flushbuffer
    if(eCompMode==CFastConv::kTimeDomain)
    {
        float** flush = 0;
        flush = new float*[1];
        flush[0] = new float[iIrLength];
        memset(flush, 0, sizeof(float) * (iIrLength - 1));
        pCFastConv->flushBuffer(flush[0]);
        phAudioFileOut->writeData(flush, iIrLength - 1);
    }
    
    time_end = clock();
    cout << "\nConvolution fininshed by " << (time_end - time_begin) * 1.F / CLOCKS_PER_SEC << "seconds." << endl;

    
    cout << "\nreading/writing done in: \t" << (clock() - time) * 1.F / CLOCKS_PER_SEC << " seconds." << endl;

    //////////////////////////////////////////////////////////////////////////////
    // clean-up
    CAudioFileIf::destroy(phAudioFile);
    CAudioFileIf::destroy(phAudioFileOut);
    CAudioFileIf::destroy(phAudioIr);
    pCFastConv->reset();
    CFastConv::destroy(pCFastConv);

    
    delete [] ppfIrAudio;
    delete[] ppfInputAudio;
    delete[] ppfOutputAudio;
    ppfIrAudio = 0;
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

