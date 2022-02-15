
#include <iostream>
#include <ctime>
#include <vector>

#include "MUSI6106Config.h"

#include "AudioFileIf.h"
#include "CombFilter.h"

using namespace std;
using std::cout;
using std::endl;

// local function declarations
void    showClInfo ();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
    std::string sInputFilePath,                 //!< file paths
                sOutputFilePath;

    static const int kBlockSize = 1024;

    clock_t time = 0;

    float **ppfAudioData = 0;
    float **ppfFilterData = 0;

    CAudioFileIf *phAudioFile = 0;
    CCombFilter *phFilterFile = 0;
    std::fstream hOutputFile;
    CAudioFileIf::FileSpec_t stFileSpec;
    
    std::string sFilterType;
    float fGain = 0;
    float fDelay = 0;

    showClInfo();

    //////////////////////////////////////////////////////////////////////////////
    // parse command line arguments
    // input example: [exe path type gain delay]
    if (argc < 5)
    {
        cout << "Missing audio input path, type, gain, delay" << endl;
        cout << "Input Example: [Exe Path Type Gain Delay]" << endl;
        cout << "Type Example: FIR or IIR" << endl;
        return -1;
    }
    else if (argc == 5)
    {
        sInputFilePath = argv[1];
        sOutputFilePath = sInputFilePath + ".txt";
        sFilterType = argv[2];
        fGain = stof(argv[3]);
        fDelay = stof(argv[4]);
        cout << "Your Input: " << endl;
        cout << "Input Path: " << sInputFilePath << endl;
        cout << "Filter Type: " << sFilterType << endl;
        cout << "Gain: " << fGain << endl;
        cout << "Delay: " << fDelay << endl;
    }
    // for test
//    sInputFilePath = "01_Rock1-90-C#_comp_mic.wav";
//    sOutputFilePath = sInputFilePath + ".txt";
//    sFilterType = "IIR";
//    fGain = stof("0.5");
//    fDelay = stof("1");
//    cout << "Your Input: " << endl;
//    cout << "Input Path: " << sInputFilePath << endl;
//    cout << "Filter Type: " << sFilterType << endl;
//    cout << "Gain: " << fGain << endl;
//    cout << "Delay: " << fDelay << endl;

    //////////////////////////////////////////////////////////////////////////////
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
    
    CCombFilter::create(phFilterFile);
    phFilterFile->init(CCombFilter::kCombFIR, fDelay, stFileSpec.fSampleRateInHz, stFileSpec.iNumChannels);
    phFilterFile->setParam(CCombFilter::kParamGain, fGain);
    phFilterFile->setParam(CCombFilter::kParamDelay, fDelay);

    //////////////////////////////////////////////////////////////////////////////
    // open the output text file
    hOutputFile.open(sOutputFilePath.c_str(), std::ios::out);
    if (!hOutputFile.is_open())
    {
        cout << "Text file open error!";
        CAudioFileIf::destroy(phAudioFile);
        return -1;
    }

    //////////////////////////////////////////////////////////////////////////////
    // allocate memory
    ppfAudioData = new float*[stFileSpec.iNumChannels];
    ppfFilterData = new float*[stFileSpec.iNumChannels];
    for (int i = 0; i < stFileSpec.iNumChannels; i++)
    {
        ppfAudioData[i] = new float[kBlockSize];
        ppfFilterData[i] = new float[kBlockSize];
    }

    if (ppfAudioData == 0)
    {
        CAudioFileIf::destroy(phAudioFile);
        hOutputFile.close();
        return -1;
    }
    if (ppfAudioData[0] == 0)
    {
        CAudioFileIf::destroy(phAudioFile);
        hOutputFile.close();
        return -1;
    }

    time = clock();

    //////////////////////////////////////////////////////////////////////////////
    // get audio data and write it to the output text file (one column per channel)
    while (!phAudioFile->isEof())
    {
        // set block length variable
        long long iNumFrames = kBlockSize;

        // read data (iNumOfFrames might be updated!)
        phAudioFile->readData(ppfAudioData, iNumFrames);
        phFilterFile->process(ppfAudioData, ppfFilterData, stFileSpec.iNumChannels);

        cout << "\r" << "reading and writing";
        
        // FIR/IIR CombFilter
        vector< vector<float> > output;

        vector<float>delayLine;
        for(int i=0; i<10; i++)
        {
            delayLine.push_back(0);
        }
        
        for(int c = 0; c < stFileSpec.iNumChannels; c++)
        {
            output.push_back(vector<float>());
            for(int n = 0; n < iNumFrames; n++)
            {
                output[c].push_back(ppfAudioData[c][n] + fGain * delayLine[delayLine.size()-1]);
                delayLine.pop_back();
                if (sFilterType == "FIR")
                {
                    delayLine.push_back(ppfAudioData[c][n]);
                } else if (sFilterType == "IIR")
                {
                    delayLine.push_back(output[c][n]);
                }
                rotate(delayLine.begin(), delayLine.begin()+(delayLine.size()-1),delayLine.end());
            }
            delayLine.clear();
            for(int i=0; i<10; i++)
            {
                delayLine.push_back(0);
            }
        }

        for (int i = 0; i < iNumFrames; i++)
        {
            for (int c = 0; c < stFileSpec.iNumChannels; c++)
            {
                hOutputFile << output[c][i] << "\t";
            }
            hOutputFile << endl;
        }
        
        // write raw audio data
//        for (int i = 0; i < iNumFrames; i++)
//        {
//            for (int c = 0; c < stFileSpec.iNumChannels; c++)
//            {
//                hOutputFile << ppfAudioData[c][i] << "\t";
//            }
//            hOutputFile << endl;
//        }
    }

    cout << "\nreading/writing done in: \t" << (clock() - time) * 1.F / CLOCKS_PER_SEC << " seconds." << endl;

    //////////////////////////////////////////////////////////////////////////////
    // clean-up (close files and free memory)
    CAudioFileIf::destroy(phAudioFile);
    CCombFilter::destroy(phFilterFile);
    hOutputFile.close();

    for (int i = 0; i < stFileSpec.iNumChannels; i++)
    {
        delete[] ppfAudioData[i];
        delete[] ppfFilterData[i];
    }
    delete[] ppfAudioData;
    delete[] ppfFilterData;
    ppfAudioData = 0;
    ppfFilterData = 0;
    

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

