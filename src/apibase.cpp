
#include <hlog.h>
#include "apibase.h"
#include "connmap.h"

#include <mysqldata.h>

using namespace HUIBASE;
using namespace HUIBASE::HMYSQL;


void CApiBase::AppendOutput(HCSTRR strKey, HCSTRR strVal) {

    m_out.setValue (strKey, strVal);

}


void CApiBase::HandleError (const HCParam & res) throw () {

    HCSTRR err_no = res.GetValue("err_no");
    if (err_no != "0") {

        //LOG_ERROR("err-no:[%s]", err_no.c_str());
        HTHROW_MIDDLE_EX(err_no, ERR_STATUS, "call middle server failed");
		//int tem = HCStr::stoi(res.GetValue("err_no"));

        //HASSERT_CGI(tem != 1002, USER_CHECK_FAILED, ERR_STATUS, "username or password is invalid");
        //HASSERT_CGI(tem != 1101, KKB_HASBIND, ERR_STATUS, "kkb has be band");
        //HASSERT_CGI(tem != 1102, KKB_STATUS, ERR_STATUS, "kkb has be band");
        HASSERT_SS(false, 999999, ERR_STATUS, "system error");

	}

}



CApiBase* CApiFactory::newApi(HCSTRR strName) {

    std::map<HSTR, api_maker>::iterator fit = m_makers.find(strName);

    if (fit == m_makers.end()) {
        return nullptr;
    }

    api_maker pfun = fit->second;

    if (pfun != nullptr) {
        return pfun();
    }

    return nullptr;

}


void CApiFactory::RegisteApi(HCSTRR strName, CApiFactory::api_maker maker) {

    if (m_makers.find(strName) != m_makers.end()) {
        exit(-1);
    }

    m_makers.insert(std::map<HSTR, api_maker>::value_type(strName, maker));

}


HSTR CApiBase::GetRes() throw () {

    HSTR res = "err_no=0&err_msg=OK";

    if (m_out.empty()) {

        return res;

    }

    stringstream ss;

    for (HCParam::iterator it = m_out.begin(); it != m_out.end(); ++it) {

        ss << it->first << "=" << it->second << "&";

    }

    HSTR tmp = ss.str();
    tmp = tmp.substr(0, tmp.length() - 1);

    return res + "&" + tmp;
}


class CTestApi : public CApiBase {
public:
    CTestApi (HCSTRR strName) : CApiBase(strName) { }

    HRET Work () throw () {
        HFUN_BEGIN;

        CRes db_res;
        m_pServer->ExecDB("testdb", "insert into t_test (`name`, age) values ('tom', 10)");

        HFUN_END;
        HRETURN_OK;
    }
};

REGISTE_API(test, CTestApi);
