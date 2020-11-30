#ifndef FEATURE_18_09_2013
#define FEATURE_18_09_2013

#include "spaceUtils.h"

#include "blueprint/buffer.h"
#include "blueprint/property.h"
#include "blueprint/glyphSpec.h"
#include "blueprint/glyphSpecProducer.h"

#include "ed/node.hpp"

#include "wykobi.hpp"

#include <boost/shared_ptr.hpp>

#include <list>
#include <vector>
#include <map>
#include <set>
#include <string>

namespace Blueprint
{
class Factory;

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class Feature : public GlyphSpecProducer, public boost::enable_shared_from_this< Feature >
{
public:
    typedef boost::shared_ptr< Feature > Ptr;
    typedef boost::shared_ptr< const Feature > PtrCst;
    typedef std::set< Ptr > PtrSet;
    typedef std::vector< Ptr > PtrVector;
    typedef std::map< std::string, Ptr > PtrMap;
    
    static const std::string& TypeName();
    Feature( Node::Ptr pParent, const std::string& strName )
        : GlyphSpecProducer( pParent, strName )
    {

    }
    Feature( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
        :   GlyphSpecProducer( pOriginal, pParent, strName )
    {
    }
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void init();
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const { return ""; }
    
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class Feature_Point : public Feature
{
public:
    typedef boost::shared_ptr< Feature_Point > Ptr;
    typedef boost::shared_ptr< const Feature_Point > PtrCst;
    
    static const std::string& TypeName();
    Feature_Point( Node::Ptr pParent, const std::string& strName );
    Feature_Point( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    
    //ControlPointCallback
    const GlyphSpec* getParent( int id ) const;
    float getX( int id ) const { return m_ptOrigin.x; }
    float getY( int id ) const { return m_ptOrigin.y; }
    void set( int id, float fX, float fY )
    {
        const float fNewValueX = Map_FloorAverage()( fX );
        const float fNewValueY = Map_FloorAverage()( fY );
        if( m_ptOrigin.x != fNewValueX || m_ptOrigin.y != fNewValueY )
        {
            m_ptOrigin.x = fNewValueX;
            m_ptOrigin.y = fNewValueY;
            setModified();
        }
    }
    const std::string& getName(  int id ) const { return Node::getName(); }
    
    virtual int getControlPointCount() { return 1; }
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        controlPoints.push_back( &m_point );
    }

    const wykobi::point2d< float >& get() const { return m_ptOrigin; }

    wykobi::point2d< float > m_ptOrigin;
    ControlPointCallback< Feature_Point > m_point;
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class Feature_Contour : public Feature
{
    typedef ControlPointCallback< Feature_Contour > PointType;
    typedef std::vector< PointType* > PointVector;
public:
    typedef boost::shared_ptr< Feature_Contour > Ptr;
    typedef boost::weak_ptr< Feature_Contour > WeakPtr;
    typedef boost::weak_ptr< const Feature_Contour > WeakPtrCst;
    typedef boost::shared_ptr< const Feature_Contour > PtrCst;
    
    static const std::string& TypeName();
    Feature_Contour( Node::Ptr pParent, const std::string& strName );
    Feature_Contour( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    virtual ~Feature_Contour();
    virtual void init();
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    bool isAutoCalculate() const;

    const wykobi::polygon< float, 2 >& getPolygon() const { return m_polygon; }
    
    //ControlPointCallback
    const GlyphSpec* getParent( int id ) const;
    float getX( int id ) const 
    { 
        return m_polygon[ id ].x; 
    }
    float getY( int id ) const 
    { 
        return m_polygon[ id ].y; 
    }
    void set( int id, float fX, float fY ) 
    { 
        if( id >= 0 && id < m_polygon.size() )
        {
            const float fNewValueX = Map_FloorAverage()( fX );
            //const float fNewValueX = fX;
            if( m_polygon[ id ].x != fNewValueX )
            {
                m_polygon[ id ].x = fNewValueX;
                setModified();
            }
            const float fNewValueY = Map_FloorAverage()( fY );
            //const float fNewValueY = fY;
            if( m_polygon[ id ].y != fNewValueY )
            {
                m_polygon[ id ].y = fNewValueY;
                setModified();
            }
        }
    }
    const std::string& getName(  int id ) const { return Node::getName(); }
    
    void setSinglePoint( float x, float y )
    {
        m_polygon.clear();
        m_polygon.push_back( wykobi::make_point< float >( Map_FloorAverage()( x ), Map_FloorAverage()( y ) ) );
        recalculateControlPoints();
    }
    
    void set( const wykobi::polygon< float, 2 >& shape )
    {
        if( !( m_polygon.size() == shape.size() ) || 
            !std::equal( m_polygon.begin(), m_polygon.end(), shape.begin() ) )
        {
            m_polygon = shape;
            for( wykobi::polygon< float, 2 >::iterator i = m_polygon.begin(),
                iEnd = m_polygon.end(); i!=iEnd; ++i )
                *i = wykobi::make_point< float >( Map_FloorAverage()( i->x ), Map_FloorAverage()( i->y ) );
            recalculateControlPoints();
        }
    }
    
    template< class T >
    void set( const T& shape )
    {
        set( wykobi::make_polygon( shape ) );
    }
    
    void recalculateControlPoints();
    
    virtual int getControlPointCount() 
    { 
        return m_points.size(); 
    }
    
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        std::copy( m_points.begin(), m_points.end(), std::back_inserter( controlPoints ) );
    }

    const PointType* getRootControlPoint() const { return m_points.empty() ? nullptr : m_points[ 0u ]; }

    virtual bool cmd_delete( const std::vector< const GlyphSpec* >& selection );
    

private:
    wykobi::polygon< float, 2 > m_polygon;
    PointVector m_points;
    Property::Ptr m_pAutoCalc;
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class Feature_ContourPoint : public Feature
{
public:
    typedef boost::shared_ptr< Feature_ContourPoint > Ptr;
    typedef boost::shared_ptr< const Feature_ContourPoint > PtrCst;
    
    static const std::string& TypeName();
    Feature_ContourPoint( Feature_Contour::Ptr pParent, const std::string& strName );
    Feature_ContourPoint( PtrCst pOriginal, Feature_Contour::Ptr pParent, const std::string& strName );
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;

    const unsigned int getContourPointIndex() const { return m_uiContourPointIndex; }
    float getRatio() const { return m_fRatio; }
    
    //ControlPointCallback
    virtual const GlyphSpec* getParent( int id ) const;
    virtual float getX( int id ) const;
    virtual float getY( int id ) const;
    virtual void set( int id, float fX, float fY );
    const std::string& getName( int id ) const { return Node::getName(); }
    
    virtual int getControlPointCount() { return 1; }
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        controlPoints.push_back( &m_point );
    }

protected:
    unsigned int m_uiContourPointIndex;
    float m_fRatio;

    ControlPointCallback< Feature_ContourPoint > m_point;
    Feature_Contour::WeakPtrCst m_pContour;
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class Feature_ContourSegment : public Feature_ContourPoint
{
public:
    typedef boost::shared_ptr< Feature_ContourSegment > Ptr;
    typedef boost::weak_ptr< Feature_ContourSegment > PtrWeak;
    typedef boost::shared_ptr< const Feature_ContourSegment > PtrCst;
    
    static const std::string& TypeName();
    Feature_ContourSegment( Feature_Contour::Ptr pParent, const std::string& strName );
    Feature_ContourSegment( PtrCst pOriginal, Feature_Contour::Ptr pParent, const std::string& strName );
    virtual void init();
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    std::string getSegmentType() const;
    bool isSegmentExterior() const;
    
    enum Points
    {
        eMidPoint,
        eLeft,
        eRight,
        TOTAL_CONTROl_POINTS
    };

    void getBoundaryPoint( Points type, unsigned int& polyIndex, float& x, float& y, float& fDistance ) const;
    float getWidth() const { return m_fWidth; }

    virtual float getX( int id ) const;
    virtual float getY( int id ) const;
    virtual void set( int id, float fX, float fY );
    virtual int getControlPointCount() { return 3; }
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        Feature_ContourPoint::getControlPoints( controlPoints );
        controlPoints.push_back( &m_pointLeft );
        controlPoints.push_back( &m_pointRight );
    }
protected:
    float m_fWidth;
    ControlPointCallback< Feature_ContourSegment > m_pointLeft, m_pointRight;
    Property::Ptr m_pSegmentType;
};

}

#endif //FEATURE_18_09_2013