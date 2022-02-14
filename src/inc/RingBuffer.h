#if !defined(__RingBuffer_hdr__)
#define __RingBuffer_hdr__

#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstring>

/*! \brief implement a circular buffer of type T
*/
template <class T> 
class CRingBuffer
{
public:
    explicit CRingBuffer(int iBufferLengthInSamples) :
        m_iBuffLength(iBufferLengthInSamples)
    {
        assert(iBufferLengthInSamples > 0);

        // allocate and init
    }

    virtual ~CRingBuffer()
    {
        // free memory
    }

    /*! add a new value of type T to write index and increment write index
    \param tNewValue the new value
    \return void
    */
    void putPostInc (T tNewValue)
    {
    }

    /*! add a new value of type T to write index
    \param tNewValue the new value
    \return void
    */
    void put(T tNewValue)
    {
    }
    
    /*! return the value at the current read index and increment the read pointer
    \return float the value from the read index
    */
    T getPostInc()
    {
        return static_cast<T>(-1);
    }

    /*! return the value at the current read index
    \return float the value from the read index
    */
    T get() const
    {
        return static_cast<T>(-1);
    }
    
    /*! set buffer content and indices to 0
    \return void
    */
    void reset()
    {
    }

    /*! return the current index for writing/put
    \return int
    */
    int getWriteIdx() const
    {
        return -1;
    }

    /*! move the write index to a new position
    \param iNewWriteIdx: new position
    \return void
    */
    void setWriteIdx(int iNewWriteIdx)
    {
    }

    /*! return the current index for reading/get
    \return int
    */
    int getReadIdx() const
    {
        return -1;
    }

    /*! move the read index to a new position
    \param iNewReadIdx: new position
    \return void
    */
    void setReadIdx(int iNewReadIdx)
    {
    }

    /*! returns the number of values currently buffered (note: 0 could also mean the buffer is full!)
    \return int
    */
    int getNumValuesInBuffer() const
    {
        return -1;
    }

    /*! returns the length of the internal buffer
    \return int
    */
    int getLength() const
    {
        return -1;
    }
private:
    CRingBuffer();
    CRingBuffer(const CRingBuffer& that);

    int m_iBuffLength;              //!< length of the internal buffer
};
#endif // __RingBuffer_hdr__
