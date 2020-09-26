

#include "midclient.h"
#include <haddr.h>
#include <hcrypto.h>
#include <hlog.h>
#include "serr.h"

using namespace HUIBASE::CRYPTO;


CMidClient::CMidClient (HCSTRR strIp, HUINT nPort, HUINT uTimeout)
    : m_strIp (strIp), m_uPort(nPort), m_uTimeout (uTimeout) {

    memset(m_rebuf, 0, MAX_RECV_BUF_LEN);

}


HRET CMidClient::CallApi(const HCParam &inp, HCParam &outp) throw (){

    HASSERT_RETURN(HIS_TRUE(inp.hasKey("apiname")), INVL_PARA);

    memset(m_rebuf, 0, MAX_RECV_BUF_LEN);

    HSTR reqSrc = inp.ParamToString (), strReq;

    if (not m_strPassword.empty()) {
        HDes3Encode(reqSrc, strReq, m_strPassword);
    } else {
        strReq = reqSrc;
    }

    HINT buf_len = sizeof(HINT) + strReq.length();
    HPSZ buf = new HCHAR[buf_len];
    CHECK_NEWPOINT(buf);

    memset(buf, 0, buf_len);
    HCHAR* pos = buf;
    *((HINT*)pos) = strReq.length();
    pos += sizeof(HINT);
    memcpy(pos, strReq.c_str(), strReq.length());

    HCIp4Addr addr(m_strIp, m_uPort);

    Init ();
    SetNonblocked();

    HRET cb = HERR_NO(OK);

    do {

        cb = ConnectWithTimeOut(addr, m_uTimeout);
        HIF_NOTOK(cb) {

            break;

        }

        cb = WriteAll(buf, buf_len, m_uTimeout);
        if (cb != buf_len) {

            break;

        }

        try {

            cb = ReadWithTimeOut (m_rebuf, MAX_RECV_BUF_LEN, m_uTimeout);

        } catch (...) {


            break;

        }

        if (cb <= 0) {

            break;

        }

        HSTR srcRecv(m_rebuf), strRecv;

        if (not m_strPassword.empty()) {
            HDes3Decode(srcRecv, strRecv, m_strPassword);
        } else {
            strRecv = srcRecv;
        }

        outp.SetParam (strRecv, "&", "=");


    } while (0);

    Close();

    return cb;
}


HRET CMidClient::HandleError(const HCParam &para) const throw (){

    HASSERT_THROW_MSG(HIS_TRUE(para.hasKey("err_no")), "parameter err_no is missing", INVL_RES);

    HCSTRR strErrorno = para.GetValue("err_no");

    if (strErrorno != "0") {

        SLOG_ERROR("call middle api failed. MiddleErrorNum[%s]", strErrorno.c_str());

        throw CMiddleException({"0", strErrorno, ERROR_INFO_MAP::Instance()->GetErrorMsg(strErrorno)}, "call api failed", HERR_NO(ERR_STATUS), HFALSE, HTRUE, 0, 0);

    }

    HRETURN_OK;
}