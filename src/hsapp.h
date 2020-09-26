/*******************************************************
 *
 * FileName: hsapp.h
 *
 * Author: Tom Hui, tomhui1009@yahoo.com, 8613760232170
 *
 * Create Date: Tue Jun 19 20:28 2018
 *
 * Brief:
 *
 *
 *******************************************************/

#ifndef __HSAPP_H__
#define __HSAPP_H__

#include <huibase.h>
#include <happ.h>

#include "hsserver.h"

using namespace HUIBASE;

class CMiddleApp : public HCApp {
 public:
    CMiddleApp (HINT argc, const HCHAR* argv[]);

    virtual ~CMiddleApp ();

    HBOOL Run ();

 private:
    virtual void init ();

    CHsServer* m_pServer = nullptr;

};


#endif //__HSAPP_H__
