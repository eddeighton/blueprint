#ifndef CONNECTION_07_NOV_2020
#define CONNECTION_07_NOV_2020

#include "basicFeature.h"
#include "basicarea.h"

#include "blueprint/buffer.h"
#include "blueprint/property.h"
#include "blueprint/site.h"

#include "ed/node.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <list>
#include <map>
#include <string>
#include <vector>

namespace Blueprint
{

wykobi::point2d< float > getContourSegmentPointAbsolute( Feature_ContourSegment::Ptr pPoint, Feature_ContourSegment::Points pointType );

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Connection : public Site, public boost::enable_shared_from_this< Connection >
{
    Connection& operator=( const Connection& );
public:
    typedef boost::shared_ptr< Connection > Ptr;
    typedef boost::shared_ptr< const Connection > PtrCst;
    typedef boost::weak_ptr< Connection > WeakPtr;
    typedef std::list< Ptr > PtrList;
    typedef std::vector< Ptr > PtrVector;
    typedef std::list< WeakPtr > WeakPtrList;
    typedef std::map< Ptr, Ptr > PtrMap;
    
    static const std::string& TypeName();
    Connection( Site::Ptr pParent, const std::string& strName );
    Connection( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void init();
    void init( const Ed::Reference& sourceRef, const Ed::Reference& targetRef );
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const { return ""; }

    Reference::Ptr getSource() { return m_pSourceRef; }
    Reference::Ptr getTarget() { return m_pTargetRef; }
    
    //GlyphSpec
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return m_pSiteParent.lock().get(); }

    //Origin
    virtual const Transform& getTransform() const { return m_transform; }
    virtual void setTransform( const Transform& transform ) 
    { 
        ASSERT( false );
    }
    virtual const MarkupPath* getPolygon()  const { return m_pPath.get(); }
	
    virtual void set( float fX, float fY ) {  }
    virtual bool canEdit()                  const { return false; }

    //Site
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const { return true; }
    virtual EvaluationResult evaluate( const EvaluationMode& mode, DataBitmap& data );
	
    virtual bool isConnection() { return true; }

private:
    Reference::Ptr m_pSourceRef, m_pTargetRef;
    MarkupPath::PathCmdVector m_path;
    std::unique_ptr< PathImpl > m_pPath;
    
    Site::WeakPtr m_pSiteParent;
    Transform m_transform;
    boost::optional< wykobi::polygon< float, 2u > > m_polygonCache;
};

}

#endif //CONNECTION_07_NOV_2020