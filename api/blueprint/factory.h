#ifndef FACTORY_18_09_2013
#define FACTORY_18_09_2013

#include "site.h"

#include "ed/node.hpp"

namespace Blueprint
{

class Factory
{
    friend class Node;
public:
    Site::Ptr create( const std::string& strName );
    Site::Ptr load( const std::string& strFilePath );
    void load( const std::string& strFilePath, Node::PtrVector& results );
    void save( Site::Ptr pNode, const std::string& strFilePath );
private:
    Node::Ptr load( Node::Ptr pParent, const Ed::Node& node );

};

}

#endif