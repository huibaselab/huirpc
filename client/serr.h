

#ifndef __SERR_H__
#define __SERR_H__


#include <huibase.h>
#include <hdict.h>

#include <hsingleton.hpp>
#include <hlog.h>
#include <libconfig.h++>

using namespace HUIBASE;

class CErrorCode {
public:    
    CErrorCode () { }

    CErrorCode (const CErrorCode& ec) 
    : m_strCode(ec.m_strCode), m_strErrorNum(ec.m_strErrorNum), m_strErrorMsg(ec.m_strErrorMsg) { }

    CErrorCode (HSTR strCode, HCSTRR strErrorNum, HCSTRR strErrorMsg)
        : m_strCode(strCode), m_strErrorNum(strErrorNum), m_strErrorMsg(strErrorMsg) { }

    HCSTRR GetErrorCode () const { return m_strCode; }

    HCSTRR GetErrorNum () const { return m_strErrorNum; }

    HCSTRR GetErrorMsg () const { return m_strErrorMsg; }

private:
    HSTR m_strCode;
    // XXX(ERROR-CATEGORY)-XXX(ERROR-TYPE)-XXXXX(ERROR-CODE)
    HSTR m_strErrorNum;
    HSTR m_strErrorMsg;
};

class CMiddleException : public HCException, public CErrorCode {
public:
    CMiddleException (const CErrorCode& error_code, HCSTRR strErrMsg, HINT err, HBOOL hComm, HBOOL bError, HINT iLineNo, HCPSZ szFile);

    HCPSZ baseMsg () const;

    virtual HCPSZ what () const throw ();
    
};


class CMiddleErrorManager {
public:
    typedef std::map<HSTR, CErrorCode> MIDDLE_ERROR_MAP;
    typedef std::map<HSTR, CErrorCode*> MIDDLE_NUM_ERROR_MAP;
    
    CMiddleErrorManager ();

    HRET ReadErrorFromFile (HCSTRR strFileName);

    HCSTRR GetErrorCategory (HCSTRR k) const { return m_error_category.GetValue(k); }

    HCSTRR GetErrorType (HCSTRR k) const { return m_error_type.GetValue(k); }

    CMiddleException GetMiddleException(HCSTRR strErrorCode, HCSTRR strErrMsg, HINT err, HBOOL hComm, HBOOL bError, HINT iLineNo, HCPSZ szFile);

    HCSTRR GetErrorMsg (HCSTRR str) const throw () { return m_error_dict.GetValue(str); }

    const CErrorCode& GetErrorCodeByNum (HCSTRR strnum) const;

private:
    void addErrorCatagoryItem (HCSTRR k, HCSTRR v) { m_error_category.setValue (k, v); }

    void addErrorTypeItem (HCSTRR k, HCSTRR v) { m_error_type.setValue (k, v); }

    void addErrorCode (const CErrorCode& code);

private:
    HCParam m_error_category;
    HCParam m_error_type;

    MIDDLE_ERROR_MAP m_error_map;
    MIDDLE_NUM_ERROR_MAP m_num_error_map;
    HCParam m_error_dict;
};

typedef HCSingleton<CMiddleErrorManager> ERROR_INFO_MAP;

#define HASSERT_SS(ff,cc,val,msg) do { \
    if (not (ff)) { \
        throw ERROR_INFO_MAP::Instance()->GetMiddleException((#cc),(msg),HERR_NO(val),HFALSE, HTRUE, __LINE__, __FILE__); \
    } } while(0)

#define HTHROW_MIDDLE_EX(num,val,msg) do { \
        throw CMiddleException(ERROR_INFO_MAP::Instance()->GetErrorCodeByNum((num)), (msg), HERR_NO(val), HFALSE, HTRUE, __LINE__, __FILE__); \
    } while(0) 

#endif 