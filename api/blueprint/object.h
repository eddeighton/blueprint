#ifndef OBJECT_01_DEC_2020
#define OBJECT_01_DEC_2020

#include "blueprint/site.h"

namespace Blueprint
{

class Object : public Site, public boost::enable_shared_from_this< Object >
{
public:
    typedef boost::shared_ptr< Object > Ptr;
    typedef boost::shared_ptr< const Object > PtrCst;
    
    static const std::string& TypeName();
    Object( Site::Ptr pParent, const std::string& strName );
    Object( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    void init();
    void init( Float x, Float y );
    
    //Site
    virtual void evaluate( const EvaluationMode& mode, EvaluationResults& results );
    
    virtual Feature_Contour::Ptr getContour() const { return m_pContour; }
private:
    Feature_Contour::Ptr m_pContour;
};

}

#endif //OBJECT_01_DEC_2020