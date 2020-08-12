#ifndef BLUEPRINT_25_09_2013
#define BLUEPRINT_25_09_2013

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

    //ImageSpec
    virtual float getX()                    const { return m_ptOrigin.x; }
    virtual float getY()                    const { return m_ptOrigin.y; }
    virtual float getOffsetX()              const { return m_ptOffset.x; }
    virtual float getOffsetY()              const { return m_ptOffset.y; }
    virtual NavBitmap::Ptr getBuffer()      const { return m_pBuffer; }
    virtual void set( float fX, float fY ) {  }
    virtual bool canEdit()                  const { return false; }

    //CmdTarget
    virtual bool canEditWithTool( const GlyphSpecProducer* pGlyphPrd, unsigned int uiToolType ) const { return false; }
    virtual void getCmds( CmdInfo::List& cmds ) const{};
    virtual void getTools( ToolInfo::List& tools ) const{};
    virtual IInteraction::Ptr beginToolDraw( unsigned int uiTool, float x, float y, float qX, float qY, boost::shared_ptr< Site > pClip ){ return IInteraction::Ptr(); }
    virtual IInteraction::Ptr beginTool( unsigned int uiTool, float x, float y, float qX, float qY, 
        GlyphSpecProducer* pHit, const std::set< GlyphSpecProducer* >& selection ){ return IInteraction::Ptr(); }

    //Site
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const { return true; }
    virtual EvaluationResult evaluate( DataBitmap& data );
    
    virtual void getContour( FloatPairVector& contour );
    virtual bool isConnection() { return true; }

private:
    Reference::Ptr m_pSourceRef, m_pTargetRef;
    
    Site::WeakPtr m_pSiteParent;
    NavBitmap::Ptr m_pBuffer;
    wykobi::point2d< float > m_ptOrigin;
    wykobi::point2d< float > m_ptOffset;
    boost::optional< wykobi::polygon< float, 2u > > m_polygonCache;
};

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

    //ImageSpec
    virtual float getX()                    const { return 0.0f; }
    virtual float getY()                    const { return 0.0f; }
    virtual float getOffsetX()              const { return 0.0f; }
    virtual float getOffsetY()              const { return 0.0f; }
    virtual NavBitmap::Ptr getBuffer()      const { return NavBitmap::Ptr(); }
    virtual bool canEdit()                  const { return false; }
    virtual void set( float fX, float fY ) {}

    //CmdTarget
    virtual bool canEditWithTool( const GlyphSpecProducer* pGlyphPrd, unsigned int uiToolType ) const;
    virtual void getCmds( CmdInfo::List& cmds ) const;
    virtual void getTools( ToolInfo::List& tools ) const;    
    virtual IInteraction::Ptr beginToolDraw( unsigned int uiTool, float x, float y, float qX, float qY, boost::shared_ptr< Site > pClip );
    virtual IInteraction::Ptr beginTool( unsigned int uiTool, float x, float y, float qX, float qY, 
        GlyphSpecProducer* pHit, const std::set< GlyphSpecProducer* >& selection );

    //Site
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const { return true; }
    virtual EvaluationResult evaluate( DataBitmap& data );
    
    virtual bool add( Node::Ptr pNewNode );
    virtual void remove( Node::Ptr pNode );
    
    virtual void getContour( FloatPairVector& contour ) { ASSERT( false ); }

    void getContour( std::vector< FloatPairVector >& contour );

private:
    void computePairings( ConnectionPairingSet& pairings ) const;
private:
    ConnectionMap m_connections;
};



}

#endif //BLUEPRINT_25_09_2013