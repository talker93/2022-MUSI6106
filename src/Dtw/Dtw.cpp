
#include "Vector.h"
#include "Util.h"

#include "Dtw.h"

CDtw::CDtw( void ) :
    m_bIsInitialized(false),
    m_pCDtw(0),
    m_iNumCols(0),
    m_iNumRows(0),
    m_ppfDistanceMatrix(0),
    m_ppfCostMatrix(0),
    m_ppfDeltaMatrix(0),
    m_ppiPathResult(0),
    m_iDec(0)
{
    this -> reset();
}

CDtw::~CDtw( void )
{
    this -> reset();
}

Error_t CDtw::init( int iNumRows, int iNumCols )
{
    m_iNumCols = iNumCols;
    m_iNumRows = iNumRows;

    m_ppfDistanceMatrix = new float* [m_iNumRows];
    for (int i = 0; i < m_iNumRows; i++)
        m_ppfDistanceMatrix[i] = new float[m_iNumCols];

    m_ppfDeltaMatrix = new float* [m_iNumRows];
    for (int i = 0; i < m_iNumRows; i++)
        m_ppfDeltaMatrix[i] = new float[m_iNumCols];

    m_ppfCostMatrix = new float* [m_iNumRows];
    for (int i = 0; i < m_iNumRows; i++)
        m_ppfCostMatrix[i] = new float[m_iNumCols];

    m_ppiPathResult = new int* [m_iNumRows + m_iNumCols - 1];
    for (int i = 0; i < m_iNumRows + m_iNumCols - 1; i++)
        m_ppiPathResult[i] = new int[2];

    m_iDec = new int* [3];
    for (int i = 0; i < 3; i++)
        m_iDec[i] = new int[2];

    m_pCDtw = new CDtw();
    m_bIsInitialized = true;
    
    return Error_t::kNoError;
}

Error_t CDtw::reset()
{
    if(m_bIsInitialized)
    {
        for (int c = 0; c < m_iNumRows; c++)
            delete m_ppfDistanceMatrix[c];
        delete[] m_ppfDistanceMatrix;
        m_ppfDistanceMatrix = 0;

        for (int c = 0; c < m_iNumRows; c++)
            delete m_ppfDeltaMatrix[c];
        delete[] m_ppfDeltaMatrix;
        m_ppfDeltaMatrix = 0;

        for (int c = 0; c < m_iNumRows; c++)
            delete m_ppfCostMatrix[c];
        delete[] m_ppfCostMatrix;
        m_ppfCostMatrix = 0;

        for (int c = 0; c < m_iNumRows + m_iNumCols - 1; c++)
            delete m_ppiPathResult[c];
        delete [] m_ppiPathResult;
        m_ppiPathResult = 0;

        for (int c = 0; c < 3; c++)
            delete m_iDec[c];
        delete [] m_iDec;
        m_iDec = 0;

        delete m_pCDtw;
        m_pCDtw = 0;
    }

    m_iNumCols = 0;
    m_iNumRows = 0;
    m_bIsInitialized    = false;

    return Error_t::kNoError;
}

Error_t CDtw::process(float **ppfDistanceMatrix)
{
    if (!m_bIsInitialized)
        return Error_t::kNotInitializedError;

    if (!ppfDistanceMatrix)
        return Error_t::kFunctionInvalidArgsError;

    // init D matrix
    for (int i = 0; i < m_iNumRows; i++)
    {
        for (int j = 0; j < m_iNumCols; j++)
        {
            m_ppfDistanceMatrix[i][j] = ppfDistanceMatrix[i][j];
        }
    }

    // init C matrix
    for (int i = 0; i < m_iNumRows; i++)
    {
        for (int j = 0; j < m_iNumCols; j++)
        {
            m_ppfCostMatrix[i][j] = 0;
        }
    }

    // calculate the 1st row and column of C matrix
    m_ppfCostMatrix[0][0] = m_ppfDistanceMatrix[0][0];

    for (int j = 1; j < m_iNumCols; j++)
    {
        m_ppfCostMatrix[0][j] = m_ppfCostMatrix[0][j - 1] + m_ppfDistanceMatrix[0][j];
    }

    for (int j = 1; j < m_iNumRows; j++)
    {
        m_ppfCostMatrix[j][0] = m_ppfCostMatrix[j - 1][0] + m_ppfDistanceMatrix[j][0];
    }

    // init Delta Matrix
    for (int i = 0; i < m_iNumRows; i++)
    {
        for (int j = 0; j < m_iNumCols; j++)
        {
            m_ppfDeltaMatrix[i][j] = 0;
        }
    }

    for (int i = 1; i < m_iNumCols; i++)
    {
        m_ppfDeltaMatrix[1][i] = 3;
    }

    for (int i = 1; i < m_iNumRows; i++)
    {
        m_ppfDeltaMatrix[i][1] = 2;
    }

    // init directions for back-tracking
    m_iDec[0][0] = -1;
    m_iDec[0][1] = -1;
    m_iDec[1][0] = -1;
    m_iDec[1][1] = 0;
    m_iDec[2][0] = 0;
    m_iDec[0][2] = -1;
    
    // calculate the rest rows and columns of C matrix
    for (int i = 1; i < m_iNumRows; i++)
    {
        for (int j = 1; j < m_iNumCols; j++)
        {
            float fC_min = argmin(m_ppfCostMatrix[i - 1][j - 1], m_ppfCostMatrix[i - 1][j], m_ppfCostMatrix[i][j]);
            m_ppfCostMatrix[i][j] = m_ppfDistanceMatrix[i][j] + fC_min;
        }
    }

    // init path matrix
    m_ppiPathResult[m_iNumRows][0] = m_iNumRows;
    m_ppiPathResult[m_iNumRows][1] = m_iNumCols;
    float* n = new float [2];
    n[0] = m_iNumRows;
    n[1] = m_iNumCols;
    //while (n[0] > 1 || n[1] > 1)
    //{
    //    n[0] = n[0] + m_iDec[m_ppfDeltaMatrix[n[0]][n[1]]][0];
    //    n[1] = n[1] + m_iDec[m_ppfDeltaMatrix[n[0]][n[1]]][1];
    //}


    return Error_t::kNoError;
}

int CDtw::getPathLength()
{
    return -1;
}

float CDtw::getPathCost() const
{
    return  m_ppfCostMatrix[m_iNumRows][m_iNumCols];
}

Error_t CDtw::getPath( int **ppiPathResult ) const
{

    return Error_t::kNoError;
}


float CDtw::argmin(float a, float b, float c)
{
    if(a<b)
    {
        if(a<c)
        {
            return a;
        }
        else if(c<a)
        {
            return c;
        }
    }
    else
    {
        if(b<c)
        {
            return b;
        }
        else if(c<b)
        {
            return c;
        }
    }
}
