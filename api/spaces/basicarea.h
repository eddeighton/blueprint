#ifndef BASIC_AREA_14_09_2013
#define BASIC_AREA_14_09_2013

#include "spaceUtils.h"
#include "basicFeature.h"

#include "blueprint/buffer.h"
#include "blueprint/site.h"

#include "wykobi.hpp"

#include <boost/optional.hpp>

#include <string>
#include <cmath>
#include <memory>

namespace Blueprint
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Clip : public Site, public boost::enable_shared_from_this< Clip >
{
public:
    typedef boost::shared_ptr< Clip > Ptr;
    typedef boost::shared_ptr< const Clip > PtrCst;

    static const std::string& TypeName();
    Clip( Node::Ptr pParent, const std::string& strName );
    Clip( PtrCst pOriginal, Node::Ptr pParent, const std::string& strName );
    ~Clip(){};
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    
    virtual void init() { Site::init(); }
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const { return Node::copy( shared_from_this(), pParent, strName ); }
    virtual void load( Factory& factory, const Ed::Node& node ) { return Node::load( shared_from_this(), factory, node ); }
    virtual void save( Ed::Node& node ) const { return Node::save( node ); }
    virtual std::string getStatement() const { return getName(); }
    
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const { ASSERT( false ); return true; }
    virtual EvaluationResult evaluate( DataBitmap& data ) 
    { 
        EvaluationResult result; 
        return result; 
    }
    
    //GlyphSpec
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return 0u; }

    //ImageSpec
    virtual float getX()                    const { return 0.0f; }
    virtual float getY()                    const { return 0.0f; }
    virtual float getOffsetX()              const { return 0.0f; }
    virtual float getOffsetY()              const { return 0.0f; }
    virtual NavBitmap::Ptr getBuffer()      const { return NavBitmap::Ptr(); }
    virtual void set( float fX, float fY ){}
    virtual bool canEdit() const { return false; }
    
    virtual bool canEditWithTool( const GlyphSpecProducer* pGlyphPrd, unsigned int uiToolType ) const { return false; }
    virtual void getCmds( CmdInfo::List& cmds ) const{};
    virtual void getTools( ToolInfo::List& tools ) const{};
    virtual IInteraction::Ptr beginToolDraw( unsigned int uiTool, float x, float y, float qX, float qY, boost::shared_ptr< Site > pClip ){ return IInteraction::Ptr(); }
    virtual IInteraction::Ptr beginTool( unsigned int uiTool, float x, float y, float qX, float qY, 
        GlyphSpecProducer* pHit, const std::set< GlyphSpecProducer* >& selection ){ return IInteraction::Ptr(); }

    
    virtual void getContour( FloatPairVector& contour ) { ASSERT( false ); }
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class Area : public Site, public boost::enable_shared_from_this< Area >
{
    friend class PerimeterWidth_Interaction;
    friend class Brush_Interaction;
    friend class Boundary_Interaction;
    friend class Polygon_Interaction;

public:
    typedef ControlPointCallback< Area > PointType;
    typedef std::vector< Feature_ContourSegment::Ptr > ContourPointVector;

    typedef boost::shared_ptr< Area > Ptr;
    typedef boost::shared_ptr< const Area > PtrCst;

    static const std::string& TypeName();
    Area( Site::Ptr pParent, const std::string& strName );
    Area( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    ~Area();
    virtual Node::PtrCst getPtr() const { return shared_from_this(); }
    virtual Node::Ptr getPtr() { return shared_from_this(); }
    
    virtual bool canEvaluate( const Site::PtrVector& evaluated ) const;
    virtual EvaluationResult evaluate( DataBitmap& data );
    
    void init( float x, float y );
    virtual void init();
    virtual Node::Ptr copy( Node::Ptr pParent, const std::string& strName ) const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual std::string getStatement() const;

    Feature_Contour::Ptr getContour() const { return m_pContour; }
    
    //GlyphSpec
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return m_pSiteParent.lock().get(); }

    //ImageSpec
    virtual float getX()                    const { return m_ptOrigin.x; }
    virtual float getY()                    const { return m_ptOrigin.y; }
    virtual float getOffsetX()              const { return m_ptOffset.x; }
    virtual float getOffsetY()              const { return m_ptOffset.y; }
    virtual NavBitmap::Ptr getBuffer()      const { return m_pBuffer; }
    virtual bool canEdit()                  const { return true; }

    virtual void set( float fX, float fY )
    {
        const float fNewValueX = Map_FloorAverage()( fX );
        const float fNewValueY = Map_FloorAverage()( fY );
        if( m_ptOrigin.x != fNewValueX || m_ptOrigin.y != fNewValueY )
        {
            m_ptOrigin.x = fNewValueX;
            m_ptOrigin.y = fNewValueY;
            setModified();
        }
    }

    //GlyphSpecProducer
    virtual void getMarkupTexts( MarkupText::List& text) { if( m_pLabel.get() ) text.push_back( m_pLabel.get() ); }
    virtual void getMarkupPaths( MarkupPath::List& paths ) { if( m_pPath.get() ) paths.push_back( m_pPath.get() ); }
    
    enum eCmds
    {
        eSomeCmd,
        TOTAL_CMDS
    };
    enum eTools
    {
        ePerimeterWidth,
        eBrush,
        eBoundary,
        ePoly,
        TOTAL_TOOLS
    };
    
    virtual bool canEditWithTool( const GlyphSpecProducer* pGlyphPrd, unsigned int uiToolType ) const;
    virtual void getCmds( CmdInfo::List& cmds ) const;
    virtual void getTools( ToolInfo::List& tools ) const;

    virtual IInteraction::Ptr beginToolDraw( unsigned int uiTool, float x, float y, float qX, float qY, Site::Ptr pClip );
    virtual IInteraction::Ptr beginTool( unsigned int uiTool, float x, float y, float qX, float qY, 
        GlyphSpecProducer* pHit, const std::set< GlyphSpecProducer* >& selection );

    const ContourPointVector& getBoundaries() const { return m_boundaryPoints; }
    
    virtual void getContour( FloatPairVector& contour );
private:
    Site::WeakPtr m_pSiteParent;
    wykobi::point2d< float > m_ptOrigin;
    wykobi::point2d< float > m_ptOffset;
    float m_fPerimeterWidth;
    MarkupPath::PathCmdVector m_path;

    NavBitmap::Ptr m_pBuffer;
    std::unique_ptr< TextImpl > m_pLabel;
    std::unique_ptr< PathImpl > m_pPath;
    Feature_Contour::Ptr m_pContour;

    ContourPointVector m_boundaryPoints;

    boost::optional< float > m_fPerimeterWidthCache;
    boost::optional< wykobi::polygon< float, 2u > > m_polygonCache;
};

}

#endif //BASIC_AREA_14_09_2013