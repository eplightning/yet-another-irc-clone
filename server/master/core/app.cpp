#include "core/app.h"

#include "core/context.h"

#include <libconfig.h++>
#include <common/types.h>
#include <server/log/stdout.h>
#include <server/misc_utils.h>

#include <thread>

YAIC_NAMESPACE

MasterServerApplication::MasterServerApplication() :
    m_clientHandler()
{
    m_context = new Context;
}

MasterServerApplication::~MasterServerApplication()
{
    delete m_context->log;
    delete m_context;
}

int MasterServerApplication::run(const char *configPath)
{
    // tworzenie kontekstu
    m_context->configPath = configPath;
    m_context->log = new LogStdout; // todo: z configa

    // kontekst gotowy, handlery możemy utworzyć
    m_clientHandler = std::make_shared<ClientHandler>(m_context);

    if (!loadConfig())
        return 1;

    if (!initHandlers())
        return 2;

    if (!initTcpServer())
        return 3;

    // event loop
    while (true) {
        Event *ev = m_context->eventQueue.pop();

        if (ev->type() == Event::Type::Packet) {
            EventPacket *evp = static_cast<EventPacket*>(ev);

            if (evp->source() == 1)
                m_context->clientDispatcher.dispatch(evp->clientid(), evp->packet());
            else if (evp->source() == 2)
                m_context->slaveDispatcher.dispatch(evp->clientid(), evp->packet());
        }
    }

    return 0;
}

bool MasterServerApplication::loadConfig()
{
    try {
        m_config.readFile((m_context->configPath + "/master.cfg").c_str());

        if (m_config.exists("clientHandler")) {
            const libconfig::Setting &section = m_config.lookup("clientHandler");

            if (section.isGroup())
                m_clientHandler->loadFromLibconfig(section);
        }
    } catch (std::exception &e) {
        *m_context->log << Log::Line::Start
                        << "Błąd podczas wczytywania konfiguracji: " << e.what()
                        << Log::Line::End;

        return false;
    }

    return true;
}

bool MasterServerApplication::initHandlers()
{
    try {
        m_clientHandler->init();
    } catch (std::exception &e) {
        *m_context->log << Log::Line::Start
                        << "Błąd podczas inicjalizacji handlerów: " << e.what()
                        << Log::Line::End;

        return false;
    }

    return true;
}

bool MasterServerApplication::initTcpServer()
{
    m_tcpThread = std::thread([&] {
        MiscUtils::blockSignals();

        m_context->tcp.runLoop();

        // todo: dodać do event loopa błąd
    });

    return true;
}

END_NAMESPACE
