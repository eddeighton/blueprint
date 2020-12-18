#include "blueprint/dataBitmap.h"

#include "blueprint/site.h"


namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
DataBitmap::Claim::Claim( Float x, Float y, NavBitmap::Ptr pBuffer, boost::shared_ptr< Site > pSite )
    :   m_x( x ),
        m_y( y ),
        m_pBuffer( pBuffer ),
        m_pSite( pSite )
{
}

bool DataBitmap::makeClaim( const DataBitmap::Claim& claim )
{
    bool bValid = true;

    //verify the claim is valid...
    /*for( Claim::Vector::iterator i = m_claims.begin(),
        iEnd = m_claims.end(); i!=iEnd; ++i )
    {

    }*/

    m_claims.push_back( claim );

    return bValid;
}


}