
#include <hmutex.h>
#include <hlog.h>
#include <haddr.h>
#include <hdict.h>
#include <hsocket.h>
#include "hsserver.h"
#include "apibase.h"
#include "connmap.h"

#include <hlog.h>
#include <libconfig.h++>

#include <hcrypto.h>

CHsServer::~CHsServer() {

    UnInit();


    for (CLIENT_MAP::iterator it = m_clients.begin(); it != m_clients.end(); ++it) {

        CMidClient* p = it->second;

        delete p;

    }

    m_clients.clear();

}


HRET CHsServer::MeInit() throw () {

    HASSERT_THROW_MSG(HIS_OK(initErrorMap()), "CHsServer Init Error Map failed", UN_INIT);

    HASSERT_THROW_MSG(HIS_OK(initBuffer()), "CHsServer Init buffer failed", UN_INIT);

    HASSERT_THROW_MSG(HIS_OK(initDbConn()), "CHsServer Init db connection failed", UN_INIT);

    HASSERT_THROW_MSG(HIS_OK(initSocket()), "CHsServer Init socket failed", UN_INIT);

    HASSERT_THROW_MSG(HIS_OK(initMq()), "CHsServer Init mq failed", UN_INIT);

    HASSERT_THROW_MSG(HIS_OK(initPool()), "CHsServer Init pool failed", UN_INIT);

    HRETURN_OK;
}


HRET CHsServer::UnInit() throw () {

    uninitMq();

    uninitSocket();

    uninitBuffer();

    HRETURN_OK;
}


CHsServer::ThreadShareData* CHsServer::getBlock(HSYS_T fd) throw () {

    ThreadShareData* p = nullptr;
    SERVER_DATA_MAP::iterator fit = m_buffer_map.find(fd);
    if (fit == m_buffer_map.end()) {

        p = (ThreadShareData*) malloc(sizeof(ThreadShareData));
        CHECK_NEWPOINT(p);
        memset(p, 0, sizeof(ThreadShareData));
        p->status = BS_BUSY;
        m_buffer_map[fd] = p; 

    } else {

        p = fit->second;
        HASSERT_THROW(p->status == BS_UNUSED, ERR_STATUS);
        p->status = BS_BUSY;
        p->bufLen = 0;

    }

    return p;

}


CHsServer::ThreadShareData* CHsServer::getTheData (HSYS_T fd) throw () {

    ThreadShareData* p = nullptr;
    SERVER_DATA_MAP::iterator fit = m_buffer_map.find(fd);
    if (fit != m_buffer_map.end()) {
        p = fit->second;
    }

    return p;
}


void CHsServer::releaseBlock(HSYS_T fd) throw () {

    ThreadShareData* p = getTheData(fd);
    HASSERT_THROW(p != nullptr, ERR_STATUS);

    p->status = BS_UNUSED;

}


void CHsServer::threadAcceptRun(HUINT uChildIndex) {

    SLOG_NORMAL("[%d] accept run...", uChildIndex);

    MsgData msg_data {0, 0, 0};
    HCTcpSocket client;
    HCIp4Addr clientAddr;
    HSTR strClientIp;

    while (m_running) {

        try {

            HASSERT_THROW_MSG(HIS_OK(Accept(client, clientAddr)), "server accept failed", NET_REQ);

            IF_FALSE(client.IsGoodSocket()) {
                SLOG_WS("Get an invalid client socket");
                continue;
            }

            client.SetReuseAddr ();
            client.SetNonblocked ();

            strClientIp = clientAddr.ToString();

            //SLOG_NORMAL("get a client connection [%s]", strClientIp.c_str());

            IF_TRUE(isBlackAddr(strClientIp)) {

                SLOG_WARNING("[%s] is in black list", strClientIp.c_str());
                client.Close();
                continue;

            }

            IF_FALSE(isInWhiteList(strClientIp)) {

                SLOG_WARNING("[%s] is not in white list", strClientIp.c_str());
                client.Close();
                continue;

            }

            ThreadShareData* pData = getBlock(client.GetSocket());
            HASSERT_THROW(pData != nullptr, ERR_STATUS);

            SLOG_NORMAL("Get a new connection from [%s] fd[%d]", strClientIp.c_str(), pData->fd);

            msg_data.fd = client.GetSocket();
            msg_data.mtype = MSG_TYPE_FD;

            pData->fd = client.GetSocket();
            pData->acceptTime = time(nullptr);

            memset(pData->addrbuf, 0, HLEN_C);
            memcpy(pData->addrbuf, strClientIp.c_str(), strClientIp.length());
            pData->addrLen = strClientIp.length();

            memset(pData->buf, 0, MAX_BUF_LEN);
            pData->bufLen = MAX_BUF_LEN;

            msg_data.sendTime = time(nullptr);

            HASSERT_THROW_MSG(HIS_OK(m_mq.Send(&msg_data, sizeof(msg_data), 0)), "accept thread send mq failed", SYS_FAILED);

        } catch (HCException& ex) {

            SLOG_ERROR("thread accept get an exception. [%s]", ex.what());

        } catch (...) {

            SLOG_ES("thread accept get an unkown exception");

        }


    }

}


void CHsServer::threadReceiveRun(HUINT uChildIndex) {

    SLOG_NORMAL("[%d] receive run...", uChildIndex);

    MsgData msg_data;
    while (m_running) {

        try {

            HSIZE size = sizeof(msg_data);
            m_mq.Recv(&msg_data, size, MSG_TYPE_FD, 0);

            //LOG_NORMAL("index: [%d]", msg_data.index);
            
            ThreadShareData* pData = getTheData(msg_data.fd);
            if (pData == nullptr) {

                SLOG_ES("CHsServer msg receive a null share data");

                releaseBlock(msg_data.fd);
                continue;

            }

            if (abs((HINT)time(nullptr) - (HINT)pData->acceptTime) > MAX_ACCEPT_TIME) {

                SLOG_ERROR("socket: [%d], accept time: [%d], receive-now: [%d]", msg_data.fd, pData->acceptTime, time(nullptr));

                HCTcpSocket _s(pData->fd);
                _s.Close();

                releaseBlock(pData->fd);

                continue;

            }

            // allow connect control.

            // receive package.
            HIF_NOTOK(readPack(pData)) {

                SLOG_ES("CHsServer receive socket failed");

                releaseBlock(msg_data.fd);
                HCTcpSocket _s(pData->fd);
                _s.Close();

                continue;
            }

            SLOG_NORMAL("receive %u from fd[%d]", pData->bufLen, msg_data.fd);

            // set receive time.
            pData->receiveTime = time(nullptr);

            // send to msg.
            msg_data.mtype = MSG_TYPE_REQ;
            msg_data.sendTime = time(nullptr);

            HASSERT_THROW_MSG(HIS_OK(m_mq.Send(&msg_data, sizeof(msg_data), 0)), "receive thread send mq failed", SYS_FAILED);


        } catch (const HCException& ex) {

            SLOG_ERROR("get an base exception: [%s]", ex.what());

            releaseBlock(msg_data.fd);

        } catch (...) {

            SLOG_ES("receive thread catch an unkown exception");

            releaseBlock(msg_data.fd);

        }

    }

}


void CHsServer::threadHandleRun (HUINT uChildIndex) {

    SLOG_NORMAL("[%d] handle run...", uChildIndex);

    MsgData msg_data;
    while (m_running) {

        try {

            SetChildStatus (uChildIndex, IDLE);

            HSIZE size = sizeof(msg_data);
            m_mq.Recv(&msg_data, size, MSG_TYPE_REQ, 0);

            SetChildStatus (uChildIndex, BUSY);

            ThreadData* ptd = (ThreadData*)CreateThreadSpecific(sizeof(ThreadData));
            ptd->index = uChildIndex;
            ptd->fd = msg_data.fd;

            doWork();

        } catch (const HCException& ex) {
            
            SLOG_ERROR("get a huibase exception [%s]", ex.what());

        } catch (...) {

            SLOG_ES("handle thread catch an unkown exception");

        }

        releaseBlock(msg_data.fd);

    }

}


void CHsServer::childRun(HUINT uChildIndex ) {

    SetChildStatus (uChildIndex, BUSY);

    if (uChildIndex == 0) {

        threadAcceptRun (uChildIndex);

    } else if (uChildIndex > 0 && uChildIndex <= MAX_LISTEN_THREADS) {

        threadReceiveRun (uChildIndex);

    } else {

        threadHandleRun(uChildIndex);

    }

}


HBOOL CHsServer::isBlackAddr (HCSTRR strAddr) const {

    for (size_t i = 0; i < m_blacklist.size(); ++i) {
        HCSTRR item = m_blacklist[i];
        if (strAddr.find(item) == 0) {
            return HTRUE;
        }
    }

    return HFALSE;
}


HRET CHsServer::initErrorMap () throw () {

    return ERROR_INFO_MAP::Instance()->ReadErrorFromFile(m_strErrorMapFile);

}


HRET CHsServer::initBuffer() throw () {

    for (HUINT i = 0; i < m_block_count; ++i) {

        ThreadShareData* p = (ThreadShareData*) malloc(sizeof(ThreadShareData));
        CHECK_NEWPOINT(p);

        memset(p, 0, sizeof(ThreadShareData));

        p->status = BS_UNUSED;

        m_buffer_map[i] = p;    

    }

    HRETURN_OK;
}


HRET CHsServer::initSocket() throw () {

    HNOTOK_RETURN(HCTcpSocket::Init());

    IF_TRUE(m_bAddrReuse) {

        HNOTOK_RETURN(HCTcpSocket::SetReuseAddr ());

    }

    HCIp4Addr addr(m_strListenIp, m_uPort);

    HNOTOK_RETURN(Bind(addr));

    HNOTOK_RETURN(Listen(m_uListenLen));

    HRETURN_OK;
}


HRET CHsServer::initMq() throw () {    

    m_mq.Init (m_msg_key, 0777);

    m_mq.Create();

    HNOTOK_RETURN(m_mq.Open());

    HRETURN_OK;

}


HRET CHsServer::initPool() throw () {

    CThreadPool::Init ();

    HRETURN_OK;
}


HRET CHsServer::initDbConn() throw () {

    using namespace libconfig;
    Config cfg;

    try {

        cfg.readFile("../conf/conn.cfg");

    } catch (const FileIOException& fiex) {

        SLOG_ERROR("I/O error while read file. msg: [%s]", fiex.what());

        HRETURN(IO_ERR);

    } catch (const ParseException& pex ) {

        SLOG_ERROR("Parse config file failed at %s:%d--%s", pex.getFile(),
                  pex.getLine(), pex.getError());

        HRETURN(INVL_RES);

    }

    HUINT db_count = m_uChildCount - MAX_LISTEN_THREADS - 1;
    SLOG_NORMAL("init db connection count: [%d]", db_count);

    {

        const Setting& root = cfg.getRoot ();

        if (root.exists("dbs")) {

            const Setting& dbs = root["dbs"];

            for (HINT i = 0; i < dbs.getLength(); ++i) {

                HSTR str, strName;
                CONN_INFO ci;

                dbs[i].lookupValue("name", strName);
                dbs[i].lookupValue("dbname", ci.m_db);
                dbs[i].lookupValue("ip", ci.m_host);
                dbs[i].lookupValue("port", str);
                ci.m_port = HCStr::stoi(str);

                dbs[i].lookupValue("user", ci.m_user);
                dbs[i].lookupValue("pass", ci.m_pw);

                HUINT k = 0;
                for (HUINT j = 0; j < db_count; ++j) {

                    k = j + MAX_LISTEN_THREADS + 1;

                    CMyConnection* p = new CMyConnection ();
                    CHECK_NEWPOINT(p);

                    HASSERT_THROW_MSG(HIS_OK(p->Connect(ci)), "initDbConn connect to db failed", DB_DISCONN);
                    p->SetUtf8 ();

                    SLOG_NORMAL("[%d][%s][%s][%s][%s][%p]", k, strName.c_str(), ci.m_db.c_str(), ci.m_host.c_str(), str.c_str(), p);

                    DB_MAP::Instance()->AddConn(strName, k, p);

                }

            }

        }

    }


    {

        const Setting& root = cfg.getRoot ();

        if (root.exists("rds")) {

            const Setting& dbs = root["rds"];

            for (HINT i = 0; i < dbs.getLength(); ++i) {

                HSTR str;
                NoSqlConnectionInfo ci;

                dbs[i].lookupValue("name", ci.strName);
                dbs[i].lookupValue("ip", ci.strIp);
                dbs[i].lookupValue("port", str);
                ci.nPort = HCStr::stoi(str);

                HUINT k = 0;
                for (HUINT j = 0; j < db_count; ++j) {

                    k = j + MAX_LISTEN_THREADS + 1;

                    CRedis* p = new CRedis(ci);
                    CHECK_NEWPOINT(p);

                    HASSERT_THROW_MSG(HIS_OK(p->Init()), "initConn connect to redis failed", RDS_ERR);

                    SLOG_NORMAL("[%d][%s][%s][%d]", k, ci.strName.c_str(), ci.strIp.c_str(), ci.nPort);

                    DB_MAP::Instance()->AddMem(ci.strName, k, p);

                }
            }
        }

    }


    {

        const Setting& root = cfg.getRoot ();

        if (root.exists("cls")){

            const Setting& dbs = root["cls"];

            for (HINT i = 0; i < dbs.getLength(); ++i) {

                HSTR strName, strIp, strPort, strTimeout, strPassword;
                HUINT nPort = 0, nTimeout = 0;

                dbs[i].lookupValue("name", strName);
                dbs[i].lookupValue("ip", strIp);
                dbs[i].lookupValue("port", strPort);
                dbs[i].lookupValue("timeout", strTimeout);

                if (dbs[i].exists("password")) {
                    dbs[i].lookupValue("password", strPassword);
                }

                nPort = HCStr::stoi(strPort);
                nTimeout = HCStr::stoi(strTimeout);

                SLOG_NORMAL("[%d][%s][%s]", i, strName.c_str(), strIp.c_str(), strPort.c_str());

                CMidClient* p = new CMidClient(strIp, nPort, nTimeout);
                CHECK_NEWPOINT(p);
                p->SetPassword(strPassword);

                m_clients.insert(CLIENT_MAP::value_type(strName, p));

            }

        }

    }

    HRETURN_OK;
}


void CHsServer::uninitBuffer () throw () {

    for (SERVER_DATA_MAP::iterator it = m_buffer_map.begin(); it != m_buffer_map.end(); ++it) {

        ThreadShareData* p = it->second;

        free(p);

    }

}


void CHsServer::uninitSocket() throw () {

    HCTcpSocket::Close ();

}


void CHsServer::uninitMq() throw () {

    m_mq.Close ();

    m_mq.Remove ();

}


HRET CHsServer::readPack(ThreadShareData* pData) throw () {

    HCTcpSocket sock(pData->fd);
    HRET cb = 0;
    HINT npos = 0;

    // add a req_len.
    HINT req_len = 0;
    cb = sock.ReadWithTimeOut(&req_len, sizeof(req_len), MAX_RECEIVE_TIMEOUT);

    if (cb != sizeof(req_len) && req_len < 3) {

        SLOG_ERROR("sock[%d] get len return[%d], len[%d]", sock.GetSocket(), cb, req_len);
        HRETURN(INVL_RES);

    }

    do {

        cb = sock.ReadWithTimeOut(pData->buf + npos, pData->bufLen - npos,  MAX_RECEIVE_TIMEOUT);

        if (cb < 0) {

            SLOG_ERROR("sock[%d] cb: [%d]", sock.GetSocket(), cb);
            HRETURN(INVL_RES);
        }

        npos += cb;

    } while (npos < req_len && cb > 0);

    pData->bufLen = req_len;

    sock.Invalid();
    HRETURN_OK;

}


HRET CHsServer::doWork() throw () {

    ThreadData* ptd = (ThreadData*)CreateThreadSpecific(sizeof(ThreadData));

    HCTcpSocket _s(ptd->fd);

    HCParam ps;

    do {

        ThreadShareData* pData = getTheData(ptd->fd);
        HSTR srcReq(pData->buf), strReq;

        // decode
        middleDecode(srcReq, strReq);

        HCStr::Trim(strReq);
        SLOG_NORMAL("thread[%d] get request[%s]", ptd->index, strReq.c_str());

        HIF_NOTOK(ps.SetParam(strReq, "&", "=")) {

            SLOG_ERROR("parse request[%s] failed", strReq.c_str());
            break;

        }

        if (ps.count(API_NAME) == 0) {

            SLOG_ERROR("no api name, request [%s] error", strReq.c_str());
            break;
        }

        HSTR strApiName = ps[API_NAME];

        CApiBase* api = maker_facetory::Instance()->newApi(strApiName);
        if (api == nullptr) {

            SLOG_ERROR("not this apiname[%s]", strApiName.c_str());
            break;

        }

        api->SetServer(this);

        shared_ptr<CApiBase> ms(api);

        HSTR str, dst_out;
        try {

            api->Init(ps);
            api->Work();

            str = api->GetRes();

        } catch (const CMiddleException& ex) {

            str = HCStr::Format("err_no=%s&err_msg=%s", ex.GetErrorNum().c_str(), ex.what());
            SLOG_ERROR("[%s] catch an exception[%s]", strApiName.c_str(), ex.what());

        } catch (const HCException& ex) {

            str = "err_no=-1&err_msg=exception in api";
            SLOG_ERROR("[%s] catch an exception[%s]", strApiName.c_str(), ex.what());

        }

        SLOG_NORMAL("thread[%d] response: [%s]", ptd->index, str.c_str());

        middleEncode(str, dst_out);
        _s.WriteAll(dst_out.c_str(), dst_out.length(), MAX_SEND_TIMEOUT);


    } while (0);

    _s.Close();

    HRETURN_OK;
}


HRET CHsServer::QueryDB (HCSTRR strName, HCSTRR sql, CRes& db_res ) throw () {

    ThreadData* ptd = (ThreadData*)CreateThreadSpecific(sizeof(ThreadData));

    HUINT index = ptd->index;

    return DB_MAP::Instance()->Query(strName, index, sql, db_res);

}

HRET CHsServer::ExecDB (HCSTRR strName, HCSTRR sql) throw () {
    //HFUN_BEGIN;

    ThreadData* ptd = (ThreadData*)CreateThreadSpecific(sizeof(ThreadData));

    //HUINT index = (ptd->index - MAX_LISTEN_THREADS) / DB_CONN_COUNT;
    HUINT index = ptd->index;
    //SLOG_NORMAL("thread index: [%d]", index);


    return DB_MAP::Instance()->Exec(strName, index, sql);

}


HRET CHsServer::MultiExec (HCSTRR strName, HCVSTRSR sqls) throw () {
    ThreadData* ptd = (ThreadData*)CreateThreadSpecific(sizeof(ThreadData));

    HUINT index = ptd->index;

    return DB_MAP::Instance()->Exec(strName, index, sqls);
}


CRedis* CHsServer::GetMem (HCSTRR strName) throw () {

    ThreadData* ptd = (ThreadData*)CreateThreadSpecific(sizeof(ThreadData));

    //HUINT index = (ptd->index - MAX_LISTEN_THREADS) / DB_CONN_COUNT;
    HUINT index = ptd->index;

    return DB_MAP::Instance()->GetMem (strName, index);

}


HRET CHsServer::CallApi (HCSTRR strName, HCSTRR strApiName, const HCParam& inp, HCParam& outp) throw () {

    CLIENT_MAP::iterator fit = m_clients.find(strName);
    HASSERT_THROW_MSG(fit != m_clients.end(), "not this client", INDEX_OUT);

    CMidClient* p = fit->second;
    HASSERT_THROW_MSG(p != nullptr, "midclient is not init", ILL_PT);

    HCParam ps = inp;
    ps.setValue(API_NAME, strApiName);

    HNOTOK_RETURN(p->CallApi(ps, outp));

    HASSERT_THROW_MSG(HIS_TRUE(outp.hasKey("err_no")), "api server return error", ERR_STATUS);

    HRETURN_OK;
}


void CHsServer::SetBlack (HCSTRR str) {

    HCStr::Split(str, ",", m_blacklist);
    
    for (size_t i = 0; i < m_blacklist.size(); ++i) {

        HCSTRR item = m_blacklist[i];
        SLOG_NORMAL("black list item: [%s]", item.c_str());

    }

}


void CHsServer::SetWhiteList (HCSTRR str) {
    HCStr::Split(str, ",", m_whiteList);
    
    for (size_t i = 0; i < m_whiteList.size(); ++i) {

        HCSTRR item = m_whiteList[i];
        SLOG_NORMAL("white list item: [%s]", item.c_str());

    }
}



HBOOL CHsServer::isInWhiteList (HCSTRR strIp) const {

    if (m_whiteList.empty()) {
        return HTRUE;
    }

    for (size_t i = 0; i < m_whiteList.size(); ++i) {
        HCSTRR item = m_whiteList[i];
        if (strIp.find(item) == 0) {
            return HTRUE;
        }
    }

    return HFALSE;

}


void CHsServer::middleEncode(HCSTRR src, HSTRR dst) {

    using namespace HUIBASE::CRYPTO;
    dst.clear();
    
    if (m_strPassword.empty()) {

        dst = src;

    } else {

        HDes3Encode(src, dst, m_strPassword);

    }

}


void CHsServer::middleDecode(HCSTRR src, HSTRR dst) {

    using namespace HUIBASE::CRYPTO;
    dst.clear();

    if (m_strPassword.empty()) {

        dst = src;

    } else {

        HDes3Decode(src, dst, m_strPassword);

    }

}