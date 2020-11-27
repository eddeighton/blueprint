#ifndef SITE_13_09_2013
#define SITE_13_09_2013

#include "blueprint/buffer.h"
#include "blueprint/glyphSpec.h"
#include "blueprint/glyphSpecProducer.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <list>
#include <map>
#include <string>
#include <vector>

namespace Blueprint
{
    
class DataBitmap;

class Factory;
class Site;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Site : 
    public GlyphSpecProducer, 
    public Origin
{
    Site& operator=( const Site& );
public:
    typedef boost::shared_ptr< Site > Ptr;
    typedef boost::shared_ptr< const Site > PtrCst;
    typedef boost::weak_ptr< Site > WeakPtr;
    typedef std::set< Ptr > PtrSet;
    typedef std::list< Ptr > PtrList;
    typedef std::vector< Ptr > PtrVector;
    typedef std::list< WeakPtr > WeakPtrList;
    typedef std::map< Ptr, Ptr > PtrMap;
    
    Site( Node::Ptr pParent, const std::string& strName );
    Site( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    virtual void init();
    virtual bool add( Node::Ptr pNewNode );
    virtual void remove( Node::Ptr pNode );

    virtual bool canEvaluate( const PtrVector& evaluated ) const=0;
    struct EvaluationResult
    {
        EvaluationResult() : bSuccess( false ) {}
        bool bSuccess;
    };
    struct EvaluationMode
    {
        bool bBitmap = false;
        bool bCellComplex = false;
        bool bClearance = false;
    };
    virtual EvaluationResult evaluate( const EvaluationMode& mode, DataBitmap& data )=0;

    //spaces
    const Site::PtrVector& getSpaces() const { return m_spaces; }

    //generator
    Matrix getAbsoluteTransform() const;
    
    virtual bool isConnection() { return false; }

protected:
    Site::PtrVector m_spaces;
};

}

#endif //BLUEPRINT_13_09_2013
