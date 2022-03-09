
#include <iostream>
#include <ctime>
#include <string.h>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "Vibrato.h"
#include "RingBuffer.h"

using namespace::std;

// local function declarations
void    showClInfo();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    std::string sInPath;
    std::string sOutPath;
    CAudioFileIf    *phInputFile = 0;
    CAudioFileIf    *phOutputFile = 0;
    CAudioFileIf::FileSpec_t sFileSpec;
    CVibrato        *pVibrato = 0;
    clock_t         time = 0;
    static const int kBlockSize = 1024;
    long long iNumFrames = kBlockSize;
    float fSampleRate;
    float fModFreq;
    float fDelay;
    float fWidth;
    float **ppfInputData = 0;
    float **ppfOutputData = 0;
    
    showClInfo();
    
    //////////////////////////////////////////////////////////////////////////////
    // parse command line arguments
    // arguments should be in this sequence: (input_file_path output_file_path sample_rate mod_frequency mod_delay mod_width)
    
    if(argc < 6) {
        cout << "missing some arguments" << endl;
    }
    else {
        sInPath = argv[1];
        sOutPath = sInPath + "_out.wav";
        fSampleRate = atof(argv[2]);
        fModFreq = atof(argv[3]);
        fDelay = atof(argv[4]);
        fWidth = atof(argv[5]);
    }
    
    //////////////////////////////////////////////////////////////////////////////
    // open the input file
    CAudioFileIf::create(phInputFile);
    phInputFile->openFile(sInPath, CAudioFileIf::kFileRead);
    if (!phInputFile->isOpen())
    {
        CAudioFileIf::destroy(phInputFile);
        return -1;
    }
    phInputFile->getFileSpec(sFileSpec);
    
    // open the output file
    CAudioFileIf::create(phOutputFile);
    phOutputFile->openFile(sOutPath, CAudioFileIf::kFileWrite, &sFileSpec);
    if (!phOutputFile->isOpen())
    {
        CAudioFileIf::destroy(phOutputFile);
        return -1;
    }
    
    //////////////////////////////////////////////////////////////////////////////
    // init the vibrato
    CVibrato::create(pVibrato);
    pVibrato->init(fDelay, sFileSpec.fSampleRateInHz, fWidth, fModFreq, sFileSpec.iNumChannels);

    // Set parameters of vibrato
//    pVibrato->setParam(CVibrato::kParamWidth, fWidth);
//    pVibrato->setParam(CVibrato::kParamModFreq, fModFreq);
    
    //////////////////////////////////////////////////////////////////////////////
    // allocate memory
    ppfInputData = new float*[sFileSpec.iNumChannels];
    for (int i = 0; i < sFileSpec.iNumChannels; i++)
        ppfInputData[i] = new float[kBlockSize];
    
    ppfOutputData = new float*[sFileSpec.iNumChannels];
    for (int i = 0; i < sFileSpec.iNumChannels; i++)
        ppfOutputData[i] = new float[kBlockSize];
    time = clock();
    
    //////////////////////////////////////////////////////////////////////////////
    // read, process and write
    while (!phInputFile->isEof())
    {
        phInputFile->readData(ppfInputData, iNumFrames);
        pVibrato->process(ppfInputData, ppfOutputData, iNumFrames);
        phOutputFile->writeData(ppfOutputData, iNumFrames);
    }
    
    cout << "\nreading/writing done in: \t" << (clock() - time)*1.F / CLOCKS_PER_SEC << " seconds." << endl;
    
    //////////////////////////////////////////////////////////////////////////////
    // clean-up
    CAudioFileIf::destroy(phInputFile);
    CAudioFileIf::destroy(phOutputFile);
    CVibrato::destroy(pVibrato);
    for (int i = 0; i < sFileSpec.iNumChannels; i++)
    {
        delete[] ppfInputData[i];
        delete[] ppfOutputData[i];
    }
    delete[] ppfInputData;
    delete[] ppfOutputData;
    ppfInputData = 0;
    ppfOutputData = 0;
    
    return 0;
    
}

void showClInfo()
{
    cout << "GTCMT MUSI6106 Executable" << endl;
    cout << "(c) 2014-2018 by Alexander Lerch" << endl;
    cout  << endl;
    
    return;
}
