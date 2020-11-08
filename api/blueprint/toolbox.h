#ifndef TOOLBOX_23_09_2013
#define TOOLBOX_23_09_2013

#include "site.h"

#include "common/tick.hpp"

#include "ed/node.hpp"

#include <boost/shared_ptr.hpp>

#include <string>

namespace Blueprint
{

class Toolbox
{
public:
    class Palette
    {
    public:
        typedef boost::shared_ptr< Palette > Ptr;
        typedef std::map< std::string, Ptr > PtrMap;
        Palette( const std::string& strName, int iMaximumSize = -1 );

        bool operator<( const Palette& cmp ) const { return m_strName < cmp.m_strName; }

        const std::string& getName() const { return m_strName; }
        const Site::PtrList& get() const { return m_clips; }
        Site::Ptr getSelection() const;
        Site::Ptr getTopMost() const { return m_clips.empty() ? Site::Ptr() : m_clips.front(); }
        const Timing::UpdateTick& getLastModifiedTick() const { return m_updateTick; }

        void add( Site::Ptr pClip, bool bSelect = true );
        template< class TCont >
        void addABunch( const TCont& container, bool bSelect = false )
        {
            for( auto& i : container )
            {
                add( i, bSelect );
            }
        }

        void remove( Site::Ptr pClip );
        void clear();

        void select( Site::Ptr pSite );

    private:
        const std::string m_strName;
        int m_iMaximumSize;
        Site::PtrList m_clips;
        Site::PtrList::iterator m_iterSelection;
        Timing::UpdateTick m_updateTick;
    };
    typedef boost::shared_ptr< Toolbox > Ptr;

    Toolbox( const std::string& strDirectoryPath );
    
    void reload();

    Site::Ptr getCurrentItem() const;
    const Palette::PtrMap& get() const { return m_palettes; }
    Palette::Ptr getPalette( const std::string& strName ) const;
    void selectPalette( Palette::Ptr pPalette );

    void add( const std::string& strName, Site::Ptr pNode, bool bSelect = false );
    template< class TCont >
    void addABunch( const std::string& strName, const TCont& container, bool bSelect = false )
    {
        for( auto& i : container )
        {
            add( strName, i, bSelect );
        }
    }
    void remove( Palette::Ptr pPalette );
    
    template< typename TValue >
    void getConfigValue( const std::string& strKey, TValue& value )
    {
        static std::map< std::string, TValue > cache;
        
        std::map< std::string, TValue >::const_iterator iFind = cache.find( strKey );
        if( iFind != cache.end() )
        {
            value = iFind->second;
            return;
        }
        
        boost::optional< const Ed::Node& > findResult = 
            Ed::find( m_config, strKey );
        Ed::IShorthandStream is( findResult.get().statement.shorthand.get() );
        is >> value;
        
        cache.insert( std::make_pair( strKey, value ) );
    }
    
private:
    void recursiveLoad( const boost::filesystem::path& pathIter, const std::string& strCurrent );
private:
    boost::filesystem::path m_rootPath;
    Palette::PtrMap m_palettes;
    Palette::Ptr m_pCurrentPalette;
    
    Ed::Node m_config;
};


}

#endif //TOOLBOX_23_09_2013