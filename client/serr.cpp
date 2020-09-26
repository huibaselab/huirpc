

#include "serr.h"


CMiddleException::CMiddleException (const CErrorCode& error_code, HCSTRR strErrMsg, HINT err, HBOOL hComm, HBOOL bError, HINT iLineNo, HCPSZ szFile) : HCException(strErrMsg, err, hComm, bError, iLineNo, szFile), CErrorCode(error_code){

}



HCPSZ CMiddleException::baseMsg () const {

    return HCException::what();

}

HCPSZ CMiddleException::what () const throw (){
    
    return GetErrorMsg ().c_str();

}



CMiddleErrorManager::CMiddleErrorManager() {

    // add default value for unkown situation.


}


HRET CMiddleErrorManager::ReadErrorFromFile (HCSTRR strFileName) {

    using namespace libconfig;
    Config cfg;

    try {

        cfg.readFile(strFileName.c_str());

    } catch (const FileIOException& fiex) {

        SLOG_ERROR("I/O error while read file. msg: [%s]", fiex.what());

        HRETURN(IO_ERR);

    } catch (const ParseException& pex ) {

        SLOG_ERROR("Parse error map file failed at %s:%d--%s", pex.getFile(),
                  pex.getLine(), pex.getError());

        HRETURN(INVL_RES);

    }

    // init category.
    {

        const Setting& root = cfg.getRoot ();

        if (root.exists("error_category")) {

            const Setting& err_cate = root["error_category"];

            for (HINT i = 0; i < err_cate.getLength(); ++i) {

                HSTR k, v;
                err_cate[i].lookupValue("name", k);
                err_cate[i].lookupValue("value", v);

                addErrorCatagoryItem(k, v);

            }

        }

    }

    // init type.
    {

        const Setting& root = cfg.getRoot ();

        if (root.exists("error_type")) {

            const Setting& err_types = root["error_type"];

            for (HINT i = 0; i < err_types.getLength(); ++i) {

                HSTR k, v;
                err_types[i].lookupValue("name", k);
                err_types[i].lookupValue("value", v);

                addErrorTypeItem(k, v);

            }

        }

    }

    // init codes.
    {

        const Setting& root = cfg.getRoot ();

        if (root.exists("error_code")) {

            const Setting& err_codes = root["error_code"];

            for (HINT i = 0; i < err_codes.getLength(); ++i) {

                HSTR c, n, m;
                err_codes[i].lookupValue("code", c);
                err_codes[i].lookupValue("num", n);
                err_codes[i].lookupValue("msg", m);

                addErrorCode({c, n, m});

            }

        }

    }

    SLOG_NORMAL("read error map file [%s] success", strFileName.c_str());
    HRETURN_OK;
}


CMiddleException CMiddleErrorManager::GetMiddleException(HCSTRR strErrCode, HCSTRR strErrMsg, HINT err, HBOOL hComm, HBOOL bError, HINT iLineNo, HCPSZ szFile) {

    return CMiddleException(m_error_map[strErrCode], strErrMsg, err, hComm, bError, iLineNo, szFile);

}


const CErrorCode& CMiddleErrorManager::GetErrorCodeByNum (HCSTRR strnum) const {

    MIDDLE_NUM_ERROR_MAP::const_iterator cfit = m_num_error_map.find(strnum);
    HASSERT_THROW(cfit != m_num_error_map.cend(), INDEX_OUT);
    //return *(m_num_error_map[strnum]);
    return *(cfit->second);

}
 

void CMiddleErrorManager::addErrorCode (const CErrorCode& code) {

    MIDDLE_ERROR_MAP::iterator ret_it = m_error_map.insert(MIDDLE_ERROR_MAP::value_type(code.GetErrorCode(), code)).first;
    m_num_error_map.insert(MIDDLE_NUM_ERROR_MAP::value_type(code.GetErrorNum(), &(ret_it->second)));
    //m_error_map[code.GetErrorCode()] = code;

    m_error_dict.setValue(code.GetErrorNum(), code.GetErrorMsg());

}


