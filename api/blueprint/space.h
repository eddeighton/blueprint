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
    
    //Site
    virtual void evaluate( const EvaluationMode& mode, EvaluationResults& results );
    
    //GlyphSpecProducer
    virtual void getMarkupPolygonGroups( MarkupPolygonGroup::List& polyGroups )
    {
        if( m_pExteriorPolygons.get() )
            polyGroups.push_back( m_pExteriorPolygons.get() );
        if( m_pInteriorContourPathImpl.get() )
            polyGroups.push_back( m_pInteriorContourPathImpl.get() );
    }
    
    //accessors
    const Polygon& getInteriorPolygon() const { return m_interiorPolygon; }
    const Polygon& getExteriorPolygon() const { return m_exteriorPolygon; }
    
    using ExteriorGroupImpl = MarkupPolygonGroupImpl< int >;
    const ExteriorGroupImpl::PolyMap& getInnerAreaExteriorPolygons() const { return m_exteriorPolyMap; }
    
    virtual Feature_Contour::Ptr getContour() const { return m_pContour; }
protected:
    Feature_Contour::Ptr m_pContour;
    Polygon m_exteriorPolygon, m_interiorPolygon;
    ExteriorGroupImpl::PolyMap m_exteriorPolyMap;
    std::unique_ptr< SimplePolygonMarkup > m_pInteriorContourPathImpl;
    std::unique_ptr< ExteriorGroupImpl > m_pExteriorPolygons;
    
    std::vector< Polygon > m_innerExteriors;
};

}

#endif //SPACE_01_DEC_2020