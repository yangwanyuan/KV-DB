#ifndef _KV_DB_DATAHANDLE_H_
#define _KV_DB_DATAHANDLE_H_

#include <string>
#include <sys/types.h>

#include <list>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "KeyDigestHandle.h"
#include "IndexManager.h"
#include "SuperBlockManager.h"
#include "SegmentManager.h"

using namespace std;

namespace kvdb{

    class DataHandle{
    public:
        bool ReadData(HashEntry* entry, string &data);
        bool WriteData(Kvdb_Digest& digest, const char* data, uint16_t length);

        DataHandle(BlockDevice* bdev, SuperBlockManager* sbm, IndexManager* im, SegmentManager* sm);
        ~DataHandle();
    private:
        BlockDevice* m_bdev;
        SuperBlockManager* m_sbm;
        IndexManager* m_im;
        SegmentManager* m_sm;
    };

    class SegmentSlice{
    public:
        SegmentSlice();
        ~SegmentSlice();
        SegmentSlice(const SegmentSlice& toCopied);
        SegmentSlice& operator=(const SegmentSlice& toCopied);

        SegmentSlice(uint32_t seg_id, SegmentManager* sm);

        bool Put(DataHeader& header, const char* data, uint16_t length);
        const void* GetSlice() const {return m_data;}
        uint32_t GetLength() const {return m_len;}
    private:
        uint32_t m_id;
        SegmentManager* m_sm;
        uint32_t m_seg_size;
        char* m_data;
        uint32_t m_len;
    };

    class KVSlice{
    public:
        KVSlice();
        ~KVSlice();
        KVSlice(const KVSlice& toCopied);
        KVSlice& operator=(const KVSlice& toCopied);

        KVSlice(const char* key, int key_len, const char* data, int data_len);

        const Kvdb_Digest& GetDigest() const {return *m_digest;}
        const char* GetKey() const {return m_key;}
        const char* GetData() const {return m_data;}
        string GetKeyStr() const;
        string GetDataStr() const;
        int GetKeyLen() const {return m_keyLen;}
        int GetDataLen() const {return m_dataLen;}
        bool IsDigestComputed() const {return m_Iscomputed;}

        void SetKeyValue(const char* key, int key_len, const char* data, int data_len);
        bool ComputeDigest();

    private:
        char* m_key;
        uint32_t m_keyLen;
        char* m_data;
        uint16_t m_dataLen;
        Kvdb_Digest *m_digest;
        bool m_Iscomputed;

        void copy_helper(const KVSlice& toCopied);
    };

    class Request{
    public:
        Request();
        ~Request();
        Request(const Request& toCopied);
        Request& operator=(const Request& toCopied);
        Request(KVSlice& slice);

        const KVSlice& GetSlice() const { return *m_slice; }
        int IsDone() const { return m_done; }
        void Done() { m_done = 1; }

        void SetState(bool state) { m_write_stat = state; }
        bool GetState() const {return m_write_stat; }

    private:
        int m_done;
        bool m_write_stat;
        KVSlice *m_slice;

    };


} //end namespace kvdb


#endif //#ifndef _KV_DB_DATAHANDLE_H_
