//  Copyright (c) Deighton Systems Limited. 2019. All Rights Reserved.
//  Author: Edward Deighton
//  License: Please see license.txt in the project root folder.

//  Use and copying of this software and preparation of derivative works
//  based upon this software are permitted. Any copy of this software or
//  of any derivative work must include the above copyright notice, this
//  paragraph and the one after it.  Any distribution of this software or
//  derivative works must comply with all applicable laws.

//  This software is made available AS IS, and COPYRIGHT OWNERS DISCLAIMS
//  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
//  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
//  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
//  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF COPYRIGHT OWNERS ARE ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGES.

#include "blueprint/blueprint.h"
#include "blueprint/factory.h"
#include "blueprint/compilation.h"
#include "blueprint/visibility.h"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/timer/timer.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <iostream>
#include <memory>
#include <map>

boost::filesystem::path constructPath( const std::string& strHTMLFile, const char* pszExt )
{
    boost::filesystem::path t = strHTMLFile;
    t.replace_extension( "" );
    std::ostringstream os;
    os << t.filename().string() << pszExt;
    return t.parent_path() / os.str();
}

void command_compile( bool bHelp, const std::vector< std::string >& args )
{
    std::string strDirectory, strProject, strBlueprint, strOut;//, strVis, strHTML, strIn;

    namespace po = boost::program_options;
    po::options_description commandOptions(" Build Project Command");
    {
        commandOptions.add_options()
            ("dir",         po::value< std::string >( &strDirectory ),  "Project directory")
            ("project",     po::value< std::string >( &strProject ),    "Project Name" )
            ("file",        po::value< std::string >( &strBlueprint ),  "Blueprint File" )
            //("html",        po::value< std::string >( &strHTML ),       "HTML file to generate" )
            //("in",          po::value< std::string >( &strIn ),         "Input file" )
            ("out",         po::value< std::string >( &strOut ),        "Output file" )
            //("vis",         po::value< std::string >( &strVis ),        "Visibility file" );
            
        ;
    }

    po::positional_options_description p;
    p.add( "dir", -1 );

    po::variables_map vm;
    po::store( po::command_line_parser( args ).options( commandOptions ).positional( p ).run(), vm );
    po::notify( vm );

    if( bHelp )
    {
        std::cout << commandOptions << "\n";
    }
    else
    {
        if( strBlueprint.empty() )
        {
            std::cout << "Missing blueprint file" << std::endl;
            return;
        }

        const boost::filesystem::path blueprintFilePath =
            boost::filesystem::edsCannonicalise(
                boost::filesystem::absolute( strBlueprint ) );

        if( !boost::filesystem::exists( blueprintFilePath ) )
        {
            THROW_RTE( "Specified blueprint file does not exist: " << blueprintFilePath.generic_string() );
        }
        
        {
            Blueprint::Factory factory;
            Blueprint::Blueprint::Ptr pBlueprint = 
                boost::dynamic_pointer_cast< ::Blueprint::Blueprint >( 
                    factory.load( blueprintFilePath.string() ) );
            VERIFY_RTE_MSG( pBlueprint, "Failed to load blueprint: " << blueprintFilePath.generic_string() );
        
            std::cout << "Loaded blueprint: " << blueprintFilePath.string() << std::endl;
            {
                const Blueprint::Site::EvaluationMode mode = { true, false, false };
                Blueprint::Site::EvaluationResults results;
                pBlueprint->evaluate( mode, results );
            }
            std::cout << "Evaluated blueprint: " << blueprintFilePath.string() << std::endl;
            
            Blueprint::Analysis::Ptr pAnalysis = 
                Blueprint::Analysis::constructFromBlueprint( pBlueprint );
            
            std::cout << "Analysis completed" << std::endl;
            
            if( !strOut.empty() )
            {
                const boost::filesystem::path compilationFilePath =
                    constructPath( strOut, ".bluc" );
                    
                std::unique_ptr< boost::filesystem::ofstream > pOutFile =
                    boost::filesystem::createBinaryOutputFileStream( compilationFilePath );
                    
                pAnalysis->save( *pOutFile );
            }
            /*
            Blueprint::Compilation compilation( pTest );
            std::cout << "Compiled blueprint: " << blueprintFilePath.string() << std::endl;
        
            Blueprint::FloorAnalysis floor( compilation, pTest );
            
            if( !strHTML.empty() )
            {
                compilation.render(         constructPath( strHTML, ".html" ) );
                compilation.renderFillers(  constructPath( strHTML, "__fillers.html" ) );
                compilation.renderFloors(   constructPath( strHTML, "__floors.html" ) );
                floor.render( constructPath( strHTML, "__floor.html" ) );
            }
            
            if( !strOut.empty() )
            {
                const boost::filesystem::path compilationFilePath =
                    constructPath( strOut, ".bluc" );
                    
                std::unique_ptr< boost::filesystem::ofstream > pOutFile =
                    boost::filesystem::createBinaryOutputFileStream( compilationFilePath );
                    
                compilation.save( *pOutFile );
            }
            
            if( !strVis.empty() )
            {
                Blueprint::Visibility visibility( floor );
                if( !strHTML.empty() )
                {
                    visibility.render( constructPath( strHTML, "__vis.html" ) );
                }
                
                const boost::filesystem::path visibilityFilePath =
                    constructPath( strVis, ".vis" );
                    
                std::unique_ptr< boost::filesystem::ofstream > pOutFile =
                    createBinaryOutputFileStream( visibilityFilePath );
                    
                visibility.save( *pOutFile );
            }*/
        }

    }

}
