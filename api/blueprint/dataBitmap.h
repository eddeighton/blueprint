#ifndef DATABITMAP_14_09_2013
#define DATABITMAP_14_09_2013

#include "blueprint/cgalSettings.h"

#include "buffer.h"

#include <vector>

namespace Blueprint
{
    
class Site;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class DataBitmap
{
public:
    class Claim
    {
    public:
        typedef std::vector< Claim > Vector;

        Claim( Float x, Float y, NavBitmap::Ptr pBuffer, boost::shared_ptr< Site > pSite );

        Float m_x, m_y;
        NavBitmap::Ptr m_pBuffer;
        boost::shared_ptr< Site > m_pSite;
    };

    bool makeClaim( const Claim& claim );

    const Claim::Vector& getClaims() const { return m_claims; }
private:
    Claim::Vector m_claims;
};

}

#endif //DATABITMAP_14_09_2013