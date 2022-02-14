
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

    showClInfo();

    //////////////////////////////////////////////////////////////////////////////
    // parse command line arguments
    if (argc < 2)
    {
        cout << "Missing audio input paths!";
        return -1;
    }
    else
    {
        sInputFilePath = argv[1];
        sOutputFilePath = sInputFilePath + ".txt";
    }

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
    phFilterFile->init(CCombFilter::kCombFIR, 1, stFileSpec.fSampleRateInHz, stFileSpec.iNumChannels);
    phFilterFile->setParam(CCombFilter::kParamGain, 1);
    phFilterFile->setParam(CCombFilter::kParamDelay, 20);

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
        float weight = 0.5;
        string filter_type = "FIR";

        vector<float>output1;
        vector<float>output2;

        vector<float>delayLine;
        for(int i=0; i<10; i++)
        {
            delayLine.push_back(0);
        }
        
        for(int n = 0; n < iNumFrames; n++)
        {
            output1.push_back(ppfAudioData[0][n] + weight * delayLine[delayLine.size()-1]);
            delayLine.pop_back();
            if (filter_type == "FIR")
            {
                delayLine.push_back(ppfAudioData[0][n]);
            } else if (filter_type == "IIR")
            {
                delayLine.push_back(output1[n]);
            }
            rotate(delayLine.begin(), delayLine.begin()+(delayLine.size()-1),delayLine.end());
        }
        
        for(int n = 0; n < iNumFrames; n++)
        {
            output2.push_back(ppfAudioData[1][n] + weight * delayLine[delayLine.size()-1]);
            delayLine.pop_back();
            if (filter_type == "FIR")
            {
                delayLine.push_back(ppfAudioData[1][n]);
            } else if (filter_type == "IIR")
            {
                delayLine.push_back(output2[n]);
            }
            rotate(delayLine.begin(), delayLine.begin()+(delayLine.size()-1),delayLine.end());
        }
        
        for (int i = 0; i < iNumFrames; i++)
        {
            for (int c = 0; c < stFileSpec.iNumChannels; c++)
            {
                if (c == 0)
                {
                    hOutputFile << output1[i] << "\t";
                } else if (c == 1)
                {
                    hOutputFile << output2[i] << "\t";
                }
            }
            hOutputFile << endl;
        }
        
        

        // write
//        for (int i = 0; i < iNumFrames; i++)
//        {
//            for (int c = 0; c < stFileSpec.iNumChannels; c++)
//            {
//                hOutputFile << ppfFilterData[c][i] << "\t";
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

