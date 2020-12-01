
#ifndef MARKUP_IMPL_01_DEC_2020
#define MARKUP_IMPL_01_DEC_2020

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
    virtual float getX() const { return m_callback.getX( m_id ); }
    virtual float getY() const { return m_callback.getY( m_id ); }
    virtual void set( float fX, float fY ) { m_callback.set( m_id, fX, fY ); }
    virtual const std::string& getName() const { return m_callback.getName( m_id ); }
    virtual bool canEdit() const { return true; }
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
class TextImpl : public MarkupText
{
public:
    TextImpl( const GlyphSpec* pParent, const std::string& strText, const float fx, const float fy, MarkupText::TextType type = eUnimportant )
        :   m_pParent( pParent ),
            m_strText( strText ),
            m_fx( fx ),
            m_fy( fy ),
            m_type( type )
    {

    }
    virtual const std::string& getName() const { return m_strText; }
    virtual const GlyphSpec* getParent() const { return m_pParent; }
    virtual const std::string& getText() const { return m_strText; }
    virtual MarkupText::TextType getType() const { return m_type; }
    virtual float getX() const { return m_fx; }
    virtual float getY() const { return m_fy; }

    void setPos( const Point2D& pos ) { m_fx = pos.x; m_fy = pos.y; }
private:
    const GlyphSpec* m_pParent;
    const std::string& m_strText;
    const MarkupText::TextType m_type;
    float m_fx, m_fy;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
class PathImpl : public MarkupPath
{
public:
    template< class T, class ValueType = Point2D >
    class AGGContainerAdaptor : public T
    {
    public:
        AGGContainerAdaptor( const T& cpy ) : T( cpy ) {}
        typedef ValueType value_type;
    };

    template< class T >
    static void aggPathToMarkupPath( MarkupPath::PathCmdVector& path, T& vs )
    {
        path.clear();
        double x,y;
        unsigned cmd;
        vs.rewind( 0u );
        while(!agg::is_stop(cmd = vs.vertex(&x, &y)))
            path.push_back( MarkupPath::Cmd( static_cast< float >( x ), static_cast< float >( y ), cmd ) );
    }

    PathImpl( PathCmdVector& path, const GlyphSpec* pParent = 0u )
        :   m_pParent( pParent ),
            m_path( path )
    {
    }
    virtual const std::string& getName() const { return m_strText; }
    virtual const GlyphSpec* getParent() const { return m_pParent; }
    virtual const PathCmdVector& getCmds() const { return m_path; }
private:
    const GlyphSpec* m_pParent;
    std::string m_strText;
    PathCmdVector& m_path;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
template< typename KeyType >
class MarkupPolygonGroupImpl : public MarkupPolygonGroup
{
public:
    using PolygonType = Polygon2D;
    using PolyMap = std::map< KeyType, PolygonType >;
    
    MarkupPolygonGroupImpl( const GlyphSpec* pParent, PolyMap& polygons, bool bFill )
        :   m_pParent( pParent ),
            m_polygons( polygons ),
            m_bFill( bFill )
    {
    }
    virtual const std::string& getName() const { return m_strText; }
    virtual const GlyphSpec* getParent() const { return m_pParent; }
    
    virtual bool isPolygonsFilled() const { return m_bFill; }
    virtual std::size_t getTotalPolygons() const { return m_polygons.size(); }
    virtual void getPolygon( std::size_t szIndex, Polygon& polygon ) const
    {
        if( szIndex < m_polygons.size() )
        {
            PolyMap::iterator i = m_polygons.begin();
            std::advance( i, szIndex );
            polygon.clear();
            for( auto p : i->second )
            {
                polygon.push_back( std::make_pair( p.x, p.y ) );
            }
        }
    }
private:
    const GlyphSpec* m_pParent;
    std::string m_strText;
    PolyMap& m_polygons;
    bool m_bFill;
};

}

#endif //MARKUP_IMPL_01_DEC_2020