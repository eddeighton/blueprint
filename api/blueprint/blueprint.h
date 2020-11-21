#ifndef BLUEPRINT_25_09_2013
#define BLUEPRINT_25_09_2013

#include "basicFeature.h"
#include "basicarea.h"
#include "connection.h"

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

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Blueprint : public Site, public boost::enable_shared_from_this< Blueprint >
{
    Blueprint& operator=( const Blueprint& );

    typedef std::pair< Area::Ptr, Feature_ContourSegment::Ptr > AreaBoundaryPair;

    struct ConnectionPairing
    {
        AreaBoundaryPair first, second;
        ConnectionPairing( const AreaBoundaryPair& _p1, const AreaBoundaryPair& _p2 )
            :   first( ( _p1 < _p2 ) ?_p1 : _p2 ), second( ( _p1 < _p2 ) ? _p2 : _p1 )
        {}
        ConnectionPairing( const ConnectionPairing& cpy )
            :   first( cpy.first ), second( cpy.second )
        {}
        bool operator<( const ConnectionPairing& cmp ) const
        {
            return ( first != cmp.first ) ? first < cmp.first : second < cmp.second;
        }
    };

    typedef std::vector< ConnectionPairing > ConnectionPairingVector;
    typedef std::set< ConnectionPairing > ConnectionPairingSet;
    typedef std::map< ConnectionPairing, Connection::Ptr > ConnectionMap;
    
    struct CompareConnectionIters
    {
        bool operator()( ConnectionMap::const_iterator i1, ConnectionPairingSet::const_iterator i2 ) const
        {
            return i1->first < (*i2);
        }
        bool opposite( ConnectionMap::const_iterator i1, ConnectionPairingSet::const_iterator i2 ) const
        {
            return (*i2) < i1->first;
        }
    };

public:
    typedef boost::shared_ptr< Blueprint > Ptr;
    typedef boost::shared_ptr< const Blueprint > PtrCst;
    typedef boost::weak_ptr< Blueprint > WeakPtr;
    typedef std::list< Ptr > PtrList;
    typedef std::vector< Ptr > PtrVector;
    typedef std::list< WeakPtr > WeakPtrList;
    typedef std::map< Ptr, Ptr > PtrMap;
    
    static const std::string& TypeName();
    Blueprint( const std::string& strName );
    Blueprint( PtrCst pOriginal, Node::Ptr pNotUsed, const std::string& strName );
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void init();
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const { return ""; }
    
    //GlyphSpec
    virtual const GlyphSpec* getParent() const { return 0u; }
    virtual const std::string& getName() const { return Node::getName(); }

    //Origin
    virtual const Transform& getTransform() const 
    { 
        static const Transform m_transform;
        return m_transform; 
    }
    virtual void setTransform( const Transform& transform ) 
    { 
        ASSERT( false );
    }
    virtual const MarkupPath* getPolygon()  const { return nullptr; }
    
    
    virtual bool canEdit()                  const { return false; }
    virtual void set( float fX, float fY ) {}

    //Site
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const { return true; }
    virtual EvaluationResult evaluate( const EvaluationMode& mode, DataBitmap& data );
    
    virtual bool add( Node::Ptr pNewNode );
    virtual void remove( Node::Ptr pNode );

private:
    void computePairings( ConnectionPairingSet& pairings ) const;
private:
    ConnectionMap m_connections;
};



}

#endif //BLUEPRINT_25_09_2013