#ifndef SPACE_01_DEC_2020
#define SPACE_01_DEC_2020

#include "blueprint/site.h"

namespace Blueprint
{

class Space : public Site, public boost::enable_shared_from_this< Space >
{
public:
    typedef boost::shared_ptr< Space > Ptr;
    typedef boost::shared_ptr< const Space > PtrCst;
    
    static const std::string& TypeName();
    Space( Site::Ptr pParent, const std::string& strName );
    Space( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    void init();
    void init( float x, float y );
    
};

}

#endif //SPACE_01_DEC_2020