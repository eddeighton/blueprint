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
        m_ptOrigin( wykobi::make_point( 0.0f, 0.0f ) )
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

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
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

float Feature_ContourPoint::getX( int id ) const 
{ 
    Feature_Contour::PtrCst pParent = m_pContour.lock();
    ASSERT( pParent );
    float fX = 0.0f;
    const wykobi::polygon< float, 2 >& contour = pParent->getPolygon();
    if( contour.size() )
    {
        unsigned int uiIndex        = m_uiContourPointIndex % contour.size();
        unsigned int uiIndexNext    = ( m_uiContourPointIndex + 1u ) % contour.size();
        fX = contour[ uiIndex ].x + ( contour[ uiIndexNext ].x - contour[ uiIndex ].x ) * m_fRatio;
        fX -= contour[0u].x;
    }

    return fX; 
}
float Feature_ContourPoint::getY( int id ) const 
{ 
    Feature_Contour::PtrCst pParent = m_pContour.lock();
    ASSERT( pParent );
    float fY = 0.0f;
    const wykobi::polygon< float, 2 >& contour = pParent->getPolygon();
    if( contour.size() )
    {
        unsigned int uiIndex        = m_uiContourPointIndex % contour.size();
        unsigned int uiIndexNext    = ( m_uiContourPointIndex + 1u ) % contour.size();
        fY = contour[ uiIndex ].y + ( contour[ uiIndexNext ].y - contour[ uiIndex ].y ) * m_fRatio;
        fY -= contour[0u].y;
    }

    return fY; 
}
void Feature_ContourPoint::set( int id, float fX, float fY )
{
    Feature_Contour::PtrCst pParent = m_pContour.lock();
    ASSERT( pParent );
    const wykobi::point2d< float >& origin = pParent->getPolygon()[0];

    const wykobi::point2d< float > ptInteractPos = 
        wykobi::make_point( fX + origin.x, fY + origin.y );
    SegmentIDRatioPointTuple result = 
        findClosestPointOnContour( pParent->getPolygon(), ptInteractPos );
    if( m_fRatio != std::get< 1 >( result ) ||  m_uiContourPointIndex != std::get< 0 >( result ) )
    {
        m_fRatio = std::get< 1 >( result );
        m_uiContourPointIndex = std::get< 0 >( result );
        setModified();
    }
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
const std::string& Feature_ContourSegment::TypeName()
{
    static const std::string strTypeName( "contoursegment" );
    return strTypeName;
}
Feature_ContourSegment::Feature_ContourSegment( Feature_Contour::Ptr pParent, const std::string& strName )
    :   Feature_ContourPoint( pParent, strName ),
        m_fWidth( 4.0f ),
        m_pointLeft( *this, 1 ),
        m_pointRight( *this, 2 )
{
}
Feature_ContourSegment::Feature_ContourSegment( PtrCst pOriginal, Feature_Contour::Ptr pParent, const std::string& strName )
    :   Feature_ContourPoint( pOriginal, pParent, strName ),
        m_fWidth( pOriginal->m_fWidth ),
        m_pointLeft( *this, 1 ),
        m_pointRight( *this, 2 )
{
}

void Feature_ContourSegment::init()
{
    Feature::init();
    
    m_pSegmentType = get< Property >( "type" );
    if( !m_pSegmentType )
    {
        m_pSegmentType = Property::Ptr( new Property( shared_from_this(), "type" ) );
        m_pSegmentType->init();
        m_pSegmentType->setStatement( "none;" );
        add( m_pSegmentType );
    }
}

Node::Ptr Feature_ContourSegment::copy( Node::Ptr pParent, const std::string& strName ) const
{   
    Feature_Contour::Ptr pFeatureContourParent = boost::dynamic_pointer_cast< Feature_Contour >( pParent );
    VERIFY_RTE( pFeatureContourParent );
    return Node::copy< Feature_ContourSegment >( 
        boost::dynamic_pointer_cast< const Feature_ContourSegment >( shared_from_this() ), pFeatureContourParent, strName );
}

void Feature_ContourSegment::load( Factory& factory, const Ed::Node& node )
{
    Node::load( shared_from_this(), factory, node );

    if( boost::optional< const Ed::Shorthand& > shOpt = node.getShorty() )
    {
        Ed::IShorthandStream is( shOpt.get() );
        is >> m_uiContourPointIndex >> m_fRatio >> m_fWidth;
    }
}

void Feature_ContourSegment::save( Ed::Node& node ) const
{
    node.statement.addTag( Ed::Identifier( TypeName() ) );

    Node::save( node );
    
    if( !node.statement.shorthand ) node.statement.shorthand = Ed::Shorthand();
    Ed::OShorthandStream os( node.statement.shorthand.get() );
    os << m_uiContourPointIndex << m_fRatio << m_fWidth;
}

std::string Feature_ContourSegment::getStatement() const
{
    std::ostringstream os;
    {
        Ed::Shorthand sh;
        {
            Ed::OShorthandStream ossh( sh );
            ossh << m_uiContourPointIndex << m_fRatio << m_fWidth;
        }
        os << sh;
    }
    return os.str();
}

std::string Feature_ContourSegment::getSegmentType() const
{
    if( m_pSegmentType )
    {
        return m_pSegmentType->getValue();
    }
    return "none";
}

bool Feature_ContourSegment::isSegmentExterior() const
{
    return getSegmentType() == "exterior;";
}

void Feature_ContourSegment::getBoundaryPoint( Points type, unsigned int& polyIndex, float& x, float& y, float& fDistance ) const
{
    x = getX( type );
    y = getY( type );
    if( type == eMidPoint ) 
    {
        polyIndex = m_uiContourPointIndex;
    }
    else if( type == eLeft )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );   
        ASSERT( pParent->getPolygon().size() > 0u );     
        polyIndex = calculateIndexOnPolygon( pParent->getPolygon(), m_uiContourPointIndex, m_fRatio, m_fWidth, fDistance );
    }
    else if( type == eRight )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );   
        ASSERT( pParent->getPolygon().size() > 0u );     
        polyIndex = calculateIndexOnPolygon( pParent->getPolygon(), m_uiContourPointIndex, m_fRatio, -m_fWidth, fDistance );
    }
    else
    {
        ASSERT( false );
    }
}

float Feature_ContourSegment::getX( int id ) const 
{ 
    if( id == eMidPoint ) 
    {
        return Feature_ContourPoint::getX( 0 );
    }
    else if( id == eLeft )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );   
        ASSERT( pParent->getPolygon().size() > 0u );     
        return calculatePointOnPolygon( pParent->getPolygon(), m_uiContourPointIndex, m_fRatio, m_fWidth ).x - pParent->getPolygon()[0u].x; 
    }
    else if( id == eRight )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );   
        ASSERT( pParent->getPolygon().size() > 0u );     
        return calculatePointOnPolygon( pParent->getPolygon(), m_uiContourPointIndex, m_fRatio, -m_fWidth ).x - pParent->getPolygon()[0u].x; 
    }
    ASSERT( false );
    return 0.0f;
}

float Feature_ContourSegment::getY( int id ) const 
{ 
    if( id == eMidPoint ) 
    {
        return Feature_ContourPoint::getY( 0 );
    }
    else if( id == eLeft )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );        
        ASSERT( pParent->getPolygon().size() > 0u );
        return calculatePointOnPolygon( pParent->getPolygon(), m_uiContourPointIndex, m_fRatio, m_fWidth ).y - pParent->getPolygon()[0u].y; 
    }
    else if( id == 2 )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );     
        ASSERT( pParent->getPolygon().size() > 0u );   
        return calculatePointOnPolygon( pParent->getPolygon(), m_uiContourPointIndex, m_fRatio, -m_fWidth ).y - pParent->getPolygon()[0u].y; 
    }
    ASSERT( false );
    return 0.0f;
}

void Feature_ContourSegment::set( int id, float fX, float fY )
{
    if( id == eMidPoint ) 
    {
        Feature_ContourPoint::set( 0, fX, fY );
    }
    else if( id == eLeft )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );
        const wykobi::point2d< float >& origin = pParent->getPolygon()[0];
        const wykobi::point2d< float > ptInteractPos = 
            wykobi::make_point( fX + origin.x, fY + origin.y );
        SegmentIDRatioPointTuple result = findClosestPointOnContour( pParent->getPolygon(), ptInteractPos );
        const float fNewWidth = calculateDistanceAroundContour( pParent->getPolygon(),
            m_uiContourPointIndex, m_fRatio, std::get< 0 >( result ), std::get< 1 >( result ) );
        if( fNewWidth != m_fWidth )
        {
            m_fWidth = fNewWidth;
            setModified();
        }
    }
    else if( id == eRight )
    {
        Feature_Contour::PtrCst pParent = m_pContour.lock();
        ASSERT( pParent );
        const wykobi::point2d< float >& origin = pParent->getPolygon()[0];
        const wykobi::point2d< float > ptInteractPos = 
            wykobi::make_point( fX + origin.x, fY + origin.y );
        SegmentIDRatioPointTuple result = findClosestPointOnContour( pParent->getPolygon(), ptInteractPos );
        const float fNewWidth = calculateDistanceAroundContour( pParent->getPolygon(),
            std::get< 0 >( result ), std::get< 1 >( result ), m_uiContourPointIndex, m_fRatio );
        if( fNewWidth != m_fWidth )
        {
            m_fWidth = fNewWidth;
            setModified();
        }
    }
}
}