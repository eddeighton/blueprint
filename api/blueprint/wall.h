#ifndef WALL_01_DEC_2020
#define WALL_01_DEC_2020

#include "blueprint/site.h"

namespace Blueprint
{

class Wall : public Site, public boost::enable_shared_from_this< Wall >
{
public:
    typedef boost::shared_ptr< Wall > Ptr;
    typedef boost::shared_ptr< const Wall > PtrCst;
    
    static const std::string& TypeName();
    Wall( Site::Ptr pParent, const std::string& strName );
    Wall( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;
    void init();
    void init( float x, float y );
    
    //Site
    virtual void evaluate( const EvaluationMode& mode, EvaluationResults& results );
    
    virtual Feature_Contour::Ptr getContour() const { return m_pContour; }
private:
    Feature_Contour::Ptr m_pContour;
    
};

}

#endif //WALL_01_DEC_2020