#ifndef SITE_13_09_2013
#define SITE_13_09_2013

#include "blueprint/cgalSettings.h"
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
        bool bArrangement   = false;
        bool bCellComplex   = false;
        bool bClearance     = false;
    };
    virtual void evaluate( const EvaluationMode& mode, EvaluationResults& results );

    //GlyphSpec
    virtual bool canEdit() const { return true; }
    virtual const std::string& getName() const { return Node::getName(); }
    virtual const GlyphSpec* getParent() const { return m_pSiteParent.lock().get(); }
    
    //origin transform
    Transform getAbsoluteTransform() const;
    virtual void setTransform( const Transform& transform );
    virtual const MarkupPolygonGroup* getMarkupContour() const
    {
        return m_pContourPathImpl.get();
    }
    virtual void set( Float fX, Float fY )
    {
        const Float fNewValueX = Map_FloorAverage()( fX );
        const Float fNewValueY = Map_FloorAverage()( fY );
        if( getTranslation( m_transform ) != Vector( fNewValueX, fNewValueY ) )
        {
            setTranslation( m_transform, fNewValueX, fNewValueY );
            setModified();
        }
    }
    
    //GlyphSpecProducer
    virtual void getMarkupTexts( MarkupText::List& text) 
    { 
        if( m_pLabel.get() ) 
            text.push_back( m_pLabel.get() ); 
    }
    
    //spaces
    const Site::PtrVector& getSites() const { return m_sites; }
    
    //cmds
    void cmd_rotateLeft( const Rect& transformBounds );
    void cmd_rotateRight( const Rect& transformBounds );
    void cmd_flipHorizontally( const Rect& transformBounds );
    void cmd_flipVertically( const Rect& transformBounds );
    
    
    virtual Feature_Contour::Ptr getContour() const = 0;
    const Polygon& getContourPolygon() const { return m_contourPolygon; }
    
protected:
    using PropertyVector = std::vector< Property::Ptr >;
    
    Site::WeakPtr m_pSiteParent;
    PropertyVector m_properties;
    
    Polygon m_contourPolygon;
    std::string m_strLabelText;
    
    std::unique_ptr< SimplePolygonMarkup > m_pContourPathImpl;
    std::unique_ptr< TextImpl > m_pLabel;
    
    Site::PtrVector m_sites;
};

}

#endif //BLUEPRINT_13_09_2013
