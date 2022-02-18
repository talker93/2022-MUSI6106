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
        m_iBuffLength(iBufferLengthInSamples),
        m_iReadIdx(0),
        m_iWriteIdx(0),
        m_ptBuff(0)
    {
        assert(iBufferLengthInSamples > 0);
        m_ptBuff = new T [m_iBuffLength];
        reset();
        // allocate and init
    }

    virtual ~CRingBuffer()
    {
        delete [] m_ptBuff;
        m_ptBuff = 0;
        // free memory
    }

    /*! add a new value of type T to write index and increment write index
    \param tNewValue the new value
    \return void
    */
    void putPostInc (T tNewValue)
    {
        put(tNewValue);
        incIdx(m_iWriteIdx);
    }

    /*! add a new value of type T to write index
    \param tNewValue the new value
    \return void
    */
    void put(T tNewValue)
    {
        m_ptBuff[m_iWriteIdx]   = tNewValue;
    }
    
    /*! return the value at the current read index and increment the read pointer
    \return float the value from the read index
    */
    T getPostInc()
    {
        T tValue = get();
        incIdx(m_iReadIdx);
        return tValue;
//        return static_cast<T>(-1);
    }

    /*! return the value at the current read index
    \return float the value from the read index
    */
    T get() const
    {
        if (fOffset == 0.f)
            return m_ptBuff[m_iReadIdx];
        else
        {
            assert (std::abs(fOffset) <= m_iBuffLength);

            int iRead   = m_iReadIdx + static_cast<int>(std::floor(fOffset));
            while (iRead > m_iBuffLength-1)
                iRead  -= m_iBuffLength;
            while (iRead < 0)
                iRead  += m_iBuffLength;

            return m_ptBuff[iRead];
        }
//        return static_cast<T>(-1);
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
    
    void incIdx (int &iIdx, int iOffset = 1)
    {
        while((iIdx + iOffset) < 0)
        {
            iOffset += m_iBuffLength;
        }
        iIdx = (iIdx + iOffset) % m_iBuffLength;
    }

    int m_iBuffLength,              //!< length of the internal buffer
        m_iReadIdx,
        m_iWriteIdx;
    
    T   *m_ptBuff;
};
#endif // __RingBuffer_hdr__
