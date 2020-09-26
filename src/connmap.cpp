

#include "connmap.h"
#include <hlog.h>
#include <hredis.h>

using namespace HUIBASE;
using namespace HUIBASE::HMYSQL;


CConnMap::~CConnMap () {

    for (CONN_MAP::iterator it = m_connmap.begin(); it != m_connmap.end(); ++it) {
        INDEX_CONNS& conns = it->second;

        for (INDEX_CONNS::iterator sit = conns.begin(); sit != conns.end(); ++sit) {

            CMyConnection* p = sit->second;

            HDELP(p);

        }


    }

}

void CConnMap::AddMem (HCSTRR str, HUINT index, CRedis* redis) {

    RDS_MAP::iterator fit = m_rdsmap.find(str);

    if (fit != m_rdsmap.end()) {

        INDEX_REDIS& conns = fit->second;

        INDEX_REDIS::iterator sfit = conns.find(index);

        if (sfit == conns.end()) {

            // add
            conns.insert(INDEX_REDIS::value_type(index, redis));

        }

    } else {

        INDEX_REDIS conn;
        conn.insert(INDEX_REDIS::value_type(index, redis));
        m_rdsmap.insert(RDS_MAP::value_type(str, conn));

    }

}


CRedis* CConnMap::GetMem (HCSTRR strName, HUINT index) {

    RDS_MAP::iterator fit = m_rdsmap.find(strName);

    if (fit != m_rdsmap.end()) {

        INDEX_REDIS& ics = fit->second;

        INDEX_REDIS::iterator sfit = ics.find(index);

        if (sfit != ics.end()) {

            CRedis* p = sfit->second;

            return p;

        }

    }

    return nullptr;
}


void CConnMap::AddConn(HCSTRR str, HUINT index, CMyConnection *p) {

    CONN_MAP::iterator fit = m_connmap.find(str);

    if (fit != m_connmap.end()) {

        INDEX_CONNS& conns = fit->second;

        INDEX_CONNS::iterator sfit = conns.find(index);

        if (sfit == conns.end()) {

            // add
            conns.insert(INDEX_CONNS::value_type(index, p));

        }

    } else {

        INDEX_CONNS conn;
        conn.insert(INDEX_CONNS::value_type(index, p));
        m_connmap.insert(CONN_MAP::value_type(str, conn));

    }

}


CMyConnection* CConnMap::GetConn (HCSTRR strName, HUINT index) {

    //SLOG_NORMAL("name[%s] index[%d]", strName.c_str(), index);
    CONN_MAP::iterator fit = m_connmap.find(strName);

    if (fit != m_connmap.end()) {

        INDEX_CONNS& ics = fit->second;

        INDEX_CONNS::iterator sfit = ics.find(index);

        if (sfit != ics.end()) {

            CMyConnection* p = sfit->second;

            IF_FALSE(p->Ping()) {
                SLOG_WS("disconnect db...");
                p->Reconnect ();
                p->SetUtf8();
            }

            return p;

        }

    }

    return nullptr;

}


HRET CConnMap::Query(HCSTRR strName, HUINT index, HCSTRR sql, CRes &db_res) throw () {

    CMyConnection* conn = GetConn(strName, index);

    HASSERT_RETURN(conn != nullptr, DB_DISCONN);

    //SLOG_NORMAL("name[%s] index[%d] conn[%p] sql[%s]", strName.c_str(), index, conn, sql.c_str());

    return conn->Query(sql, db_res);

}


HRET CConnMap::Exec (HCSTRR strName, HUINT index, HCSTRR sql) throw () {

    CMyConnection* conn = GetConn(strName, index);
    //LOG_NORMAL("test_conn[%p] exec...", conn);

    HASSERT_RETURN(conn != nullptr, DB_DISCONN);

    return conn->Exec(sql);

}



HRET CConnMap::Exec (HCSTRR strName, HUINT index, HCVSTRSR sqlss) throw () {

    CMyConnection* conn = GetConn(strName, index);

    HASSERT_RETURN(conn != nullptr, DB_DISCONN);

    return conn->MultiExec(sqlss);

}
