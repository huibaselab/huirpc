/*******************************************************
 *
 * FileName: midclient.h
 *
 * Author: Tom Hui, tomhui1009@yahoo.com, 8613760232170
 *
 * Create Date: Sat Jun 23 13:23 2018
 *
 * Brief:
 *
 *
 *******************************************************/


#ifndef __MID_CLIENT_H__
#define __MID_CLIENT_H__


#include <huibase.h>
#include <hsocket.h>
#include <hdict.h>

using namespace HUIBASE;


class CMidClient : public HCTcpSocket {
 public:
    static constexpr HUINT MAX_RECV_BUF_LEN = 1024 * 1024 * 2; //2M

 public:
    CMidClient (HCSTRR strIp, HUINT uPort, HUINT uTimeout = 3);

    virtual ~CMidClient () { }

    HINT GetErrNo () const { return m_nErrNo; }
    HCSTRR GetErrMsg () const { return m_strErrMsg; }

    HCSTRR GetPassword () const { return m_strPassword; }
    void SetPassword (HCSTRR strPassword) { m_strPassword = strPassword; }

    HRET CallApi (const HCParam& inp, HCParam& outp) throw ();

    HRET HandleError(const HCParam &para) const throw ();

 private:
    HSTR m_strIp;
    HUINT m_uPort;
    HUINT m_uTimeout;
    HSTR m_strPassword;

    HINT m_nErrNo;
    HSTR m_strErrMsg;

    HCHAR m_rebuf[MAX_RECV_BUF_LEN];
};




#endif //__MID_CLIENT_H__
