/**
 * This file has been created by Three Cubes.
 * 
 * ****************
 *    Changelog
 * ****************
 * 2023-06-17(@Shumeras) - Scaffolded functions. Added main generate world CLI function and generate new and from world functions;
 */

#pragma once

#include "settings.h"
#include "porting.h"
#include "exceptions.h"
#include "content/subgames.h"
#include "filesys.h"
#include "server.h"
#include "content/subgames.h"

#include <string>
#include <stack>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cerrno>

StringMap createStringMapFromParamString(const std::string paramString)
{
    std::stringstream paramStream(paramString);
    std::stack<std::string> params;
    std::string param = "";
    StringMap paramMap;

    while (std::getline(paramStream, param, ';'))
    {
        params.push(param);
    }
    
    std::size_t pos = std::string::npos;
    while(!params.empty())
    {
        param = params.top();
        pos = param.find('=');
        //TODO throw err
        paramMap.insert(
            std::pair<std::string, std::string>(param.substr(0, pos), param.substr(pos+1, param.size()))
        );
        params.pop();
    }
    return paramMap;
}

//TODO this is deffinetely still messy
// Will probably work for now, but neet to double check erronious inputs
// Make sure there is no undefined behaviour and handle errors

bool generateWorldNew(const Settings& cmdArgs)
{
    // Get name             --name
    // Get path             --out-path
    // Get subgameid        --subgameid
    // Dmg enabled          --dmg-enabled
    // Creative             --creative
    // Get mapgen settings  --mapgen-opts 

    std::string name = cmdArgs.exists("name")? cmdArgs.get("name") : "MyWorld";
    std::string path = (cmdArgs.exists("out-path")
        ? cmdArgs.get("out-path") 
        : porting::path_user + DIR_DELIM "worlds") 
            + DIR_DELIM + sanitizeDirName(name, "world_");
        
    std::vector<SubgameSpec> games = getAvailableGames();
    if(games.size() == 0)
    {
        std::cerr<< "No subgames available" << std::endl;
        return false;
    }
    std::string gameId = cmdArgs.exists("subgameid")
        ? cmdArgs.get("subgameid") 
        : games.front().id;
    bool enableDmg = cmdArgs.exists("dmg-enabled") 
        ? cmdArgs.getBool("dmg-enabled") 
        : true;
    bool creative = cmdArgs.exists("creative") 
        ? cmdArgs.getBool("creative") 
        : true;
    std::string worldGenOptString = "mg_name=v7;mg_flags=light,decorations,ores,caves,biomes,dungeons;fixed_map_seed=";
    
    StringMap worldGenOpts = createStringMapFromParamString(worldGenOptString);
    // Set inital worldGenOpts in case something is unspecified later
   // std::cout<<"----- MAPGEN OPTIONS -----"<<std::endl;
    for (auto &it : worldGenOpts) {
	//	std::cout<< it.first + "=" + it.second<<std::endl;
        g_settings->set(it.first, it.second);
	}
    
    if(cmdArgs.exists("mapgen-opts"))
    {
        worldGenOpts = createStringMapFromParamString(cmdArgs.get("mapgen-opts"));
        for (auto &it : worldGenOpts) {
	    //	std::cout<< it.first + "=" + it.second<<std::endl;
            g_settings->set(it.first, it.second);
	    }   
    }

    // Finding/Setting subgamespec
    auto gameIt = std::find_if(games.begin(), games.end(), 
		[gameId] (const SubgameSpec &spec) 
		{
			return spec.id == gameId;
		}
	);
    if (gameIt == games.end()) {
		std::cerr<<"Game '" + gameId + "' not found"<<std::endl;
		return false;
	}

    g_settings->setBool("creative_mode", creative);
    g_settings->setBool("enable_damage", enableDmg);
    
    try{
        loadGameConfAndInitWorld(path, name, *gameIt, true);
    }
    catch (const BaseException &exception){
        std::cerr<<exception.what()<<std::endl;
        return false;
    }
    std::cout<< "World created at: " + path << std::endl;
    return true;
}

bool generateWorldFromWorld(const Settings& cmdArgs)
{
    // Get gen source       --source
    // Get path             --out-path
    // Get name             --name
    
    // Checking source
    if(!cmdArgs.exists("source"))
    {
        std::cerr<<"No source for generation specified"<<std::endl;
        return false;
    }
    
    std::string source = fs::AbsolutePath(cmdArgs.get("source"));
    //std::cout<<source<<std::endl;
    
    if(source == "" || !fs::PathExists(source) || !fs::IsDir(source))
    {
        std::cout<<"Provided source: '" + source 
            + "' does not exist or is not a world folder" <<std::endl;
        return false;
    }

    std::vector<std::string> dirContent;
    fs::GetRecursiveSubPaths(source, dirContent, true);
    std::vector<std::string> expectedMapFiles = {
        "auth.sqlite",
        "env_meta.txt",
        "map.sqlite",
        "map_meta.txt",
        "mod_storage.sqlite",
        "players.sqlite",
        "world.mt"
    };

    // Strip filenames from subdirs
    for (auto &&file : dirContent)
    {
        file = fs::GetFilenameFromPath(file.c_str());
        std::cout<<file<<std::endl;
    }

    bool containsAll = std::all_of(expectedMapFiles.begin(), expectedMapFiles.end(), 
        [&](const std::string& s) {

            return std::find(dirContent.begin(), dirContent.end(), s) != dirContent.end();
        }
    );

    if(!containsAll){
        std::cout<<"Provided source world is either unitialized or is not a world folder"<<std::endl;
        return false;
    }

    // Cheking name
    std::string name = cmdArgs.exists("name")
        ? cmdArgs.get("name")
        : fs::GetFilenameFromPath(source.c_str());

    if(name.compare(fs::GetFilenameFromPath(source.c_str())) == 0)
        name += "_copy";

    // Check output dir
    std::string outputPath = cmdArgs.exists("out-path")
        ? cmdArgs.get("out-path")
        : porting::path_share + DIR_DELIM + "worlds";

    outputPath += DIR_DELIM + name;

    if(fs::PathExists(outputPath))
    {
        std::cout<<"Provided output path '" + outputPath 
            + "' already exists."<<std::endl;
        return false;
    }

    // Copy
    fs::CreateDir(outputPath);
    fs::CopyDir(source, outputPath);

    // Update copied world info with new name
    std::string worldConfigFilePath = outputPath + DIR_DELIM + "world.mt";
    std::vector<std::string> lines;

    std::ifstream inFile(worldConfigFilePath);
    if (inFile.is_open()) {
        std::string line;
        while (getline(inFile, line)) {
            lines.push_back(line);
        }
        inFile.close();
    } else {
        std::cerr << "Unable to open world config file for reading."<<std::endl;
        return false;
    }

    if (!lines.empty()) {
        std::string target = "world_name";
        std::transform(lines.begin(), lines.end(), lines.begin(),
            [&](std::string& str) {
                if (str.substr(0, target.size()).compare(target) == 0)
                {
                    str = "world_name = " + name;
                }
                return str;
            }
        );
        // Might be an issue if there is for some reason the world.mt doesn't contain a world_name,
        // but that probably idicates a fuck-up in another location
    }
    else{
        std::cerr << "World config file is empty?"<<std::endl;
        return false;
    }

    std::ofstream outFile(worldConfigFilePath);
    if (outFile.is_open()) {
        for (const auto& line : lines) {
            outFile << line << "\n";
        }
        outFile.close();
    } else {
        std::cout << "Unable to open world config file for writing.\n";
        return false;
    }

    return true;
}

bool generateWorldFromTemplate(const Settings& cmdArgs)
{
    //TODO this at a later date
    //Just copying a world is probably fine for now
    return false;
}

bool generateWorldCli(const Settings& cmdArgs){
    // Generation strategy  --from      new|world|template
    // Get gen source       --source
    // Get path             --out-path
    // Get name             --name
    // Get subgameid        --gameid
    // Get mapgen settings  --mapgen-opts 
    // Dmg enabled          --dmg-enabled
    // Creative             --creative

    // Default settings
    std::string generationStrategy = "new";
    try
    {
        generationStrategy = cmdArgs.get("from");
    }
    catch (const SettingNotFoundException &exception)
    {
        std::cout << "No world generation specified - assuming 'new'" << std::endl;
    }

    if(generationStrategy == "new")
        return generateWorldNew(cmdArgs);

    if(generationStrategy == "world")
        return generateWorldFromWorld(cmdArgs);

    if(generationStrategy == "template")
        return generateWorldFromTemplate(cmdArgs);
    
    std::cerr<<"Unknown generation strategy: " + generationStrategy << std::endl;
    return false;
    
}
