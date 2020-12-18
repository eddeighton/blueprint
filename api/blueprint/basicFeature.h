#ifndef FEATURE_18_09_2013
#define FEATURE_18_09_2013

#include "blueprint/cgalSettings.h"
#include "blueprint/property.h"
#include "blueprint/glyphSpec.h"
#include "blueprint/glyphSpecProducer.h"
#include "blueprint/markup.h"

#include "ed/node.hpp"

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
    Float getX( int ) const { return CGAL::to_double( m_ptOrigin.x() ); }
    Float getY( int ) const { return CGAL::to_double( m_ptOrigin.y() ); }
    void set( int, Float fX, Float fY )
    {
        const Point ptNew( Map_FloorAverage()( fX ), Map_FloorAverage()( fY ) );
        if( m_ptOrigin != ptNew )
        {
            m_ptOrigin = ptNew;
            setModified();
        }
    }
    const std::string& getName( int ) const { return Node::getName(); }
    
    virtual int getControlPointCount() { return 1; }
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        controlPoints.push_back( &m_point );
    }

    //const Point& get() const { return m_ptOrigin; }

    Point m_ptOrigin;
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

    const Polygon& getPolygon() const { return m_polygon; }
    const std::string& getName( int ) const { return Node::getName(); }
    
    //ControlPointCallback
    const GlyphSpec* getParent( int id ) const;
    Float getX( int id ) const;
    Float getY( int id ) const;
    void set( int id, Float fX, Float fY );
    void setSinglePoint( Float x, Float y );
    void set( const Polygon& shape );
    
    /*
    template< class T >
    void set( const T& shape )
    {
        set( wykobi::make_polygon( shape ) );
    }*/
    
    void recalculateControlPoints();
    
    virtual int getControlPointCount() 
    { 
        return static_cast< int >( m_points.size() );
    }
    
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        std::copy( m_points.begin(), m_points.end(), std::back_inserter( controlPoints ) );
    }

    const PointType* getRootControlPoint() const { return m_points.empty() ? nullptr : m_points[ 0u ]; }

    virtual bool cmd_delete( const std::vector< const GlyphSpec* >& selection );
    

private:
    Polygon m_polygon;
    PointVector m_points;
    Property::Ptr m_pAutoCalc;
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/*
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
    Float getRatio() const { return m_fRatio; }
    
    //ControlPointCallback
    virtual const GlyphSpec* getParent( int id ) const;
    virtual Float getX( int id ) const;
    virtual Float getY( int id ) const;
    virtual void set( int id, Float fX, Float fY );
    const std::string& getName( int ) const { return Node::getName(); }
    
    virtual int getControlPointCount() { return 1; }
    virtual void getControlPoints( ControlPoint::List& controlPoints )
    {
        controlPoints.push_back( &m_point );
    }

protected:
    unsigned int m_uiContourPointIndex;
    Float m_fRatio;

    ControlPointCallback< Feature_ContourPoint > m_point;
    Feature_Contour::WeakPtrCst m_pContour;
};
*/

}

#endif //FEATURE_18_09_2013