#ifndef _KV_DB_INDEXMANAGER_H_
#define _KV_DB_INDEXMANAGER_H_

#include <string>
#include <sys/time.h>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "Utils.h"
#include "KeyDigestHandle.h"
#include "LinkedList.h"

using namespace std;

namespace kvdb{

    class DataHeader {
    public:
        Kvdb_Digest key_digest;
        uint16_t data_size;
        uint32_t data_offset;
        uint32_t next_header_offset;

    public:
        DataHeader();
        DataHeader(Kvdb_Digest &digest, uint16_t data_size, uint32_t data_offset, uint32_t next_header_offset);
        DataHeader(const DataHeader& toBeCopied);
        ~DataHeader();
        DataHeader& operator=(const DataHeader& toBeCopied);
        uint16_t GetDataSize(){return data_size;}
        uint32_t GetDataOffset(){return data_offset;}
        uint32_t GetNextHeadOffset(){return next_header_offset;}

    } __attribute__((__packed__));

    class DataHeaderOffset{
    public:
        uint64_t physical_offset;

    public:
        DataHeaderOffset(): physical_offset(0){}
        DataHeaderOffset(uint64_t offset);
        DataHeaderOffset(const DataHeaderOffset& toBeCopied);
        ~DataHeaderOffset();
        DataHeaderOffset& operator=(const DataHeaderOffset& toBeCopied);
        uint64_t GetHeaderOffset(){return physical_offset;}

    }__attribute__((__packed__));

    class HashEntryOnDisk {
    public:
        DataHeader header;
        DataHeaderOffset header_offset;

    public:
        HashEntryOnDisk();
        HashEntryOnDisk(DataHeader& dataheader, DataHeaderOffset& offset);
        HashEntryOnDisk(const HashEntryOnDisk& toBeCopied);
        ~HashEntryOnDisk();

        bool operator== (const HashEntryOnDisk& toBeCompare);
        HashEntryOnDisk& operator= (const HashEntryOnDisk& toBeCopied);

        uint64_t GetHeaderOffsetPhy(){return header_offset.GetHeaderOffset();}
        uint16_t GetDataSize(){return header.GetDataSize();}
        uint32_t GetDataOffsetInSeg(){return header.GetDataOffset();}
        uint32_t GetNextHeadOffsetInSeg(){return header.GetNextHeadOffset();}

    } __attribute__((__packed__));


    class HashEntry
    {
    public:
        HashEntryOnDisk entryOndisk;
        void* pointer;

    public:
        HashEntry();
        HashEntry(HashEntryOnDisk& entry_ondisk, void* read_point);
        HashEntry(const HashEntry&);
        ~HashEntry();
        bool operator== (const HashEntry& toBeCompare);
        HashEntry& operator= (const HashEntry& toBeCopied);

        uint64_t GetHeaderOffsetPhy(){return entryOndisk.GetHeaderOffsetPhy();}
        uint16_t GetDataSize(){return entryOndisk.GetDataSize();}
        uint32_t GetDataOffsetInSeg(){return entryOndisk.GetDataOffsetInSeg();}
        uint32_t GetNextHeadOffsetInSeg(){return entryOndisk.GetNextHeadOffsetInSeg();}
        void* GetReadCachePtr(){return pointer;}

    } __attribute__((__packed__));

    

    class IndexManager{
    public:
        static uint64_t GetIndexSizeOnDevice(uint32_t ht_size);
        bool InitIndexForCreateDB(uint32_t numObjects);

        bool LoadIndexFromDevice(uint64_t offset, uint32_t ht_size);
        bool WriteIndexToDevice(uint64_t offset);

        bool UpdateIndexFromInsert(DataHeader *data_header, Kvdb_Digest *digest, uint32_t header_offset, uint64_t seg_offset);
        bool GetHashEntry(Kvdb_Digest *digest, HashEntry &hash_entry);

        
        uint32_t GetHashTableSize(){return m_size;}

        IndexManager(BlockDevice* bdev);
        ~IndexManager();

    private:
        uint32_t ComputeHashSizeForCreateDB(uint32_t number);
        void CreateListIfNotExist(uint32_t index);

        bool InitHashTable(uint32_t size);
        void DestroyHashTable();

        bool _RebuildHashTable(uint64_t offset);
        bool _RebuildTime(uint64_t offset);
        bool _LoadDataFromDevice(void* data, uint64_t length, uint64_t offset); 
        bool _ConvertHashEntryFromDiskToMem(int* counter, HashEntryOnDisk* entry_ondisk);

        bool _PersistHashTable(uint64_t offset);
        bool _PersistTime(uint64_t offset);
        bool _WriteDataToDevice(void* data, uint64_t length, uint64_t offset);


        LinkedList<HashEntry>** m_hashtable;  
        uint32_t m_size;
        BlockDevice* m_bdev;

        Timing* m_last_timestamp;

    };

}// namespace kvdb
#endif //#ifndef _KV_DB_INDEXMANAGER_H_
