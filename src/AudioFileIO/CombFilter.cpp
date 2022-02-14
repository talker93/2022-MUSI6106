//
//  CombFilter.cpp
//  AudioFileIO
//
//  Created by 江山 on 2/13/22.
//

#include <cassert>
#include <iostream>
#include <string.h>
#include <vector>
#include <fstream>

#include "Util.h"
#include "CombFilter.h"
#include "ErrorDef.h"

#ifdef WITH_SNDLIB
#include "sndlib.h"
#endif //WITH_SNDLIB

using namespace std;

Error_t CCombFilter::process( float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames)
{
    float weight = 0.5;
    
    vector<float>input;
    for(int i=0; i<100; i++) {
        input.push_back(0);
    }
    input[0] = 1;

    vector<float>output;

    vector<float>delayLine;
    for(int i=0; i<10; i++) {
        delayLine.push_back(0);
//        cout << delayLine[i] << ",";
    }

    for(int n = 0; n < input.size(); n++) {
        output.push_back(input[n] + weight * delayLine[delayLine.size()-1]);
        delayLine.pop_back();
        delayLine.push_back(input[n]);
        rotate(delayLine.begin(), delayLine.begin()+(delayLine.size()-1),delayLine.end());
//        for(int i=0; i<10; i++) {
//            cout << delayLine[i] << ",";
//        }
        cout << output[n] << "," << n << endl;
    }
    
    
    
    return Error_t::kNoError;
}
