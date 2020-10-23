#include "spaces/basicarea.h"

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
/*
class PerimeterWidth_Interaction : public IInteraction
{
    friend class Area;
public:
    struct InitialValue
    {
        float x,y;
        InitialValue( float _x, float _y ) : x( _x ), y( _y ) {}
    };
    typedef std::vector< InitialValue > InitialValueVector;

private:
    PerimeterWidth_Interaction( Area& area, float x, float y )
        :   m_area( area )
    {
        m_startX = x;
        m_startY = y;
        m_fPerimeter = m_area.m_fPerimeterWidth;
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        float fDeltaX = x - m_startX;
        float fDeltaY = y - m_startY;

        m_area.m_fPerimeterWidth = Map_FloorAverageMin( 1.0f )( m_fPerimeter + fDeltaX );
    }

private:
    Area& m_area;
    float m_startX, m_startY, m_fPerimeter;
    InitialValueVector m_initialValues;
};
*/
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/*
class Brush_Interaction : public IInteraction
{
    friend class Area;
public:
    struct InitialValue
    {
        float x,y;
        InitialValue( float _x, float _y ) : x( _x ), y( _y ) {}
    };
    typedef std::vector< InitialValue > InitialValueVector;

private:
    Brush_Interaction( Area& area, float x, float y, float qX, float qY )
        :   m_area( area ),
            m_qX( qX ),
            m_qY( qY )
    {
        m_points.push_back( wykobi::make_point( x, y ) );
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        m_points.push_back( wykobi::make_point( x, y ) );
        
    }

private:
    Area& m_area;
    float m_startX, m_startY, m_qX, m_qY;
    PointVector m_points;
    InitialValueVector m_initialValues;
};
*/



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
const std::string& Area::TypeName()
{
    static const std::string strTypeName( "area" );
    return strTypeName;
}

Area::Area( Site::Ptr pParent, const std::string& strName )
    :   Site( pParent, strName ),
        m_pSiteParent( pParent ),
        m_ptOrigin( wykobi::make_point( 0.0f, 0.0f ) ),
        m_ptOffset( wykobi::make_point( 0.0f, 0.0f ) )
{
}

Area::Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_pSiteParent( pParent ),
        m_ptOrigin( pOriginal->m_ptOrigin ),
        m_ptOffset( pOriginal->m_ptOffset )
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

    m_ptOrigin.x = Map_FloorAverage()( x );
    m_ptOrigin.y = Map_FloorAverage()( y );
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

    if( !m_pLabel.get() )
        m_pLabel.reset( new TextImpl( this, Node::getName(), 0.0f, 0.0f ) ); 
    
    if( !m_pPath.get() )
        m_pPath.reset( new PathImpl( m_path, this ) );
    
    if( !m_pPath2.get() )
        m_pPath2.reset( new PathImpl( m_path2, this ) );

    m_boundaryPoints.clear();
    for_each_recursive( 
        generics::collectIfConvert( m_boundaryPoints, 
            Node::ConvertPtrType< Feature_ContourSegment >(), 
            Node::ConvertPtrType< Feature_ContourSegment >() ),
            Node::ConvertPtrType< Site >() );

    if( !m_pBuffer ) m_pBuffer.reset( new NavBitmap( 1u, 1u ) );
}

void Area::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_ptOrigin.x >> m_ptOrigin.y;
    }
}

void Area::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
    os << m_ptOrigin.x << m_ptOrigin.y;
}

std::string Area::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_ptOrigin.x << m_ptOrigin.y;
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
        
        const wykobi::rectangle< float > aabbBox = wykobi::aabb( polygon );

        m_pLabel->setPos( wykobi::centroid( aabbBox ) );
        
        //update the paths
        {
            PathImpl::aggPathToMarkupPath( m_path, Adaptor( WykobiPolygonAdaptor( polygon ), true ) );
            {
                wykobi::polygon< float, 2 > extrudedContour;
                offsetSimplePolygon( polygon, extrudedContour, 2.0 );
                PathImpl::aggPathToMarkupPath( m_path2, Adaptor( WykobiPolygonAdaptor( extrudedContour ), true ) );
            }
            m_polygonCache = polygon;
        }
        
        //this does not work due to the way the SpaceGlyphs::SpaceGlyphs creates the m_pImageGlyph somehow?
        /*if( !mode.bBitmap )
        {
            if( m_pBuffer )
                m_pBuffer.reset();
            m_ptOffset = calculateOffset( aabbBox );
        }
        else*/
        {
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
        }
    }

    Site::EvaluationResult result;

    for( PtrVector::iterator i = m_spaces.begin(),
        iEnd = m_spaces.end(); i!=iEnd; ++i )
        (*i)->evaluate( mode, data );

    return result;
}

void Area::getContour( FloatPairVector& contour ) 
{ 
    ASSERT( m_pContour );
    if( m_pContour )
    {
        wykobi::point2d< float > ptOrigin = m_ptOrigin;
        Site::Ptr pIter = shared_from_this();
        while( pIter = boost::dynamic_pointer_cast< Site >( pIter->Node::getParent() ) )
            ptOrigin = ptOrigin + wykobi::make_vector( pIter->getX(), pIter->getY() );

        const wykobi::polygon< float, 2u >& polygon = m_pContour->get();
        for( wykobi::polygon< float, 2u >::const_iterator 
            i = polygon.begin(),
            iEnd = polygon.end(); i!=iEnd; ++i )
        {
            contour.push_back( FloatPair( ptOrigin.x + i->x, ptOrigin.y + i->y ) );
        }
    }
}
/*
bool Area::canEditWithTool( const GlyphSpecProducer* pGlyphPrd, unsigned int uiToolType ) const
{
    bool bCanEdit = false;
    if( uiToolType == IEditContext::eSelect || 
        uiToolType == IEditContext::eDraw )
        bCanEdit = true;
    else
    {
        switch( uiToolType )
        {
            case ePerimeterWidth:

                break;
            case eBrush:

                break;
            case eBoundary:
                if( dynamic_cast< const Feature_ContourSegment* >( pGlyphPrd ) )
                    bCanEdit = true;
                break;
            case ePoly:
                if( pGlyphPrd )
                {
                    if( m_pContour.get() == pGlyphPrd || pGlyphPrd->getParent() == m_pContour )
                        bCanEdit = true;
                }
                break;
            default:
                break;
        }
    }
    return bCanEdit;
}*/

void Area::getCmds( CmdInfo::List& cmds ) const
{
    cmds.push_back( CmdTarget::CmdInfo( "SomeCmd", eSomeCmd ) );
}

}