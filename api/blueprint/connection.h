#ifndef CONNECTION_07_NOV_2020
#define CONNECTION_07_NOV_2020

#include "blueprint/site.h"

namespace Blueprint
{
    
class Connection : public Site, public boost::enable_shared_from_this< Connection >
{
public:
    typedef boost::shared_ptr< Connection > Ptr;
    typedef boost::shared_ptr< const Connection > PtrCst;
    
    static const std::string& TypeName();
    Connection( Site::Ptr pParent, const std::string& strName );
    Connection( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    void init();
    void init( float x, float y );
    
};

}

#endif //CONNECTION_07_NOV_2020