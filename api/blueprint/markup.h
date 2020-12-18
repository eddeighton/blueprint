
#ifndef MARKUP_IMPL_01_DEC_2020
#define MARKUP_IMPL_01_DEC_2020

#include "blueprint/cgalSettings.h"
#include "blueprint/geometry.h"
#include "blueprint/glyphSpec.h"

namespace Blueprint
{
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
template< class T >
class ControlPointCallback : public ControlPoint
{
    T& m_callback;
    int m_id;
public:
    ControlPointCallback( T& callback, int id )
        :   m_callback( callback ),
            m_id( id )
    {}
    
    void setIndex( int iIndex ) { m_id = iIndex; }
    
    virtual const GlyphSpec* getParent() const { return m_callback.getParent( m_id ); }
    virtual Float getX() const { return m_callback.getX( m_id ); }
    virtual Float getY() const { return m_callback.getY( m_id ); }
    virtual void set( Float fX, Float fY ) { m_callback.set( m_id, fX, fY ); }
    //virtual const std::string& getName() const { return m_callback.getName( m_id ); }
    virtual bool canEdit() const { return true; }
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class TextImpl : public MarkupText
{
public:
    TextImpl( const GlyphSpec* pParent, const std::string& strText, 
                const Float fx, const Float fy, 
                MarkupText::TextType type = eUnimportant )
        :   m_pParent( pParent ),
            m_strText( strText ),
            m_point( fx, fy ),
            m_type( type )
    {

    }
   // virtual const std::string& getName() const { return m_strText; }
    virtual const GlyphSpec* getParent() const { return m_pParent; }
    virtual const std::string& getText() const { return m_strText; }
    virtual MarkupText::TextType getType() const { return m_type; }
    virtual Float getX() const { return CGAL::to_double( m_point.x() ); }
    virtual Float getY() const { return CGAL::to_double( m_point.y() ); }

    //void setPos( const Point& pos ) { m_point = pos; }
private:
    const GlyphSpec* m_pParent;
    const std::string& m_strText;
    const MarkupText::TextType m_type;
    Point m_point;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class SimplePolygonMarkup : public MarkupPolygonGroup
{
public:
    SimplePolygonMarkup( const GlyphSpec* pParent, const Polygon& polygon, bool bFill )
        :   m_pParent( pParent ),
            m_polygon( polygon ),
            m_bFill( bFill )
    {
    }
    //virtual const std::string& getName() const { return m_strText; }
    virtual const GlyphSpec* getParent() const { return m_pParent; }
    
    virtual bool isPolygonsFilled() const { return m_bFill; }
    virtual std::size_t getTotalPolygons() const { return 1U; }
    virtual void getPolygon( std::size_t szIndex, MarkupPolygon& polygon ) const
    {
        if( 0U == szIndex )
        {
            polygon.clear();
            for( auto p : m_polygon )
            {
                polygon.push_back( std::make_pair( 
                    CGAL::to_double( p.x() ), 
                    CGAL::to_double( p.y() ) ) );
            }
        }
    }
    
private:
    const GlyphSpec* m_pParent;
    const Polygon& m_polygon;
    bool m_bFill;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
template< typename KeyType >
class MarkupPolygonGroupImpl : public MarkupPolygonGroup
{
public:
    using PolygonType = Polygon;
    using PolyMap = std::map< KeyType, PolygonType >;
    
    MarkupPolygonGroupImpl( const GlyphSpec* pParent, PolyMap& polygons, bool bFill )
        :   m_pParent( pParent ),
            m_polygons( polygons ),
            m_bFill( bFill )
    {
    }
    //virtual const std::string& getName() const { return m_strText; }
    virtual const GlyphSpec* getParent() const { return m_pParent; }
    
    virtual bool isPolygonsFilled() const { return m_bFill; }
    virtual std::size_t getTotalPolygons() const { return m_polygons.size(); }
    virtual void getPolygon( std::size_t szIndex, MarkupPolygon& polygon ) const
    {
        if( szIndex < m_polygons.size() )
        {
            PolyMap::iterator i = m_polygons.begin();
            std::advance( i, szIndex );
            polygon.clear();
            for( auto p : i->second )
            {
                polygon.push_back( std::make_pair( 
                    CGAL::to_double( p.x() ), 
                    CGAL::to_double( p.y() ) ) );
            }
        }
    }
private:
    const GlyphSpec* m_pParent;
    //std::string m_strText;
    PolyMap& m_polygons;
    bool m_bFill;
};

}

#endif //MARKUP_IMPL_01_DEC_2020