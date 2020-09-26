
#include <hlog.h>
#include "hsapp.h"

CMiddleApp::CMiddleApp (HINT argc, const HCHAR* argv[])
    : HCApp (argc, argv) {
}


CMiddleApp::~CMiddleApp() {

    HDELP(m_pServer);

}

HBOOL CMiddleApp::Run() {

    while (IsRunning ()) {

        try {

            m_pServer->ManageChild();

        } catch(const HCException& ex) {

            SLOG_ERROR("CMiddleApp::Run get an exception[%s]", ex.what());

        }

    }

    m_pServer->SetStop ();

    return HTRUE;

}


void CMiddleApp::init() {

    m_pServer = new CHsServer(m_conf.GetValue("serverName"), m_conf.GetValue("ip"), m_conf.GetIntValue("port"), m_conf.GetIntValue("listenLen"), HTRUE, m_conf.GetIntValue("mq_key"), m_conf.GetIntValue("thread_count"), m_conf.GetValue("lock_file"));
    CHECK_NEWPOINT(m_pServer);

    IF_TRUE(m_conf.IsHere("black_list")) {
        m_pServer->SetBlack(m_conf.GetValue("black_list"));
    }

    IF_TRUE(m_conf.IsHere("white_list")) {
        m_pServer->SetWhiteList(m_conf.GetValue("white_list"));
    }

    IF_TRUE(m_conf.IsHere("error_file")) {
        m_pServer->SetErrorMapFile(m_conf.GetValue("error_file"));
    }

    IF_TRUE(m_conf.IsHere("password")) {
        m_pServer->SetPassword(m_conf.GetValue("password"));
    }

    HASSERT_THROW_MSG(HIS_OK(m_pServer->MeInit()), "CMiddleApp init server failed", UN_INIT);

}
