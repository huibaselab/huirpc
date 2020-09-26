/*******************************************************
 *
 * FileName: hsserver.h
 *
 * Author: Tom Hui, tomhui1009@yahoo.com, 8613760232170
 *
 * Create Date: Sat Jun 02 21:32 2018
 *
 * Brief:
 *
 *
 *******************************************************/


#ifndef __HS_SERVER_H__
#define __HS_SERVER_H__


#include <hmessagequeue.h>
#include <huibase.h>
#include <pool.h>
#include <hmutex.h>
#include <map>
#include <hsocket.h>
#include <mysqldata.h>

#include <hconnection.h>

#include <midclient.h>
#include <hredis.h>
#include "serr.h"

using namespace HUIBASE;
using namespace HUIBASE::HMYSQL;
using namespace HUIBASE::NOSQL;

#define API_NAME "apiname"

class CHsServer : public HCTcpSocket, public CThreadPool {
 public:
    typedef std::map<HSTR, CMidClient*> CLIENT_MAP;

 public:
    static constexpr HUINT MAX_BUF_LEN = 1024 * 1024 * 2; // 1M
    static constexpr HINT MAX_ACCEPT_TIME = 2;
    static constexpr HINT MAX_RECEIVE_TIMEOUT = 1;
    static constexpr HINT MAX_SEND_TIMEOUT = 2;

    static constexpr HUINT MAX_LISTEN_THREADS = 8;

    typedef enum {
        BS_UNUSED,
        BS_BUSY
    } BLOCK_STATUS;

    typedef enum {
        MSG_TYPE_FD = 1,
        MSG_TYPE_REQ
    } MSG_TYPE;

    typedef struct _threadShareData_ {
        BLOCK_STATUS status;
        HSYS_T fd;
        HTIME acceptTime;
        HTIME receiveTime;
        HCHAR addrbuf[HLEN_C];
        HUINT addrLen;
        HUINT bufLen;
        HCHAR buf[MAX_BUF_LEN];
    } ThreadShareData;

    typedef std::map<HSYS_T, ThreadShareData*> SERVER_DATA_MAP;

    typedef struct _msg_data_{
        HLONG mtype;
        HSYS_T fd;
        HTIME sendTime;
    } MsgData;

    typedef struct _thread_data {
        HUINT index;
        HUINT fd;
    } ThreadData;

 public:
    CHsServer (HCSTRR strServerName, HCSTRR strIp, HUINT uPort, HUINT uListenLen, HBOOL bAddrReuse,
               HSYS_T msg_key, HUINT uChildLen,  HCSTRR strLockFileName, HUINT block_len = 8192)
        : HCTcpSocket (), CThreadPool (uChildLen, strLockFileName), m_strServerName(strServerName),
        m_strListenIp(strIp), m_uPort(uPort), m_uListenLen(uListenLen), m_bAddrReuse (bAddrReuse),
        m_msg_key(msg_key), m_block_count (block_len) {

        m_uChildCount = uChildLen;


        }

    ~ CHsServer();


    HRET MeInit () throw ();

    HRET UnInit () throw ();

    void SetStop () { m_running = false; }

    HRET QueryDB (HCSTRR strName, HCSTRR sql, CRes& db_res ) throw ();

    HRET ExecDB (HCSTRR strName, HCSTRR sql) throw ();

    HRET MultiExec (HCSTRR strName, HCVSTRSR sqls) throw ();

    CRedis* GetMem (HCSTRR strName) throw ();

    HRET CallApi (HCSTRR strName, HCSTRR strApiName, const HCParam& inp, HCParam& outp) throw ();

    void SetBlack (HCSTRR str);

    void SetWhiteList (HCSTRR str);

    HCSTRR GetErrorMapFile () const { return m_strErrorMapFile; }
    
    void SetErrorMapFile (HCSTRR strFileMapFile) { m_strErrorMapFile = strFileMapFile; }

    HCSTRR GetPassword () const { return m_strPassword; }

    void SetPassword (HCSTRR strPassword) { m_strPassword = strPassword; }

 protected:
    ThreadShareData* getBlock (HSYS_T fd) throw ();

    ThreadShareData* getTheData (HSYS_T fd) throw ();

    void releaseBlock (HSYS_T fd) throw ();

    void threadAcceptRun (HUINT uChildIndex);

    void threadReceiveRun (HUINT uChildIndex);

    void threadHandleRun (HUINT uChildIndex);

    void childRun (HUINT uChildIndex);

    HBOOL isBlackAddr (HCSTRR strAddr) const;


 private:
    HRET initErrorMap () throw ();

    HRET initBuffer () throw ();

    HRET initSocket () throw ();

    HRET initMq () throw ();

    HRET initPool () throw ();

    HRET initDbConn () throw ();

    void uninitBuffer () throw ();

    void uninitSocket () throw ();

    void uninitMq () throw ();

    HRET readPack (ThreadShareData* pData) throw ();

    HRET doWork () throw ();

    HBOOL isInWhiteList (HCSTRR strIp) const;

    void middleEncode(HCSTRR src, HSTRR dst);

    void middleDecode(HCSTRR src, HSTRR dst);

 private:
    HSTR m_strServerName;

    HSTR m_strListenIp;

    HUINT m_uPort;

    HUINT m_uListenLen;

    HBOOL m_bAddrReuse;

    HUINT m_uChildCount;

    HSYS_T m_msg_key;

    HUINT m_block_count = 8192;

    HVSTRS m_blacklist;

    HVSTRS m_whiteList;

    SERVER_DATA_MAP m_buffer_map;

    HCMessageQueue m_mq;

    volatile bool m_running = true;

    CLIENT_MAP m_clients;

    HSTR m_strErrorMapFile;

    HSTR m_strPassword;
};



#endif //__HS_SERVER_H__
