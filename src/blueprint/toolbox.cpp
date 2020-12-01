#include "blueprint/toolbox.h"
#include "blueprint/factory.h"

#include "blueprint/clip.h"
#include "blueprint/space.h"
#include "blueprint/wall.h"
#include "blueprint/connection.h"
#include "blueprint/object.h"

#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include <boost/filesystem.hpp>

namespace Blueprint
{
    
Toolbox::Palette::Palette( const std::string& strName, int iMaximumSize )
    :   m_strName( strName ),
        m_iMaximumSize( iMaximumSize ),
        m_iterSelection( m_clips.end() )
{
}

void Toolbox::Palette::add( Site::Ptr pClip, bool bSelect )
{
    m_clips.push_front( pClip );
    if( bSelect || m_iterSelection == m_clips.end() )
        m_iterSelection = m_clips.begin();

    if( m_iMaximumSize > 0 )
    {
        while( static_cast< int >( m_clips.size() ) > m_iMaximumSize )
        {
            if( !bSelect && *m_iterSelection == m_clips.back() )
            {
                Site::Ptr pSelectedSite = *m_iterSelection;
                m_clips.pop_back();
                if( m_clips.size() > 1u )
                {
                    m_clips.pop_back();
                    m_clips.push_back( pSelectedSite );
                    m_iterSelection = m_clips.end();
                    --m_iterSelection;
                }
                else
                    m_iterSelection = m_clips.end();
            }
            else
                m_clips.pop_back();
        }
    }

    m_updateTick.update();
}

void Toolbox::Palette::remove( Site::Ptr pClip )
{
    Site::PtrList::iterator iFind = std::find( m_clips.begin(), m_clips.end(), pClip );
    if( iFind != m_clips.end() )
    {
        const bool bWasSelection = ( m_iterSelection == m_clips.end() ) ? false : ( pClip == *m_iterSelection );
        Site::PtrList::iterator iNext = m_clips.erase( iFind );
        if( bWasSelection )
        {
            if( !m_clips.empty() )
            {
                if( iNext != m_clips.end() )
                    m_iterSelection = iNext;
                else
                    m_iterSelection = --(m_clips.end());
            }
            else
                m_iterSelection = m_clips.end();
        }
    }
    m_updateTick.update();
}

void Toolbox::Palette::clear()
{
    m_iterSelection = m_clips.end();
    m_clips.clear();
    m_updateTick.update();
}

Site::Ptr Toolbox::Palette::getSelection() const
{
    Site::Ptr pSelected;
    if( m_iterSelection != m_clips.end() )
        pSelected = *m_iterSelection;
    return pSelected;
}

void Toolbox::Palette::select( Site::Ptr pSite )
{
    Site::PtrList::iterator iFind = std::find( m_clips.begin(), m_clips.end(), pSite );
    ASSERT( iFind != m_clips.end() );
    if( iFind != m_clips.end() )
        m_iterSelection = iFind;
    m_updateTick.update();
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

    
std::string pathToName( const boost::filesystem::path& p )
{
    return p.string();
    //return p.leaf().replace_extension().string();
}

void Toolbox::recursiveLoad( const boost::filesystem::path& pathIter, const std::string& strCurrent )
{
    using namespace boost::filesystem;
    for( directory_iterator iter( pathIter ); iter != directory_iterator(); ++iter )
    {
        boost::filesystem::path pth = *iter;
        if( is_regular_file( *iter ) && pth.extension().string() == ".blu" )
        {
            //load the file into the current palette
            Factory factory;
            Node::PtrVector loadedNodes;
            factory.load( canonical( absolute( *iter ) ).string(), loadedNodes );

            Site::PtrList clips;
            std::for_each( loadedNodes.begin(), loadedNodes.end(),
                generics::collectIfConvert( clips, 
                    Node::ConvertPtrType< Site >(), Node::ConvertPtrType< Site >() ) );
            addABunch( strCurrent, clips );
        }
        else if( is_directory( *iter ) )
        {
            recursiveLoad( *iter, pathToName( *iter ) );
        }
    }
}
    
Toolbox::Toolbox( const std::string& strDirectoryPath )
{
    //recursively load all blueprints under the root directory
    using namespace boost::filesystem;
    VERIFY_RTE_MSG( exists( strDirectoryPath ), "Could not locate toolbox data path at: " << strDirectoryPath );
    m_rootPath = canonical( absolute( strDirectoryPath ) );
    VERIFY_RTE_MSG( exists( m_rootPath ), "Path did not canonicalise properly: " << m_rootPath.string() );
    
    //attempt to load the config file if it exists
    const path configFile = m_rootPath / "config.ed";
    if( exists( configFile ) )
    {
        Ed::BasicFileSystem filesystem;
        Ed::File edConfigFile( filesystem, configFile.string() );
        edConfigFile.expandShorthand();
        edConfigFile.removeTypes();
        edConfigFile.toNode( m_config );
    }
    else
    {
        THROW_RTE( "Failed to locate file: " << configFile.string() );
    }
    
    reload();
}

void Toolbox::reload()
{
    //recursively load all blueprints under the root directory
    using namespace boost::filesystem;
    
    m_pCurrentPalette.reset();
    m_palettes.clear();
    
    //generate defaults...
    {
        Clip::Ptr pDefaultClip( new Clip( Site::Ptr(), "space" ) );
        Space::Ptr pDefaultSpace( new Space( pDefaultClip, "space" ) );
        pDefaultClip->add( pDefaultSpace );
        add( "clipboard", pDefaultClip, true );
    }
    {
        Clip::Ptr pDefaultClip( new Clip( Site::Ptr(), "wall" ) );
        Wall::Ptr pDefaultSpace( new Wall( pDefaultClip, "wall" ) );
        pDefaultClip->add( pDefaultSpace );
        add( "clipboard", pDefaultClip, true );
    }
    {
        Clip::Ptr pDefaultClip( new Clip( Site::Ptr(), "object" ) );
        Object::Ptr pDefaultSpace( new Object( pDefaultClip, "object" ) );
        pDefaultClip->add( pDefaultSpace );
        add( "clipboard", pDefaultClip, true );
    }
    {
        Clip::Ptr pDefaultClip( new Clip( Site::Ptr(), "connection" ) );
        Connection::Ptr pDefaultSpace( new Connection( pDefaultClip, "connection" ) );
        pDefaultClip->add( pDefaultSpace );
        add( "clipboard", pDefaultClip, true );
    }

    recursiveLoad( m_rootPath, pathToName( m_rootPath ) );
}
    
Site::Ptr Toolbox::getCurrentItem() const
{
    Site::Ptr pItem;
    if( m_pCurrentPalette )
        pItem = m_pCurrentPalette->getSelection();
    return pItem;
}

Toolbox::Palette::Ptr Toolbox::getPalette( const std::string& strName ) const
{
    Palette::Ptr pResult;
    Palette::PtrMap::const_iterator iFind = m_palettes.find( strName );
    if( iFind != m_palettes.end() )
        pResult = iFind->second;
    return pResult;
}

void Toolbox::selectPalette( Palette::Ptr pPalette )
{
    m_pCurrentPalette = pPalette;
}

void Toolbox::add( const std::string& strName, Site::Ptr pNode, bool bSelect )
{
    Palette::Ptr pPalette = getPalette( strName );
    if( !pPalette )
    {
        pPalette = Toolbox::Palette::Ptr( new Toolbox::Palette( strName ) );
        m_palettes.insert( std::make_pair( strName, pPalette ) );
    }
    pPalette->add( pNode, bSelect );
    if( !m_pCurrentPalette ) m_pCurrentPalette = pPalette;
}

void Toolbox::remove( Palette::Ptr pPalette )
{
    Palette::PtrMap::const_iterator iFind = m_palettes.find( pPalette->getName() );
    if( iFind != m_palettes.end() )
    {
        iFind = m_palettes.erase( iFind );
        if( m_pCurrentPalette == pPalette )
        {
            if( iFind != m_palettes.end() )
                m_pCurrentPalette = iFind->second;
            else if( m_palettes.empty() )
                m_pCurrentPalette.reset();
            else
                m_pCurrentPalette = (--(m_palettes.end()))->second;
        }
    }
}

}