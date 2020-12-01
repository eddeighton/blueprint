#ifndef SITE_13_09_2013
#define SITE_13_09_2013

#include "blueprint/geometry.h"
#include "blueprint/glyphSpec.h"
#include "blueprint/glyphSpecProducer.h"
#include "blueprint/markup.h"
#include "blueprint/basicFeature.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <list>
#include <map>
#include <string>
#include <vector>

namespace Blueprint
{
    
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
    
    Site( Site::Ptr pParent, const std::string& strName );
    Site( PtrCst pOriginal, Site::Ptr pParent, const std::string& strName );
    virtual std::string getStatement() const;
    virtual void load( Factory& factory, const Ed::Node& node );
    virtual void save( Ed::Node& node ) const;
    virtual void init();
    virtual bool add( Node::Ptr pNewNode );
    virtual void remove( Node::Ptr pNode );
    
    //evaluation
    struct EvaluationResults
    {
        std::vector< std::string > errors;
    };
    struct EvaluationMode
    {
        bool bBitmap        = false;
        bool bCellComplex   = false;
        bool bClearance     = false;
    };
    virtual void evaluate( const EvaluationMode& mode, EvaluationResults& results );

    //GlyphSpec
    virtual bool canEdit() const { return true; }
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return m_pSiteParent.lock().get(); }
    
    //origin transform
    Matrix getAbsoluteTransform() const;
    virtual void setTransform( const Transform& transform );
    virtual const MarkupPath* getMarkupContour() const
    {
        return m_pContourPathImpl.get();
    }
    Feature_Contour::Ptr getContour() const { return m_pContour; }
    virtual void set( float fX, float fY )
    {
        const float fNewValueX = Map_FloorAverage()( fX );
        const float fNewValueY = Map_FloorAverage()( fY );
        if( ( m_transform.X() != fNewValueX ) || ( m_transform.Y() != fNewValueY ) )
        {
            m_transform.setTranslation( fNewValueX, fNewValueY );
            setModified();
        }
    }
    
    //GlyphSpecProducer
    virtual void getMarkupTexts( MarkupText::List& text) 
    { 
        if( m_pLabel.get() ) 
            text.push_back( m_pLabel.get() ); 
    }
    virtual void getMarkupPaths( MarkupPath::List& paths ) 
    { 
        if( m_pContourPathImpl.get() ) 
            paths.push_back( m_pContourPathImpl.get() ); 
    }
    virtual void getMarkupPolygonGroups( MarkupPolygonGroup::List& polyGroups )
    {
        if( m_pExteriorPolygons.get() )
            polyGroups.push_back( m_pExteriorPolygons.get() );
    }
    
    //spaces
    const Site::PtrVector& getSpaces() const { return m_spaces; }
    
    //cmds
    void cmd_rotateLeft( const Rect2D& transformBounds );
    void cmd_rotateRight( const Rect2D& transformBounds );
    void cmd_flipHorizontally( const Rect2D& transformBounds );
    void cmd_flipVertically( const Rect2D& transformBounds );
    
protected:
    using ExteriorGroupImpl = MarkupPolygonGroupImpl< int >;
    using PropertyVector = std::vector< Property::Ptr >;
    
    Feature_Contour::Ptr m_pContour;
    PropertyVector m_properties;
    
    Site::WeakPtr m_pSiteParent;
    MarkupPath::PathCmdVector m_contourPath;
    ExteriorGroupImpl::PolyMap m_exteriorPolyMap;
    std::string m_strLabelText;
    boost::optional< wykobi::polygon< float, 2u > > m_polygonCache;
    
    std::unique_ptr< PathImpl > m_pContourPathImpl;
    std::unique_ptr< TextImpl > m_pLabel;
    std::unique_ptr< ExteriorGroupImpl > m_pExteriorPolygons;
    
    Site::PtrVector m_spaces;
};

}

#endif //BLUEPRINT_13_09_2013
