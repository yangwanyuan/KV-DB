#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>


#include "IndexManager.h"


namespace kvdb{

    DataHeader::DataHeader() : data_size(0), data_offset(0), next_header_offset(0)
    {
        //memset(&key_digest, 0, sizeof(Kvdb_Digest));
        ;
    }

    DataHeader::DataHeader(Kvdb_Digest &digest, uint16_t size, uint32_t offset, uint32_t next_offset)
    {
        key_digest = digest;
        data_size = size;
        data_offset = offset;
        next_header_offset = next_offset;
    }

    DataHeader::DataHeader(const DataHeader& toBeCopied)
    {
        key_digest = toBeCopied.key_digest;
        data_size = toBeCopied.data_size;
        data_offset = toBeCopied.data_offset;
        next_header_offset = toBeCopied.next_header_offset;
    }

    DataHeader::~DataHeader()
    {
        return;
    }

    DataHeader& DataHeader::operator=(const DataHeader& toBeCopied)
    {
        key_digest = toBeCopied.key_digest;
        data_size = toBeCopied.data_size;
        data_offset = toBeCopied.data_offset;
        next_header_offset = toBeCopied.next_header_offset;
        return *this;
    }


    DataHeaderOffset::DataHeaderOffset(uint32_t offset)
    {
        physical_offset = offset;
    }

    DataHeaderOffset::DataHeaderOffset(const DataHeaderOffset& toBeCopied)
    {
        physical_offset = toBeCopied.physical_offset;
    }

    DataHeaderOffset::~DataHeaderOffset()
    {
        return;
    }

    DataHeaderOffset& DataHeaderOffset::operator=(const DataHeaderOffset& toBeCopied)
    {
        physical_offset = toBeCopied.physical_offset;
        return *this;
    }

    HashEntryOnDisk::HashEntryOnDisk()
    {
        ;
    }

    HashEntryOnDisk::HashEntryOnDisk(DataHeader& dataheader, DataHeaderOffset& offset)
    {
        header = dataheader;
        header_offset = offset;
        return;
    }

    HashEntryOnDisk::HashEntryOnDisk(const HashEntryOnDisk& toBeCopied)
    {
        header = toBeCopied.header;
        header_offset = toBeCopied.header_offset;
    }

    HashEntryOnDisk::~HashEntryOnDisk()
    {
        ;
    }

    bool HashEntryOnDisk::operator== (const HashEntryOnDisk& toBeCompare)
    {
        return true;
    }

    HashEntryOnDisk& HashEntryOnDisk::operator= (const HashEntryOnDisk& toBeCopied)
    {
        header = toBeCopied.header;
        header_offset = toBeCopied.header_offset;
        return *this;
    }

    HashEntry::HashEntry()
    {
        pointer = NULL;
        return;
    }

    HashEntry::HashEntry(HashEntryOnDisk& entry_ondisk, void* read_ptr)
    {
        entryOndisk = entry_ondisk;
        pointer = read_ptr;
    }

    HashEntry::HashEntry(const HashEntry& toBeCopied)
    {
        entryOndisk = toBeCopied.entryOndisk;
        pointer = toBeCopied.pointer;
    }

    HashEntry::~HashEntry()
    {
        pointer = NULL;
        return;
    }

    bool HashEntry::operator==(const HashEntry& toBeCompare)
    {
        if (!memcmp(&(entryOndisk.header.key_digest), &(toBeCompare.entryOndisk.header.key_digest), sizeof(Kvdb_Digest) ))
        {
            return true;
        }
        return false;
    }

    HashEntry& HashEntry::operator=(const HashEntry& toBeCopied)
    {
        entryOndisk = toBeCopied.entryOndisk;
        pointer = toBeCopied.pointer;
        return *this;
    }

    bool IndexManager::InitIndexForCreateDB(uint64_t numObjects)
    {
        m_size = ComputeHashSizeForCreateDB(numObjects);
        
        if (!InitHashTable(m_size))
        {
            return false;
        }

        return true;
    }

    bool IndexManager::LoadIndexFromDevice(uint64_t offset, uint64_t ht_size)
    {
        m_size = ht_size;

        //Read timestamp from device
        ssize_t timeLength = Timing::GetTimeSizeOf();
        if (!_RebuildTime(offset))
        {
            return false;
        }
        __DEBUG("Load Hashtable timestamp: %s", Timing::TimeToChar(*m_last_timestamp));
        offset += timeLength;
        
        if (!_RebuildHashTable(offset))
        {
            return false;
        }
        __DEBUG("Rebuild Hashtable Success");

        return true;
    }

    bool IndexManager::_RebuildTime(uint64_t offset)
    {
        ssize_t timeLength = Timing::GetTimeSizeOf();
        time_t _time;
        if (m_bdev->pRead(&_time, timeLength, offset) != timeLength)
        {
            perror("Error in reading timestamp from file\n");
            return false;
        }
        m_last_timestamp->SetTime(_time);
        return true;
    }

    bool IndexManager::WriteIndexToDevice(uint64_t offset)
    {
        //Write timestamp to device
        ssize_t timeLength = Timing::GetTimeSizeOf();
        if (!_PersistTime(offset))
        {
            return false;
        }
        __DEBUG("Write Hashtable timestamp: %s", Timing::TimeToChar(*m_last_timestamp));
        offset += timeLength;


        if (!_PersistHashTable(offset))
        {
            return false;
        }
        __DEBUG("Persist Hashtable Success");
    
        return true;
    }

    bool IndexManager::_PersistTime(uint64_t offset)
    {
        ssize_t timeLength = Timing::GetTimeSizeOf();
        m_last_timestamp->Update();
        time_t _time =m_last_timestamp->GetTime();

        if (m_bdev->pWrite((void *)&_time, timeLength, offset ) != timeLength)
        {
            perror("Error write timestamp to file\n");
            return false;
        }
        return true;
    }

    bool IndexManager::UpdateIndexFromInsert(DataHeader *data_header, Kvdb_Digest *digest, uint32_t header_offset)
    {
        HashEntryOnDisk entry_ondisk;
        entry_ondisk.header = *data_header;
        entry_ondisk.header_offset.physical_offset = header_offset;

        HashEntry entry(entry_ondisk, NULL);

        uint32_t hash_index = KeyDigestHandle::Hash(digest) % m_size;

        CreateListIfNotExist(hash_index);

        LinkedList<HashEntry> *entry_list = m_hashtable[hash_index];
        if (!entry_list->search(entry))
        {
            entry_list->insert(entry);
        }
        else
        {
            entry_list->remove(entry);
            entry_list->insert(entry);
        }
        return true; 
    }

    bool IndexManager::GetHashEntry(Kvdb_Digest *digest, HashEntry &entry)
    {
        uint32_t hash_index = KeyDigestHandle::Hash(digest) % m_size;

        CreateListIfNotExist(hash_index);

        LinkedList<HashEntry> *entry_list = m_hashtable[hash_index];

        entry.entryOndisk.header.key_digest = *digest;
        
        if (entry_list->search(entry))
        {
             vector<HashEntry> tmp_vec = entry_list->get();
             for (vector<HashEntry>::iterator iter = tmp_vec.begin(); iter!=tmp_vec.end(); iter++)
             {
                 if (iter->entryOndisk.header.key_digest == entry.entryOndisk.header.key_digest)
                 {
                     entry = *iter;
                     return true;
                 }
             }
        }
        return false;
    }

    uint64_t IndexManager::GetIndexSizeOnDevice(uint64_t ht_size)
    {
        uint64_t index_size = sizeof(time_t) + sizeof(HashEntryOnDisk) * ht_size;
        uint64_t index_size_pages = index_size / getpagesize();
        return (index_size_pages + 1) * getpagesize();
    }


    IndexManager::IndexManager(BlockDevice* bdev):
        m_hashtable(NULL), m_size(0), m_bdev(bdev)
    {
        m_last_timestamp = new Timing();
        return ;
    }

    IndexManager::~IndexManager()
    {
        if (m_last_timestamp)
        {
            delete m_last_timestamp;
        }
        if (m_hashtable)
        {
            DestroyHashTable();
        }
    }

    uint32_t IndexManager::ComputeHashSizeForCreateDB(uint32_t number)
    {
        // Gets the next highest power of 2 larger than number
        number--;
        number = (number >> 1) | number;
        number = (number >> 2) | number;
        number = (number >> 4) | number;
        number = (number >> 8) | number;
        number = (number >> 16) | number;
        number++;
        return number;
    }

    void IndexManager::CreateListIfNotExist(int index)
    {
        if (!m_hashtable[index])
        {
            LinkedList<HashEntry> *entry_list = new LinkedList<HashEntry>;
            m_hashtable[index] = entry_list;
        }
        return;
    }

    bool IndexManager::InitHashTable(int size)
    {
        m_hashtable =  new LinkedList<HashEntry>*[m_size];

        for (int i = 0; i < size; i++)
        {
            m_hashtable[i]=NULL;
        }
        return true;
    }

    void IndexManager::DestroyHashTable()
    {
        for (int i = 0; i < m_size; i++)
        {
            if (m_hashtable[i])
            {
                delete m_hashtable[i];
                m_hashtable[i] = NULL;
            }
        }
        delete[] m_hashtable;
        m_hashtable = NULL;
        return;
    }

    bool IndexManager::_RebuildHashTable(uint64_t offset)
    {
        //Init hashtable
        if (!InitHashTable(m_size))
        {
            return false;
        }
        
        __DEBUG("InitHashTable success");

        //Read hashtable 
        uint64_t table_length = sizeof(int) * m_size;
        int* counter = new int[m_size];
        if (!_LoadDataFromDevice((void*)counter, table_length, offset))
        {
            return false;
        }
        offset += table_length;
        __DEBUG("Read hashtable success");

        //Read all hash_entry
        uint64_t length = sizeof(HashEntryOnDisk) * m_size;
        HashEntryOnDisk *entry_ondisk = new HashEntryOnDisk[m_size];
        if (!_LoadDataFromDevice((void*)entry_ondisk, length, offset))
        {
            return false;
        }
        offset += length;
        __DEBUG("Read all hash_entry success");
       
        //Convert hashtable from device to memory
        if (!_ConvertHashEntryFromDiskToMem(counter, entry_ondisk))
        {
            return false;
        }
        __DEBUG("rebuild hash_table success");

        delete[] entry_ondisk;
        delete[] counter;

        return true; 
    }

    bool IndexManager::_LoadDataFromDevice(void* data, uint64_t length, uint64_t offset)
    {
        ssize_t nread;
        uint64_t h_offset = 0;
        while ((nread = m_bdev->pRead((uint64_t *)data + h_offset, length, offset)) > 0){
            length -= nread;
            offset += nread;
            h_offset += nread;
            if (nread < 0)
            {
                perror("Error in reading hashtable from file\n");
                return false;
            }
        }
        return true;
    }

    bool IndexManager::_WriteDataToDevice(void* data, uint64_t length, uint64_t offset)
    {
        ssize_t nwrite;
        uint64_t h_offset = 0;
        while ((nwrite = m_bdev->pWrite((uint64_t *)data + h_offset, length, offset)) > 0){
            length -= nwrite;
            offset += nwrite;
            h_offset += nwrite;
            if (nwrite < 0)
            {
                perror("Error in reading hashtable from file\n");
                return false;
            }
        }
        return true;
    }

    bool IndexManager::_ConvertHashEntryFromDiskToMem(int* counter, HashEntryOnDisk* entry_ondisk)
    {
        //Convert hashtable from device to memory
        int entry_index = 0;
        for (int i = 0; i < m_size; i++)
        {
            
            int entry_num = counter[i];
            for (int j = 0; j < entry_num; j++)
            {
                HashEntry entry(entry_ondisk[entry_index], 0);

                CreateListIfNotExist(i);
                m_hashtable[i]->insert(entry);
                entry_index++;
            }
            if (entry_num > 0)
            {
                __DEBUG("read hashtable[%d]=%d", i, entry_num);
            }
        }
        return true;
    }

    bool IndexManager::_PersistHashTable(uint64_t offset)
    {
        uint64_t entry_total = 0;

        //write hashtable to device
        uint64_t table_length = sizeof(int) * m_size;
        int* counter = new int[m_size];
        for (int i = 0; i < m_size; i++)
        {
            counter[i] = (m_hashtable[i]? m_hashtable[i]->get_size(): 0);
            entry_total += counter[i];
        }
        if (!_WriteDataToDevice((void*)counter, table_length, offset))
        {
            return false;
        }
        offset += table_length;
        delete[] counter;
        __DEBUG("write hashtable to device success");


        //write hash entry to device
        uint64_t length = sizeof(HashEntryOnDisk) * entry_total;
        HashEntryOnDisk *entry_ondisk = new HashEntryOnDisk[entry_total];
        int entry_index = 0;
        for (int i = 0; i < m_size; i++)
        {
            if (!m_hashtable[i])
            {
                continue;
            }
            vector<HashEntry> tmp_vec = m_hashtable[i]->get();
            for (vector<HashEntry>::iterator iter = tmp_vec.begin(); iter!=tmp_vec.end(); iter++)
            {
                entry_ondisk[entry_index++] = (iter->entryOndisk);
            }
        }

        if (!_WriteDataToDevice((void *)entry_ondisk, length, offset))
        {
            return false;
        }
        delete[] entry_ondisk;
        __DEBUG("write all hash entry to device success");

        return true;
    
    }
}

