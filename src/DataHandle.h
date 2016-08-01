#ifndef _KV_DB_DATAHANDLE_H_
#define _KV_DB_DATAHANDLE_H_

#include <string>
#include <sys/types.h>
#include "Db_Structure.h"
#include "BlockDevice.h"
#include "KeyDigestHandle.h"

using namespace std;

namespace kvdb{
    class DataHandle{
    public:
        bool ReadDataHeader(off_t offset, DataHeader &data_header, string &key);
        bool WriteDataHeader();
        bool ReadData(DataHeader* data_header, string &data);
        bool WriteData(DataHeader* data_header, const char* data, uint32_t length, off_t offset);
        bool DeleteData(const char* key, uint32_t key_len, off_t offset); 

        DataHandle(BlockDevice* bdev);
        ~DataHandle();
    private:
        BlockDevice* m_bdev;
    };
}


#endif //#ifndef _KV_DB_DATAHANDLE_H_