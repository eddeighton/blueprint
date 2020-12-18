#include "blueprint/basicFeature.h"
#include "blueprint/blueprint.h"

#include "blueprint/factory.h"
#include "blueprint/serialisation.h"

#include "ed/ed.hpp"

#include <boost/optional.hpp>

#include "common/assert_verify.hpp"
#include "common/stl.hpp"


namespace Blueprint
{

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const std::string& Feature::TypeName()
{
    static const std::string strTypeName( "feature" );
    return strTypeName;
}
void Feature::init()
{
    Node::init();
}
Node::Ptr Feature::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    return Node::copy< Feature >( shared_from_this(), pParent, strName );
}
void Feature::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
}

void Feature::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const std::string& Feature_Point::TypeName()
{
    static const std::string strTypeName( "point" );
    return strTypeName;
}
Feature_Point::Feature_Point( Node::Ptr pParent, const std::string& strName )
    :   Feature( pParent, strName ),
        m_point( *this, 0 ),
        m_ptOrigin( 0.0f, 0.0f )
{
}
Feature_Point::Feature_Point( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
    :   Feature( pOriginal, pParent, strName ),
        m_point( *this, 0 ),
        m_ptOrigin( pOriginal->m_ptOrigin )
{

}

Node::Ptr Feature_Point::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    return Node::copy< Feature_Point >( 
        boost::dynamic_pointer_cast< const Feature_Point >( shared_from_this() ), pParent, strName );
}

void Feature_Point::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_ptOrigin;
    }
}

void Feature_Point::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
    os << m_ptOrigin;
}

std::string Feature_Point::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_ptOrigin;
        }
        os << sh;
    }
    return os.str();
}
    
const GlyphSpec* Feature_Point::getParent( int id ) const 
{ 
    if( Feature_Point::Ptr pParent = boost::dynamic_pointer_cast< Feature_Point >( m_pParent.lock() ) )
        return &pParent->m_point;
    else if( Site::Ptr pParent = boost::dynamic_pointer_cast< Site >( m_pParent.lock() ) )
        return pParent.get();
    else
        return 0u; 
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const std::string& Feature_Contour::TypeName()
{
    static const std::string strTypeName( "contour" );
    return strTypeName;
}
Feature_Contour::Feature_Contour( Node::Ptr pParent, const std::string& strName )
    :   Feature( pParent, strName )
{
}

Feature_Contour::Feature_Contour( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
    :   Feature( pOriginal, pParent, strName ),
        m_polygon( pOriginal->m_polygon )
{
    recalculateControlPoints();
}

void Feature_Contour::init()
{
    Feature::init();
    
    m_pAutoCalc = get< Property >( "auto" );
    if( !m_pAutoCalc )
    {
        m_pAutoCalc = Property::Ptr( new Property( shared_from_this(), "auto" ) );
        m_pAutoCalc->init();
        m_pAutoCalc->setStatement( "false" );
        add( m_pAutoCalc );
    }
}

Node::Ptr Feature_Contour::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    return Node::copy< Feature_Contour >( 
        boost::dynamic_pointer_cast< const Feature_Contour >( shared_from_this() ), pParent, strName );
}

Feature_Contour::~Feature_Contour()
{
    generics::deleteAndClear( m_points );
}

void Feature_Contour::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );

    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_polygon;
    }

    recalculateControlPoints();
}

void Feature_Contour::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
    os << m_polygon;
}

std::string Feature_Contour::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_polygon;
        }
        os << sh;
    }
    return os.str();
}

bool Feature_Contour::isAutoCalculate() const
{
    if( m_pAutoCalc && m_pAutoCalc->getValue() == "true" )
        return true;
    else
        return false;
}

const GlyphSpec* Feature_Contour::getParent( int id ) const 
{ 
    if( Feature_Point::Ptr pParent = boost::dynamic_pointer_cast< Feature_Point >( m_pParent.lock() ) )
        return &pParent->m_point;
    else if( Site::Ptr pParent = boost::dynamic_pointer_cast< Site >( m_pParent.lock() ) )
        return pParent.get();
    else
        return 0u; 
}

Float Feature_Contour::getX( int id ) const 
{ 
    return CGAL::to_double( m_polygon[ id ].x() ); 
}
Float Feature_Contour::getY( int id ) const 
{ 
    return CGAL::to_double( m_polygon[ id ].y() ); 
}
void Feature_Contour::set( int id, Float fX, Float fY ) 
{ 
    if( id >= 0 && id < m_polygon.size() )
    {
        const Point ptNew( Map_FloorAverage()( fX ), Map_FloorAverage()( fY ) );
        if( m_polygon[ id ] != ptNew )
        {
            m_polygon[ id ] = ptNew;
            setModified();
        }
    }
}

void Feature_Contour::setSinglePoint( Float x, Float y )
{
    m_polygon.clear();
    m_polygon.push_back( Point( Map_FloorAverage()( x ), Map_FloorAverage()( y ) ) );
    recalculateControlPoints();
}

void Feature_Contour::set( const Polygon& shape )
{
    if( !( m_polygon.size() == shape.size() ) || 
        !std::equal( m_polygon.begin(), m_polygon.end(), shape.begin() ) )
    {
        m_polygon = shape;
        for( auto i = m_polygon.begin(),
            iEnd = m_polygon.end(); i!=iEnd; ++i )
            *i = Point( Map_FloorAverage()( CGAL::to_double( i->x() ) ), 
                        Map_FloorAverage()( CGAL::to_double( i->y() ) ) );
        recalculateControlPoints();
    }
}

void Feature_Contour::recalculateControlPoints()
{
    generics::deleteAndClear( m_points );
    if( !isAutoCalculate() )
    {
        int id = 0;
        for( auto i = m_polygon.begin(), 
                iEnd = m_polygon.end(); i!=iEnd; ++i, ++id )
        {
            m_points.push_back( new PointType( *this, id ) );
        }
    }
}

bool Feature_Contour::cmd_delete( const std::vector< const GlyphSpec* >& selection ) 
{ 
    std::vector< int > removals;
    
    {
        int iCounter = m_points.size() - 1;
        for( auto i = m_points.rbegin(),
            iEnd = m_points.rend(); i!=iEnd; ++i, --iCounter )
        {
            for( const GlyphSpec* pGlyphSpec : selection )
            {
                if( pGlyphSpec == *i )
                {
                    removals.push_back( iCounter );
                }
            }
        }
    }
    
    for( int i : removals )
    {
        m_polygon.erase( m_polygon.begin() + i );
        m_points.erase( m_points.begin() + i );
    }
    
    {
        std::size_t sz = 0U;
        for( PointType* pPoint : m_points )
            pPoint->setIndex( sz++ );
    }
    
    return !removals.empty(); 
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/*
const std::string& Feature_ContourPoint::TypeName()
{
    static const std::string strTypeName( "contourpoint" );
    return strTypeName;
}
Feature_ContourPoint::Feature_ContourPoint( Feature_Contour::Ptr pParent, const std::string& strName )
    :   Feature( pParent, strName ),
        m_point( *this, 0 ),
        m_pContour( pParent ),
        m_uiContourPointIndex( 0u ),
        m_fRatio( 0.0f )
{
}
Feature_ContourPoint::Feature_ContourPoint( PtrCst pOriginal, Feature_Contour::Ptr pParent, const std::string& strName )
    :   Feature( pOriginal, pParent, strName ),
        m_point( *this, 0 ),
        m_pContour( pParent ),
        m_uiContourPointIndex( pOriginal->m_uiContourPointIndex ),
        m_fRatio( pOriginal->m_fRatio )
{

}

Node::Ptr Feature_ContourPoint::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    Feature_Contour::Ptr pFeatureContourParent = boost::dynamic_pointer_cast< Feature_Contour >( pParent );
    VERIFY_RTE( pFeatureContourParent );
    return Node::copy< Feature_ContourPoint >( 
        boost::dynamic_pointer_cast< const Feature_ContourPoint >( shared_from_this() ), pFeatureContourParent, strName );
}

void Feature_ContourPoint::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );
    
    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_uiContourPointIndex >> m_fRatio;
    }
}

void Feature_ContourPoint::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );

    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
     os << m_uiContourPointIndex << m_fRatio;
}

std::string Feature_ContourPoint::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_uiContourPointIndex << m_fRatio;
        }
        os << sh;
    }
    return os.str();
}

const GlyphSpec* Feature_ContourPoint::getParent( int id ) const 
{ 
    if( Feature_Contour::Ptr pParent = boost::dynamic_pointer_cast< Feature_Contour >( m_pParent.lock() ) )
    {
        const GlyphSpec* pParentRoot = pParent->getRootControlPoint();
        ASSERT( pParentRoot );
        return pParentRoot;
    }
    else
    {
        ASSERT( false );
        return 0u; 
    }
}

Float Feature_ContourPoint::getX( int id ) const 
{ 
    THROW_RTE( "TODO" );
    Feature_Contour::PtrCst pParent = m_pContour.lock();
    ASSERT( pParent );
    Float fX = 0.0f;
    const Polygon& contour = pParent->getPolygon();
    if( contour.size() )
    {
        unsigned int uiIndex        = m_uiContourPointIndex % contour.size();
        unsigned int uiIndexNext    = ( m_uiContourPointIndex + 1u ) % contour.size();
        fX = contour[ uiIndex ].x + ( contour[ uiIndexNext ].x - contour[ uiIndex ].x ) * m_fRatio;
        fX -= contour[0u].x;
    }

    return fX; 
}
Float Feature_ContourPoint::getY( int id ) const 
{ 
    THROW_RTE( "TODO" );
    Feature_Contour::PtrCst pParent = m_pContour.lock();
    ASSERT( pParent );
    Float fY = 0.0f;
    const Polygon& contour = pParent->getPolygon();
    if( contour.size() )
    {
        unsigned int uiIndex        = m_uiContourPointIndex % contour.size();
        unsigned int uiIndexNext    = ( m_uiContourPointIndex + 1u ) % contour.size();
        fY = contour[ uiIndex ].y + ( contour[ uiIndexNext ].y - contour[ uiIndex ].y ) * m_fRatio;
        fY -= contour[0u].y;
    }

    return fY; 
}
void Feature_ContourPoint::set( int id, Float fX, Float fY )
{
    THROW_RTE( "TODO" );
    
    Feature_Contour::PtrCst pParent = m_pContour.lock();
    ASSERT( pParent );
    const Point& origin = pParent->getPolygon()[0];

    const Point ptInteractPos( fX + origin.x, fY + origin.y );
    SegmentIDRatioPointTuple result = 
        findClosestPointOnContour( pParent->getPolygon(), ptInteractPos );
    if( m_fRatio != std::get< 1 >( result ) ||  m_uiContourPointIndex != std::get< 0 >( result ) )
    {
        m_fRatio = std::get< 1 >( result );
        m_uiContourPointIndex = std::get< 0 >( result );
        setModified();
    }
}*/

}