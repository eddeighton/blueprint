
#ifndef GLYPHSPEC_PRODUCER_07_NOV_2020
#define GLYPHSPEC_PRODUCER_07_NOV_2020

#include "glyphSpec.h"
#include "node.h"

#include "ed/ed.hpp"

#include "common/stl.hpp"
#include "common/compose.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/chrono.hpp>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

namespace Blueprint
{

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
class GlyphSpecProducer : public Node
{
public:
    typedef boost::shared_ptr< GlyphSpecProducer > Ptr;
    typedef boost::shared_ptr< const GlyphSpecProducer > PtrCst;

    GlyphSpecProducer( Node::Ptr pParent, const std::string& strName )
        : Node( pParent, strName )
    {
    }
    GlyphSpecProducer( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName )
        :   Node( pOriginal, pParent, strName )
    {
    }
    virtual int getControlPointCount() { return 0; }
    virtual void getControlPoints( ControlPoint::List& ) {}
    
    virtual void getMarkupPaths( MarkupPath::List& ) {}
    virtual void getMarkupPolygonGroups( MarkupPolygonGroup::List& ) {}
    virtual void getMarkupTexts( MarkupText::List& ) {}

    virtual bool cmd_delete( const GlyphSpec* ) { return false; }
};


}

#endif //GLYPHSPEC_PRODUCER_07_NOV_2020