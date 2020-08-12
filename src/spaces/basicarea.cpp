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

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
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


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Boundary_Interaction : public IInteraction
{
    friend class Area;
public:
    /*
    struct InitialValue
    {
        float x,y;
        InitialValue( float _x, float _y ) : x( _x ), y( _y ) {}
    };
    typedef std::vector< InitialValue > InitialValueVector;
    */
private:
    Boundary_Interaction( Area& area, float x, float y, float qX, float qY )
        :   m_area( area ),
            m_qX( qX ),
            m_qY( qY ),
            m_pBoundaryPoint( new Feature_ContourSegment( area.m_pContour, area.m_pContour->generateNewNodeName( "boundary" ) ) )
    {
        m_startX = x = Math::quantize_roundUp( x, qX );
        m_startY = y = Math::quantize_roundUp( y, qY );
        VERIFY_RTE( area.m_pContour->add( m_pBoundaryPoint ) );
        m_area.m_boundaryPoints.push_back( m_pBoundaryPoint );
        
        const wykobi::point2d< float >& origin = m_area.m_pContour->get()[0];
        OnMove( x, y );
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        float fDeltaX = Math::quantize_roundUp( x, m_qX );
        float fDeltaY = Math::quantize_roundUp( y, m_qY );

        const wykobi::point2d< float >& origin = m_area.m_pContour->get()[0];
        m_pBoundaryPoint->set( 0, fDeltaX - origin.x, fDeltaY - origin.y );
    }

private:
    Area& m_area;
    Feature_ContourSegment::Ptr m_pBoundaryPoint;
    float m_startX, m_startY, m_qX, m_qY;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Polygon_Interaction : public IInteraction
{
    friend class Area;
private:
    Polygon_Interaction( Area& area, float x, float y, float qX, float qY )
        :   m_area( area ),
            m_qX( qX ),
            m_qY( qY )
    {
        const wykobi::point2d< float >& origin = m_area.m_pContour->get()[0];
        m_startX = x = Math::quantize_roundUp( x, qX );
        m_startY = y = Math::quantize_roundUp( y, qY );

        Feature_Contour::Ptr pContour = area.m_pContour;

        wykobi::polygon< float, 2 > poly = pContour->get();
        m_iPointIndex = poly.size();
        poly.push_back( wykobi::make_point< float >( m_startX, m_startY ) );
        area.m_pContour->set( poly );

        OnMove( x, y );
    }
    
public:
    virtual void OnMove( float x, float y )
    {
        const float fDeltaX = Math::quantize_roundUp( x, m_qX );
        const float fDeltaY = Math::quantize_roundUp( y, m_qY );

        m_area.m_pContour->set( m_iPointIndex, fDeltaX, fDeltaY  );
    }

private:
    Area& m_area;
    float m_startX, m_startY, m_qX, m_qY;
    int m_iPointIndex;
};



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
        m_ptOffset( wykobi::make_point( 0.0f, 0.0f ) ),
        m_fPerimeterWidth( 1.0f )
{
}

Area::Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName )
    :   Site( pOriginal, pParent, strName ),
        m_pSiteParent( pParent ),
        m_ptOrigin( pOriginal->m_ptOrigin ),
        m_ptOffset( pOriginal->m_ptOffset ),
        m_fPerimeterWidth( pOriginal->m_fPerimeterWidth )
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

void Area::init( float x, float y )
{
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

Site::EvaluationResult Area::evaluate( DataBitmap& data )
{
    bool bSuccess = true;

    const wykobi::polygon< float, 2u >& polygon = m_pContour->get();

    if( !m_polygonCache || 
        !( m_polygonCache.get().size() == polygon.size() ) || 
        !std::equal( polygon.begin(), polygon.end(), m_polygonCache.get().begin() ) || 
        !m_fPerimeterWidthCache || 
        !(m_fPerimeterWidthCache.get() == m_fPerimeterWidth))
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

        wykobi::polygon< float, 2u > outerPolygon;

        //if( m_fPerimeterWidth > 0.0f )
        //    offsetSimplePolygon( polygon, outerPolygon, m_fPerimeterWidth );
        //else
            outerPolygon = polygon;

        const wykobi::rectangle< float > aabbBox = wykobi::aabb( outerPolygon );

        m_pLabel->setPos( wykobi::centroid( aabbBox ) );
    
        //allocate the buffer on demand
        m_ptOffset = sizeBuffer( m_pBuffer, aabbBox );

        //rasterize bounding rect space
        typedef PathImpl::AGGContainerAdaptor< wykobi::polygon< float, 2 > > WykobiPolygonAdaptor;
        typedef agg::poly_container_adaptor< WykobiPolygonAdaptor > Adaptor;
    
        Rasteriser ras( m_pBuffer );
        if( bIsInteriorArea )
            ras.renderPath( Adaptor( WykobiPolygonAdaptor( outerPolygon ), true ),
                Rasteriser::ColourType( 1u ), -m_ptOffset.x, -m_ptOffset.y, 0.0f );
        else
            ras.renderPath( Adaptor( WykobiPolygonAdaptor( outerPolygon ), true ),
                Rasteriser::ColourType( 1u ), -m_ptOffset.x, -m_ptOffset.y, 1.0f );
        //ras.renderPath( Adaptor( WykobiPolygonAdaptor( outerPolygon ), true ),
        //    Rasteriser::ColourType( 255u ), -m_ptOffset.x, -m_ptOffset.y, 1.0f );

        
        data.makeClaim( DataBitmap::Claim( 
            ptOrigin.x + m_ptOffset.x, 
            ptOrigin.y + m_ptOffset.y, m_pBuffer, shared_from_this() ) );

        PathImpl::aggPathToMarkupPath( m_path, Adaptor( WykobiPolygonAdaptor( polygon ), true ) );

        //mess with the bitmap
        //PerimeterVisitor::bfs( PerimeterVisitor::makeI2( 0, 0 ), PerimeterVisitor( m_pBuffer ) );
        m_polygonCache = polygon;
        m_fPerimeterWidthCache = m_fPerimeterWidth;
        m_pBuffer->setModified();
    }

    Site::EvaluationResult result;

    for( PtrVector::iterator i = m_spaces.begin(),
        iEnd = m_spaces.end(); i!=iEnd; ++i )
        (*i)->evaluate( data );

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
}

void Area::getCmds( CmdInfo::List& cmds ) const
{
    cmds.push_back( CmdTarget::CmdInfo( "SomeCmd", eSomeCmd ) );
}

void Area::getTools( ToolInfo::List& tools ) const
{
    //tools.push_back( CmdTarget::ToolInfo( "Perimeter Width", ePerimeterWidth ) );
    //tools.push_back( CmdTarget::ToolInfo( "Brush", eBrush ) );
    tools.push_back( CmdTarget::ToolInfo( "Boundary", eBoundary ) );
    tools.push_back( CmdTarget::ToolInfo( "Polygon", ePoly ) );
}

IInteraction::Ptr Area::beginToolDraw( unsigned int uiTool, float x, float y, float qX, float qY, Site::Ptr pClip )
{
    IInteraction::Ptr pInteraction;
    switch( uiTool )
    {
        case ePerimeterWidth:
            //pInteraction.reset( new PerimeterWidth_Interaction( *this, x, y ) );
            break;
        case eBrush:
            //pInteraction.reset( new Brush_Interaction( *this, x, y, qX, qY ) );
            break;
        case eBoundary:
            pInteraction.reset( new Boundary_Interaction( *this, x, y, qX, qY ) );
            break;
        case ePoly:
            pInteraction.reset( new Polygon_Interaction( *this, x, y, qX, qY ) );
            break;
        default:
            break;
    }
    return pInteraction;
}

IInteraction::Ptr Area::beginTool( unsigned int uiTool, float x, float y, float qX, float qY, 
    GlyphSpecProducer* pHit, const std::set< GlyphSpecProducer* >& selection )
{
    IInteraction::Ptr pInteraction;
    switch( uiTool )
    {
        case ePerimeterWidth:
            //pInteraction.reset( new PerimeterWidth_Interaction( *this, x, y ) );
            break;
        case eBrush:
            //pInteraction.reset( new Brush_Interaction( *this, x, y, qX, qY ) );
            break;
        case eBoundary:
            pInteraction.reset( new Boundary_Interaction( *this, x, y, qX, qY ) );
            break;
        case ePoly:
            pInteraction.reset( new Polygon_Interaction( *this, x, y, qX, qY ) );
            break;
        default:
            break;
    }
    return pInteraction;
}

}