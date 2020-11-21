

#include "blueprint/edit.h"

#include "blueprint/connection.h"
#include "ed/ed.hpp"

#include "common/compose.hpp"
#include "common/assert_verify.hpp"

#include "wykobi.hpp"
#include "wykobi_algorithm.hpp"

#include <boost/numeric/conversion/bounds.hpp>

#include <algorithm>
#include <iomanip>

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Connection::TypeName()
{
    static const std::string strTypeName( "connection" );
    return strTypeName;
}

Connection::Connection( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName ),
        m_pSiteParent( pParent )
{

}

Connection::Connection( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_pSiteParent( pParent ),
        m_transform( pOriginal->m_transform )
{
}

Node::Ptr Connection::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Connection >( 
        boost::dynamic_pointer_cast< const Connection >( shared_from_this() ), pSiteParent, strName );
}

void Connection::init()
{
    Site::init();
    
    if( !( m_pSourceRef = get< Reference >( "source" ) ) )
    {
        m_pSourceRef = Reference::Ptr( new Reference( shared_from_this(), "source" ) );
        m_pSourceRef->init();
        add( m_pSourceRef );
    }
    if( !( m_pTargetRef = get< Reference >( "target" ) ) )
    {
        m_pTargetRef = Reference::Ptr( new Reference( shared_from_this(), "target" ) );
        m_pTargetRef->init();
        add( m_pTargetRef );
    }
    //if( !m_pBuffer ) m_pBuffer.reset( new NavBitmap( 1u, 1u ) );
    
    if( !m_pPath.get() )
        m_pPath.reset( new PathImpl( m_path, this ) );
}
void Connection::init( const Ed::Reference& sourceRef, const Ed::Reference& targetRef )
{
    Connection::init();

    m_pSourceRef->setValue( sourceRef );
    m_pTargetRef->setValue( targetRef );
}

void Connection::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
}

void Connection::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Site::save( node );
}

wykobi::point2d< float > getContourSegmentPointAbsolute( Feature_ContourSegment::Ptr pPoint, Feature_ContourSegment::Points pointType )
{
    ASSERT( pPoint );
    
    Feature_Contour::Ptr pContour = boost::dynamic_pointer_cast< Feature_Contour >( pPoint->Node::getParent() );
    ASSERT( pContour );
    
    Area::Ptr pArea = boost::dynamic_pointer_cast< Area >( pContour->Node::getParent() );
    ASSERT( pArea );
    
    float x = pContour->getX( 0u ) + pPoint->getX( pointType );
    float y = pContour->getY( 0u ) + pPoint->getY( pointType );
    
    pArea->getTransform().transform( x, y );
    
    return wykobi::make_point< float >( x, y );
}

Site::EvaluationResult Connection::evaluate( const EvaluationMode& mode, DataBitmap& data )
{
    EvaluationResult result;

    ASSERT( m_pSourceRef && m_pTargetRef );
    
    RefPtr pSource( shared_from_this(), m_pSourceRef->getValue() );
    RefPtr pTarget( shared_from_this(), m_pTargetRef->getValue() );

    Feature_ContourSegment::Ptr pSourceSegment = pSource.get< Feature_ContourSegment >();
    Feature_ContourSegment::Ptr pTargetSegment = pTarget.get< Feature_ContourSegment >();
    if( pSourceSegment && pTargetSegment )
    {
        std::vector< wykobi::point2d< float > > points, convexHull;
        points.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eLeft ) );
        points.push_back( getContourSegmentPointAbsolute( pSourceSegment, Feature_ContourSegment::eRight ) );
        points.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eLeft ) );
        points.push_back( getContourSegmentPointAbsolute( pTargetSegment, Feature_ContourSegment::eRight ) );

        wykobi::algorithm::convex_hull_graham_scan< wykobi::point2d< float > >( 
            points.begin(), points.end(), std::back_inserter( convexHull ) );
    
        wykobi::polygon< float, 2u > contour( convexHull.size() );
        std::copy( convexHull.rbegin(), convexHull.rend(), contour.begin() );
        
        if( !m_polygonCache || !(m_polygonCache.get().size()==contour.size() ) ||
            !std::equal( contour.begin(), contour.end(), m_polygonCache.get().begin() ) )
        {
            //convert polygon to local coords
            {
                wykobi::polygon< float, 2u > temp( contour );
                for( wykobi::point2d< float >& pt : temp )
                {
                    //this only works because a connection NEVER has a rotation or scaling
                    pt.x -= m_transform.X();
                    pt.y -= m_transform.Y();
                }
                typedef PathImpl::AGGContainerAdaptor< wykobi::polygon< float, 2 > > WykobiPolygonAdaptor;
                typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
                PathImpl::aggPathToMarkupPath( m_path, Adaptor( WykobiPolygonAdaptor( temp ), true ) );
            }
            
            
            //const wykobi::rectangle< float > aabbBox = wykobi::aabb( contour );
            //m_ptOffset = sizeBuffer( m_pBuffer, aabbBox );
            //Rasteriser ras( m_pBuffer );
            //typedef PathImpl::AGGContainerAdaptor< wykobi::polygon< float, 2 > > WykobiPolygonAdaptor;
            //typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
            //ras.renderPath( Adaptor( WykobiPolygonAdaptor( contour ), true ),
            //    Rasteriser::ColourType( 255u ), -m_ptOffset.x, -m_ptOffset.y, 0.0f );
            //ras.renderPath( Adaptor( WykobiPolygonAdaptor( contour ), true ),
            //    Rasteriser::ColourType( 1u ), -m_ptOffset.x, -m_ptOffset.y, 1.0f );
            //m_pBuffer->setModified();
            m_polygonCache = contour;
        }
        
        //data.makeClaim( DataBitmap::Claim( m_transform.x + m_ptOffset.x, 
        //    m_transform.y + m_ptOffset.y, m_pBuffer, shared_from_this() ) );
        
    }
        

    return result;
}

}