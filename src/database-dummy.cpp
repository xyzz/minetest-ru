/*
Dummy "database" class
*/


#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"
#include "voxel.h"
#include "porting.h"
#include "mapgen.h"
#include "nodemetadata.h"
#include "settings.h"
#include "log.h"
#include "profiler.h"
#include "nodedef.h"
#include "gamedef.h"
#include "util/directiontables.h"
#include "rollback_interface.h"

#include "database-dummy.h"

Database_Dummy::Database_Dummy(ServerMap *map)
{
	srvmap = map;
}

int Database_Dummy::Initialized(void)
{
	return 1;
}

void Database_Dummy::beginSave() {}
void Database_Dummy::endSave() {}

void Database_Dummy::saveBlock(MapBlock *block)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Dummy blocks are not written
	*/
	if(block->isDummy())
	{
		return;
	}

	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST;
	// Get destination
	v3s16 p3d = block->getPos();

	/*
		[0] u8 serialization version
		[1] data
	*/

	std::ostringstream o(std::ios_base::binary);
	o.write((char*)&version, 1);
	// Write basic data
	block->serialize(o, version, true);
	// Write block to database
	std::string tmp = o.str();

	m_database[getBlockAsInteger(p3d)] = tmp;
	// We just wrote it to the disk so clear modified flag
	block->resetModified();
}

MapBlock* Database_Dummy::loadBlock(v3s16 blockpos)
{
	v2s16 p2d(blockpos.X, blockpos.Z);

        if(m_database.count(getBlockAsInteger(blockpos))) {
                /*
                        Make sure sector is loaded
                */
                MapSector *sector = srvmap->createSector(p2d);
                /*
                        Load block
                */
                std::string datastr = m_database[getBlockAsInteger(blockpos)];
//                srvmap->loadBlock(&datastr, blockpos, sector, false);

		try {
                	std::istringstream is(datastr, std::ios_base::binary);
                   	u8 version = SER_FMT_VER_INVALID;
                     	is.read((char*)&version, 1);

                     	if(is.fail())
                             	throw SerializationError("ServerMap::loadBlock(): Failed"
                                	             " to read MapBlock version");

                     	MapBlock *block = NULL;
                     	bool created_new = false;
                     	block = sector->getBlockNoCreateNoEx(blockpos.Y);
                     	if(block == NULL)
                     	{
                             	block = sector->createBlankBlockNoInsert(blockpos.Y);
                             	created_new = true;
                     	}
                     	// Read basic data
                     	block->deSerialize(is, version, true);
                     	// If it's a new block, insert it to the map
                     	if(created_new)
                             	sector->insertBlock(block);
                     	/*
                             	Save blocks loaded in old format in new format
                     	*/

                     	//if(version < SER_FMT_VER_HIGHEST || save_after_load)
                     	// Only save if asked to; no need to update version
                     	//if(save_after_load)
                        //     	saveBlock(block);
                     	// We just loaded it from, so it's up-to-date.
                     	block->resetModified();

             	}
             	catch(SerializationError &e)
             	{
                     	errorstream<<"Invalid block data in database"
                                     <<" ("<<blockpos.X<<","<<blockpos.Y<<","<<blockpos.Z<<")"
                                     <<" (SerializationError): "<<e.what()<<std::endl;
                     // TODO: Block should be marked as invalid in memory so that it is
                     // not touched but the game can run

                     	if(g_settings->getBool("ignore_world_load_errors")){
                             errorstream<<"Ignoring block load error. Duck and cover! "
                                             <<"(ignore_world_load_errors)"<<std::endl;
                     	} else {
                             throw SerializationError("Invalid block data in database");
                             //assert(0);
                     	}
             	}

                return srvmap->getBlockNoCreateNoEx(blockpos);  // should not be using this here
        }
	return(NULL);
}

void Database_Dummy::listAllLoadableBlocks(core::list<v3s16> &dst)
{
	for(std::map<unsigned long long, std::string>::iterator x = m_database.begin(); x != m_database.end(); ++x)
	{
		v3s16 p = getIntegerAsBlock(x->first);
		//dstream<<"block_i="<<block_i<<" p="<<PP(p)<<std::endl;
		dst.push_back(p);
	}
}

Database_Dummy::~Database_Dummy()
{
	m_database.clear();
}
