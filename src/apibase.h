/*******************************************************
 *
 * FileName: apibase.h
 *
 * Author: Tom Hui, tomhui1009@yahoo.com, 8613760232170
 *
 * Create Date: Wed Jun 20 12:57 2018
 *
 * Brief:
 *
 *
 *******************************************************/

#ifndef __API_BASE_H__
#define __API_BASE_H__

#include <huibase.h>
#include <hsingleton.hpp>
#include <hdict.h>
#include "hsserver.h"

using namespace HUIBASE;



class CHsServer;
class CApiBase {
 public:
    CApiBase (HCSTRR strName) : m_strApiName (strName) { }

    virtual ~CApiBase () { }

    void SetServer(CHsServer* pServer) { m_pServer = pServer; }

    HCSTRR GetName () const { return m_strApiName; }

    virtual HRET Init (const HCParam& ps) throw () { m_ps = ps; HRETURN_OK; }

    virtual HRET Work () throw () = 0;

    virtual HSTR GetRes () throw ();

 protected:
    void AppendOutput (HCSTRR key, HCSTRR val);

	void HandleError (const HCParam & res) throw ();

    HCParam m_ps;

    HCParam m_out;

 private:
    HSTR m_strApiName;

 protected:
    CHsServer* m_pServer = nullptr;

};


class CApiFactory {
 public:
	typedef CApiBase* (*api_maker) ();
 public:

	CApiBase* newApi(HCSTRR strName) ;

	void RegisteApi (HCSTRR strName, api_maker maker);


 private:
	std::map<HSTR, api_maker> m_makers;

};

typedef HCSingleton<CApiFactory> maker_facetory;


#define REGISTE_API(name,apiobj)                                    \
	static CApiBase* class_##name_##apiobj () {                         \
		return new apiobj(#name);                                       \
	}                                                                   \
	class __CAPICreate_##name_##apiobj {                            \
	public:                                                             \
		__CAPICreate_##name_##apiobj () {                           \
			maker_facetory::Instance()->RegisteApi(#name,class_##name_##apiobj); \
		}                                                               \
	};                                                                  \
	static const __CAPICreate_##name_##apiobj __creator_##name_##apiobj_maker


#endif
