/*###################################################################################
	
	Author: Wayky

	Sprite-Splitter:
		A simple program for sprite bulk extraction. Separates single frames from sprite sheets of
		the game 'Nuclear Throne' (and possibly other GameMaker Studio titles too).

		This program cannot extract 'data.win' files, so make sure to use other
		tools for that, like quickbms:
			http://aluigi.altervista.org/quickbms.htm
			http://aluigi.org/papers/bms/yoyogames.bms

	Thanks to PoroCYon for illustrating SPRT and TPAG file formats:
		https://gist.github.com/PoroCYon/4045acfcad7728b87a0d#sprt--listchunksprite

###################################################################################*/


#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <algorithm>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"

namespace opt = boost::program_options;
namespace fs = boost::filesystem;

/*
	Type size constants for readability
*/
constexpr unsigned ATT_LEN = 11; // Number of attributes per entry in paging table
constexpr unsigned ATT_SIZE = 2; // Single Word byte size
constexpr unsigned DW = 4; // Double Word byte size


/*
	Loads binary file as a vector of unsigned chars
*/
std::vector<uint8_t> load_buffer( const fs::path & f_path ){
	std::ifstream file( f_path.string(), std::ios::binary );
	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
	file.close();

	return buffer;
}

/*
	Loads table file as vector of unsigned chars and runs basic format test.
	Semantical correctness of entries' attributes is checked as needed in split_sprite()
*/
std::vector<uint8_t> load_table( const fs::path & f_path ){
	std::vector<uint8_t> table = load_buffer(f_path);
	uint32_t n_entries = *reinterpret_cast<uint32_t*>(&table[0]);

	if( (n_entries*(DW + ATT_LEN*ATT_SIZE) + DW) > table.size() )
		throw "Invalid table size";

	return table;
}

/*
	Checks wether frame offset and size are valid for the appropiate sprite sheet 
*/
bool __is_data_correct__( uint16_t * data, const cv::Mat & Sheet ){
	return (!Sheet.empty() and
			data[0]<Sheet.rows and
			data[1]<Sheet.cols and
			(data[2]+data[1])<Sheet.cols and (data[3]+data[0])<Sheet.rows );
}

/*
	Splits frames described in file 'sprt_path', and stores results in 'out_dir_path'.
*/
void split_sprite( std::vector<uint8_t> & table, std::vector<cv::Mat> & sheets, const fs::path & sprt_path, const fs::path & out_dir_path, bool verbose=false ){
	std::vector<uint8_t> sprt_info = load_buffer(sprt_path);
	
	uint32_t n_entries = *reinterpret_cast<uint32_t*>(&table[0]);
	uint32_t lowest_entry = *reinterpret_cast<uint32_t*>(&table[DW]); // Entry with lowest offset key in table
	uint32_t highest_entry = *reinterpret_cast<uint32_t*>(&table[DW + DW*n_entries]); // Entry with highest offset key in table
	uint32_t attrib_offset = DW*(n_entries+1); // Offset to the actual data of the entries

	uint32_t n_sprites = *reinterpret_cast<uint32_t*>(&sprt_info[13*DW]);
	uint32_t * sprites_offsets = reinterpret_cast<uint32_t*>(&sprt_info[14*DW]);
	uint32_t global_offset;
	uint16_t * data;

	if( verbose ){
		std::cout << "\n\nFile: " << sprt_path
			  	  << "\nNumber of frames: " << n_sprites;
	}

	for( unsigned i=0; i<n_sprites; ++i ){

		// If there isn't a entry on table for a certain frame or it isn't valid, skip it.
		if( sprites_offsets[i]>highest_entry or sprites_offsets[i]<lowest_entry or ((sprites_offsets[i] - lowest_entry)%(ATT_LEN*ATT_SIZE))!=0 ){
			if( verbose )
				std::cerr << "\n\tSkipping frame " << i+1 << " (invalid table entry)";
			continue;
		}

		global_offset = (sprites_offsets[i] - lowest_entry) + attrib_offset;
		data = reinterpret_cast<uint16_t*>( &table[global_offset] );

		if( data[10] > sheets.size() ){
			if( verbose )
				std::cerr << "\n\tSkipping frame " << i+1 << " (invalid sheet " << data[10] << ")";
			continue;
		}

		if( !__is_data_correct__( data, sheets[ data[10] ] ) ){
			if( verbose )
				std::cerr << "\n\tSkipping frame " << i+1 << " (incorrect frame data)";
			continue;
		}

		cv::Rect frame_box(data[0], data[1], data[2], data[3]);
		cv::Mat Frame = sheets[ data[10] ](frame_box);
		fs::path output = out_dir_path / sprt_path.filename();
		output /= sprt_path.filename().string() + std::to_string(i+1) + std::string(".png");

		if( verbose ){
			printf("\n\tFrame %d: [x:%d, y:%d, w:%d, h:%d, bbX:%d, bbY:%d, bbW:%d, bbH:%d, sheet:%d]",i+1, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[10]) ;
			std::cout << "\n\tOutput: " << output << "\n";
		}

		if( !fs::exists( output.parent_path() ) )
			fs::create_directory( output.parent_path() );

		if( !cv::imwrite( output.string(), Frame, std::vector<int>(cv::IMWRITE_PNG_COMPRESSION,0) ) )
			std::cerr << "Couldn't save " << output;
	}
}

int main( int argc, char ** argv ){

	/*
		Argument Handling
	*/
	opt::options_description description("Supported Arguments");

	description.add_options()
		("help","Show help page")
		("data", opt::value< std::vector<std::string> >(),"Specifies folder with data.win files (assumes SPRT, TPAG and TXTR folders do exist). To be used exclusively with --target option")
		("sprt", opt::value< std::vector<std::string> >(),"Specifies folder with sprt files")
		("tpag", opt::value< std::vector<std::string> >(),"Specifies paging table folder")
		("txtr", opt::value< std::vector<std::string> >(),"Specifies sprite sheets' location")
		("target,t", opt::value< std::vector<std::string> >(),"Specifies folder where splitted sprites will be saved")
		("verbose,v","Shows information during execution")
	;

	opt::variables_map vm;
	opt::store(opt::parse_command_line(argc, argv, description), vm);
	opt::notify(vm);

	fs::path sprt;
	fs::path tpag;
	fs::path txtr;
	fs::path target;

	bool verbose = vm.count("verbose") > 0;

	if( vm.count("help") ){
		std::cout << description << "\n";
		return EXIT_FAILURE;
	}

	if( vm.count("target") ){
		target = fs::path(vm["target"].as< std::vector<std::string> >()[0]);

		if( vm.count("data") ){
		
			fs::path dat = fs::path(vm["data"].as< std::vector<std::string> >()[0]);
			sprt = dat / fs::path("SPRT");
			tpag = dat / fs::path("TPAG");
			txtr = dat / fs::path("TXTR");
			
		}
		else if( vm.count("sprt") && vm.count("tpag") && vm.count("txtr") ){
			sprt = fs::path( fs::path(vm["sprt"].as< std::vector<std::string> >()[0]) );
			tpag = fs::path( fs::path(vm["tpag"].as< std::vector<std::string> >()[0]) );
			txtr = fs::path( fs::path(vm["txtr"].as< std::vector<std::string> >()[0]) );
		}
		else{
			std::cerr << "Error: Must specify either --data folder or all folders individually";
			return EXIT_FAILURE;
		}
	}
	else{
		std::cerr << "Must specify a --target directory for sprites";
		return EXIT_FAILURE;
	}


	/*
		Load paging table, if possible.
	*/	
	fs::directory_iterator end;

	fs::path tab;
	std::regex pattern("(.*)(\\.dat)");

	for ( boost::filesystem::directory_iterator it( tpag ); it!=end; ++it ){
		// File must be regular and end in '.dat'. Format is roughly checked in load_table()
		if( fs::is_regular( it->status() ) and std::regex_match( it->path().filename().string(), pattern ) )
			tab = it->path();
	}

	if( tab.empty() ){
		std::cerr << "Error: No paging table was found in specified folder";
		return EXIT_FAILURE;
	}

	// Check format and load if succesful
	std::vector<uint8_t> table = load_table( tab );
	

	/*
		Load all sprite sheets.
	*/	
	std::vector<cv::Mat> sheets;
	for ( boost::filesystem::directory_iterator it( txtr ); it!=end; ++it ){
		sheets.push_back( cv::imread( it->path().string(), cv::IMREAD_UNCHANGED ) );
	}


	/*
		Checks if target directory exist, creates it if not
	*/
	if( !fs::exists( target ) )
		fs::create_directory(target);


	/*
		Iterate through every sprite description file, and apply the corresponding split.
	*/
	for ( boost::filesystem::directory_iterator it( sprt ); it!=end; ++it )
		// Files must be regular and have at least 14 32bit fields, assumig a minimum of 1 frame per sprite.
		if( fs::is_regular( it->status() ) and fs::file_size( it->path() ) > (14*DW) )
			split_sprite( table, sheets, it->path(), target, verbose );


	return 0;
}