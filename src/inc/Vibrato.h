#if !defined(__Vibrato_hdr__)
#define __Vibrato_hdr__

#include "ErrorDef.h"
#include "Lfo.h"
#include "RingBuffer.h"

/*! \brief process audio data with vibrato
*/

class CVibrato {
public:
    /*! version number*/
    enum Version_t {
        kMajor,             //!< major version number
        kMinor,             //!< minor version number
        kPatch,             //!< patch version number
        
        kNumVersionInts
    };
    
    /*!list of parameters for vibrato*/
    enum VibratoParam_t {
        kParamModFreq,          //!< ModFrequency
        kParamModWidth,         //!< ModWidth
        kParamDelay,            //!< Basic Delay
        kParamSampleRate,       //!< Sample Rate
        
        kNumFilterParams
    };
    
    /*! returns the current project version
     \param eVersionIdx specifies what version to return (kMajor, kMinor, ...)
     \return const int
     */
    static const int getVersion (const Version_t eVersionIdx);
    
    /*! returns the date of the build
     \return const char*
     */
    static const char* getBuildDate ();
    
    /*！open a new instance for vibrato
    \param pCVibrato
    \return Error_t
    */
    static Error_t create (CVibrato*& pCVibrato);
    
    /*! destroy vibrato instance
    \param pCVibrato
    \return Error_t
     */
    static Error_t destroy (CVibrato*& pCVibrato);
    
    /*! set basic values, allocate memory for ringbuffer
     \param fDelayInS
     \param fSampleRateInHz
     \param fModWidthInS
     \param fModFreqInHz
     \param iNumChannels
     \return Error_t
     */
    Error_t init (float fDelayInS, float fSampleRateInHz, float fModWidthInS, float fModFreqInHz, int iNumChannels);
    
    /*! reset instance to initial state
     \return Error_t
     */
    Error_t reset ();
    
    /*! set values for parameters
     \param eParam
     \return Error_t
     */
    Error_t setParam (VibratoParam_t eParam, float fParamValue);
    
    /*! processes one block of audio
     \param ppfInputBuffer input buffer [numChannels][iNumberOfFrames]
     \param ppfOutputBuffer output buffer [numChannels][iNumberOfFrames]
     \param iNumberOfFrames buffer length (per channel)
     \return Error_t
     */
    float getParam(VibratoParam_t eParam) const;
    
    /*! process one block of audio
     \param ppfInputBuffer input buffer [numChannels][iNumberOfFrames]
     \param ppfOutputBuffer output buffer [numChannels][iNumberOfFrames]
     \param iNumberOfFrames buffer length (per channel)
     \return Error_t
     */
    Error_t process (float **ppfInputBuffer, float **ppfOutputBuffer, int iNumberOfFrames);
    
protected:
    CVibrato ();
    virtual ~CVibrato ();
    
private:
    bool        m_bIsInitialized;
    CLfo        *m_pCLfo;
    CVibrato    *m_pCVibrato;
    CRingBuffer<float> **m_ppCRingBuffer;
    float       m_fSampleRateInHz;
    float       m_fDelayInS;
    float       m_fModWidthInS;
    float       m_fModFreqInHz;
    int         m_iNumChannels;
    int         m_iNumberOfFrames;
};


#endif // #if !defined(__Vibrato_hdr__)
