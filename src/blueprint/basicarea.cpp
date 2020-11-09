#include "blueprint/basicarea.h"

#include "blueprint/edit.h"

#include "ed/ed.hpp"

#include "common/rounding.hpp"

#include <cmath>

namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Clip::TypeName()
{
    static const std::string strTypeName( "clip" );
    return strTypeName;
}

Clip::Clip( Node::Ptr pParent, const std::string& strName )
:   Site( pParent, strName )
{
}

Clip::Clip( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
:   Site( pOriginal, pParent, strName )
{
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Area::TypeName()
{
    static const std::string strTypeName( "area" );
    return strTypeName;
}

Area::Area( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName ),
        m_pSiteParent( pParent )
{
}

Area::Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_pSiteParent( pParent ),
        m_transform( pOriginal->m_transform )
{
}

Area::~Area()
{
}

Node::Ptr Area::copy( Node::Ptr pParent, const std::string& strName ) const
{
    Site::Ptr pSiteParent = boost::dynamic_pointer_cast< Site >( pParent );
    VERIFY_RTE( pSiteParent || !pParent );
    return Node::copy< Area >( 
        boost::dynamic_pointer_cast< const Area >( shared_from_this() ), pSiteParent, strName );
}

void Area::init( float x, float y, bool bEmptyContour )
{
    if( bEmptyContour )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
        m_pContour->init();
        add( m_pContour );
    }
    
    init();
    
    m_transform.setTranslation( Map_FloorAverage()( x ), Map_FloorAverage()( y ) );
}

void Area::init()
{
    Site::init();

    if( !( m_pContour = get< Feature_Contour >( "contour" ) ) )
    {
        m_pContour = Feature_Contour::Ptr( new Feature_Contour( shared_from_this(), "contour" ) );
        m_pContour->init();
        m_pContour->set( wykobi::make_rectangle< float >( -15, -15, 15, 15 ) );
        add( m_pContour );
    }
    
    if( !m_pPath.get() )
        m_pPath.reset( new PathImpl( m_path, this ) );

    m_boundaryPoints.clear();
    for_each_recursive( 
        generics::collectIfConvert( m_boundaryPoints, 
            Node::ConvertPtrType< Feature_ContourSegment >(), 
            Node::ConvertPtrType< Feature_ContourSegment >() ),
            Node::ConvertPtrType< Site >() );

    m_strLabelText.clear();
    {
        std::ostringstream os;
        os << Node::getName();
        {
            PropertyVector m_properties;
            for_each_recursive( 
                generics::collectIfConvert( m_properties, 
                    Node::ConvertPtrType< Property >(), 
                    Node::ConvertPtrType< Property >() ),
                    Node::ConvertPtrType< Site >() );
                    
            for( Property::Ptr pProperty : m_properties )
            {
                os << "\n" << pProperty->getName() << ": " << pProperty->getValue();
            }
        }
        m_strLabelText = os.str();
    }

    if( !m_pLabel.get() )
        m_pLabel.reset( new TextImpl( this, m_strLabelText, 0.0f, 0.0f ) ); 
    
    //if( !m_pBuffer ) m_pBuffer.reset( new NavBitmap( 1u, 1u ) );
}

void Area::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_transform;
    }
}

void Area::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) 
    {
        node.statement.shorthand = Ed::Shorthand();
    }
    
    {
        Ed::OShorthandStream os( node.statement.shorthand.get() );
        os << m_transform;
    }
}

std::string Area::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_transform;
        }
        os << sh;
    }
    return os.str();
}

bool Area::canEvaluate( const Site::PtrVector& evaluated ) const
{
    bool bCanEvaluate = true;
    return bCanEvaluate;
}

Site::EvaluationResult Area::evaluate( const EvaluationMode& mode, DataBitmap& data )
{
    typedef PathImpl::AGGContainerAdaptor< wykobi::polygon< float, 2 > > WykobiPolygonAdaptor;
    typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
    bool bSuccess = true;

    const wykobi::polygon< float, 2u >& polygon = m_pContour->get();

    if( !m_polygonCache || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ))
    {
        /*
        bool bIsInteriorArea = false;
        wykobi::point2d< float > ptOrigin = m_ptOrigin;
        {
            int iParentCount = 0;
            Site::Ptr pIter = shared_from_this();
            while( pIter = boost::dynamic_pointer_cast< Site >( pIter->Node::getParent() ) )
            {
                ptOrigin = ptOrigin + wykobi::make_vector( pIter->getX(), pIter->getY() );
                ++iParentCount;
            } 
            bIsInteriorArea = iParentCount > 1;
        }
        */
        
        //const wykobi::rectangle< float > aabbBox = wykobi::aabb( polygon );
        //auto labelPos = wykobi::centroid( aabbBox );
        
       //labelPos.x -= 2.0f;
        //labelPos.y += 3.0f;
        //m_pLabel->setPos( labelPos );
        
        /*
        for( std::shared_ptr< TextImpl > p : m_propertyLabels )
        {
            labelPos.y -= 3.0f;
            p->setPos( labelPos );
        }*/
        
        //update the paths
        {
            PathImpl::aggPathToMarkupPath( m_path, Adaptor( WykobiPolygonAdaptor( polygon ), true ) );
            //{
            //    wykobi::polygon< float, 2 > extrudedContour;
            //    offsetSimplePolygon( polygon, extrudedContour, 2.0 );
            //    PathImpl::aggPathToMarkupPath( m_path2, Adaptor( WykobiPolygonAdaptor( extrudedContour ), true ) );
            //}
            m_polygonCache = polygon;
        }
        
        //this does not work due to the way the SpaceGlyphs::SpaceGlyphs creates the m_pImageGlyph somehow?
        //if( !mode.bBitmap )
        {
            //if( m_pBuffer )
            //    m_pBuffer.reset();
            //m_ptOffset = calculateOffset( aabbBox );
        }
        //else*/
        /*{
            if( !m_pBuffer )
                m_pBuffer.reset( new NavBitmap( 1u, 1u ) );
    
            //allocate the buffer on demand
            m_ptOffset = sizeBuffer( m_pBuffer, aabbBox );

            //rasterize bounding rect space
            Rasteriser ras( m_pBuffer );
            if( bIsInteriorArea )
                ras.renderPath( Adaptor( WykobiPolygonAdaptor( polygon ), true ),
                    Rasteriser::ColourType( 1u ), -m_ptOffset.x, -m_ptOffset.y, 0.0f );
            else
                ras.renderPath( Adaptor( WykobiPolygonAdaptor( polygon ), true ),
                    Rasteriser::ColourType( 1u ), -m_ptOffset.x, -m_ptOffset.y, 1.0f );
                    
            //ras.renderPath( Adaptor( WykobiPolygonAdaptor( outerPolygon ), true ),
            //    Rasteriser::ColourType( 255u ), -m_ptOffset.x, -m_ptOffset.y, 1.0f );

            data.makeClaim( DataBitmap::Claim( 
                ptOrigin.x + m_ptOffset.x, 
                ptOrigin.y + m_ptOffset.y, m_pBuffer, shared_from_this() ) );

            m_pBuffer->setModified();
            
            //mess with the bitmap
            //PerimeterVisitor::bfs( PerimeterVisitor::makeI2( 0, 0 ), PerimeterVisitor( m_pBuffer ) );
        }*/
    }

    Site::EvaluationResult result;

    for( PtrVector::iterator i = m_spaces.begin(),
        iEnd = m_spaces.end(); i!=iEnd; ++i )
        (*i)->evaluate( mode, data );

    return result;
}


void Area::getAbsoluteContour( FloatPairVector& contour ) 
{ 
    ASSERT( m_pContour );
    if( m_pContour )
    {
        Matrix absTransform;
        getAbsoluteTransform( absTransform );

        const wykobi::polygon< float, 2u >& polygon = m_pContour->get();
        for( wykobi::polygon< float, 2u >::const_iterator 
            i = polygon.begin(),
            iEnd = polygon.end(); i!=iEnd; ++i )
        {
            float x = i->x, y = i->y;
            absTransform.transform( x, y );
            contour.push_back( FloatPair( x, y ) );
        }
    }
}

void Area::setTransform( const Transform& transform )
{ 
    m_transform = transform; 
    setModified();
}
void Area::cmd_rotateLeft()
{
    m_transform.rotateLeft();
    setModified();
}

void Area::cmd_rotateRight()
{
    m_transform.rotateRight();
    setModified();
}

void Area::cmd_flipHorizontally()
{
    m_transform.flipHorizontally();
    setModified();
}

void Area::cmd_flipVertically()
{
    m_transform.flipVertically();
    setModified();
}

}