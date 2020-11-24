#ifndef BASIC_AREA_14_09_2013
#define BASIC_AREA_14_09_2013

#include "spaceUtils.h"
#include "basicFeature.h"

#include "blueprint/buffer.h"
#include "blueprint/site.h"
#include "blueprint/connection.h"

#include "wykobi.hpp"

#include "common/angle.hpp"

#include <boost/optional.hpp>

#include <string>
#include <cmath>
#include <memory>

namespace Blueprint
{

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Area : public Site, public boost::enable_shared_from_this< Area >
{
    friend class PerimeterWidth_Interaction;
    friend class Brush_Interaction;
    friend class Boundary_Interaction;
    friend class Polygon_Interaction;
    
public:
    
public:
    typedef ControlPointCallback< Area > PointType;
    typedef std::vector< Feature_ContourSegment::Ptr > ContourPointVector;
    typedef std::vector< Property::Ptr > PropertyVector;

    typedef boost::shared_ptr< Area > Ptr;
    typedef boost::shared_ptr< const Area > PtrCst;

    static const std::string& TypeName();
    Area( Site::Ptr pParent, const std::string& strName );
    Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    ~Area();
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const;
    virtual EvaluationResult evaluate( const EvaluationMode& mode, DataBitmap& data );
    
    void init( float x, float y, bool bEmptyContour );
    virtual void init();
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;

    Feature_Contour::Ptr getContour() const { return m_pContour; }
    const ConnectionAnalysis& getConnections() const { return m_connections; }
    
    //GlyphSpec
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return m_pSiteParent.lock().get(); }

    //Origin
    virtual const Transform& getTransform() const { return m_transform; }
    virtual void setTransform( const Transform& transform );
    virtual const MarkupPath* getPolygon()  const { return m_pPath.get(); }
    
    virtual bool canEdit()                  const { return true; }

    virtual void set( float fX, float fY )
    {
        const float fNewValueX = Map_FloorAverage()( fX );
        const float fNewValueY = Map_FloorAverage()( fY );
        if( ( m_transform.X() != fNewValueX ) || ( m_transform.Y() != fNewValueY ) )
        {
            m_transform.setTranslation( fNewValueX, fNewValueY );
            setModified();
        }
    }

    //GlyphSpecProducer
    virtual void getMarkupTexts( MarkupText::List& text) 
    { 
        if( m_pLabel.get() ) 
            text.push_back( m_pLabel.get() ); 
    }
    
    virtual void getMarkupPaths( MarkupPath::List& paths ) 
    { 
        if( m_pPath.get() ) 
            paths.push_back( m_pPath.get() ); 
        
    }
    
    virtual void getMarkupPolygonGroups( MarkupPolygonGroup::List& polyGroups )
    {
        if( m_pPolygonGroup.get() )
            polyGroups.push_back( m_pPolygonGroup.get() );
    }
    
    //cmds
    void cmd_rotateLeft();
    void cmd_rotateRight();
    void cmd_flipHorizontally();
    void cmd_flipVertically();

    const ContourPointVector& getBoundaries() const { return m_boundaryPoints; }
    const PropertyVector& getProperties() const { return m_properties; }
private:
    ConnectionAnalysis m_connections;
    
    Site::WeakPtr m_pSiteParent;
    Transform m_transform;
    MarkupPath::PathCmdVector m_path;

    using MarkupGroupImpl = MarkupPolygonGroupImpl< ConnectionAnalysis::ConnectionPair >;
    
    std::unique_ptr< TextImpl > m_pLabel;
    std::unique_ptr< PathImpl > m_pPath;
    std::unique_ptr< MarkupGroupImpl > m_pPolygonGroup;
    Feature_Contour::Ptr m_pContour;
    
    MarkupGroupImpl::PolyMap m_polygonMap;
    ContourPointVector m_boundaryPoints;
    PropertyVector m_properties;
    std::string m_strLabelText;
    
    boost::optional< wykobi::polygon< float, 2u > > m_polygonCache;
};

}

#endif //BASIC_AREA_14_09_2013