//
//  CombFilter.h
//  MUSI6106
//
//  Created by 江山 on 2/13/22.
//

#ifndef CombFilter_h
#define CombFilter_h

#endif /* CombFilter_h */

#include "CombFilterIf.h"

#define WITH_SNDLIB

class CCombFilterBase; // in case you intend to add an internal base class that the user doesn't see (not required)


class CCombFilter
{
public:
    Error_t process (float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames);
    
protected:
    CCombFilter();
    virtual ~CCombFilter();
};
