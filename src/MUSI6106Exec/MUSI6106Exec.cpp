
#include <iostream>
#include <ctime>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "CombFilterIf.h"

using std::cout;
using std::endl;
using std::string;

// local function declarations
void    showClInfo ();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    showClInfo ();
    // //////////
    // declare global variables
    CAudioFileIf *phInputFile = 0;
    CAudioFileIf *phOutputFile = 0;
    CAudioFileIf::FileSpec_t sFileSpec;
    CCombFilterIf *phFilter = 0;
    CCombFilterIf::CombFilterType_t combType;
    std::fstream hOutputFile;
    clock_t time = 0;
    
    float **ppfInputData = 0;
    float **ppfOutputData = 0;
    
    int kBlockSize = 1024;
    string sInPath;
    string sOutPath;
    float fDelay;
    float fMaxDelay = 1;
    float fGain;
    

    //////////////////////////////////////////////////////////////////////////////
    // parse command line arguments
    if(argc < 5) {
        cout << "missing some arguments" << endl;
    }
    else {
        sInPath = argv[1];
        sOutPath = sInPath + "_out.wav";
        if(strcmp(argv[2], "fir")) {
            combType = CCombFilterIf::kCombFIR;
        } else {
            combType = CCombFilterIf::kCombIIR;
        }
        fDelay = atof(argv[3]);
        fGain = atof(argv[4]);
    }
    

    //////////////////////////////////////////////////////////////////////////////
    // open the input file
    CAudioFileIf::create(phInputFile);
    phInputFile->openFile(sInPath, CAudioFileIf::kFileRead);
    while(!phInputFile->isOpen()) {
        CAudioFileIf::destroy(phInputFile);
        return -1;
    }
    phInputFile->getFileSpec(sFileSpec);
    
    // Create the output file
    CAudioFileIf::create(phOutputFile);
    phOutputFile->openFile(sOutPath, CAudioFileIf::kFileWrite, &sFileSpec);
    while(!phOutputFile->isOpen()) {
        CAudioFileIf::destroy(phOutputFile);
        return -1;
    }
    
    
    // init the filter
    CCombFilterIf::create(phFilter);
    phFilter->init(combType, fMaxDelay, sFileSpec.fSampleRateInHz, sFileSpec.iNumChannels);
    phFilter->setParam(CCombFilterIf::kParamDelay, fDelay);
    phFilter->setParam(CCombFilterIf::kParamGain, fGain);

    
    //////////////////////////////////////////////////////////////////////////////
    // Allocate memory for pointers
    ppfInputData = new float*[sFileSpec.iNumChannels];
    for(int i=0; i<sFileSpec.iNumChannels; i++) {
        ppfInputData[i] = new float[kBlockSize];
    }
    // ppfOutputData[channel][kBlockSize]
    ppfOutputData = new float*[sFileSpec.iNumChannels];
    for(int i=0; i<sFileSpec.iNumChannels; i++) {
        ppfOutputData[i] = new float[kBlockSize];
    }
    time = clock();

    
    //////////////////////////////////////////////////////////////////////////////
    // read, process and write
    long long iNumFrames = kBlockSize;
    while(!phInputFile->isEof()) {
        phInputFile->readData(ppfInputData, iNumFrames);
        phFilter->process(ppfInputData, ppfOutputData, iNumFrames);
        phOutputFile->writeData(ppfOutputData, iNumFrames);
    }
    cout << "\nreading/writing done in: \t" << (clock() - time) * 1.F / CLOCKS_PER_SEC << " seconds." << endl;

    //////////////////////////////////////////////////////////////////////////////
    // free memory
    CAudioFileIf::destroy(phInputFile);
    CAudioFileIf::destroy(phOutputFile);
    CCombFilterIf::destroy(phFilter);
    for(int i=0; i<sFileSpec.iNumChannels; i++) {
            delete [] ppfInputData[i];
            delete [] ppfOutputData[i];
    }
    delete [] ppfInputData;
    delete [] ppfOutputData;
    ppfInputData = 0;
    ppfOutputData = 0;

    // all done
    return 0;

}


void     showClInfo()
{
    cout << "MUSI6106 Assignment Executable" << endl;
    cout << "(c) 2014-2022 by Alexander Lerch" << endl;
    cout  << endl;

    return;
}

