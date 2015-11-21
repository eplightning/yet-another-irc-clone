#include <core/app.h>

#include <core/context.h>
#include <common/types.h>
#include <server/log/stdout.h>
#include <server/misc_utils.h>
#include <server/syseventloop.h>

#include <libconfig.h++>
#include <thread>

#include <signal.h>

YAIC_NAMESPACE

MasterServerApplication::MasterServerApplication()
    : m_context(new Context), m_userModule(nullptr)
{

}

MasterServerApplication::~MasterServerApplication()
{
    if (m_tcpThread.joinable()) {
        m_context->tcp.stopLoop();
        m_tcpThread.join();
    }

    if (m_sysThread.joinable()) {
        m_context->sysLoop->stopLoop();
        m_sysThread.join();
    }

    if (m_userModule != nullptr)
        delete m_userModule;

    delete m_context;
}

int MasterServerApplication::run(const char *configPath)
{
    // blokujemy ood razu wszystkie sygnały
    MiscUtils::blockSignals();

    // tworzenie kontekstu
    m_context->configPath = configPath;
    m_context->log = new LogStdout; // todo: z configa
    m_context->sysLoop = SysEventLoop::factory(&m_context->eventQueue);

    // kontekst gotowy, moduły możemy utworzyć
    m_userModule = new UserModule(m_context);

    if (!loadConfig())
        return 1;

    if (!initModules())
        return 2;

    if (!initTcpServer())
        return 3;

    if (!initSysEvent())
        return 4;

    // event loop
    bool looping = true;

    while (looping) {
        Event *ev = m_context->eventQueue.pop();

        if (ev->type() == Event::Type::Packet) {
            EventPacket *evp = static_cast<EventPacket*>(ev);

            if (evp->source() == 1)
                m_context->userDispatcher.dispatch(evp->clientid(), evp->packet());
            else if (evp->source() == 2)
                m_context->slaveDispatcher.dispatch(evp->clientid(), evp->packet());

            delete evp->packet();
        } else if (ev->type() == Event::Type::Simple) {
            // killing
            looping = false;
        } else if (ev->type() == Event::Type::Timer) {
            EventTimer *evt = static_cast<EventTimer*>(ev);

            m_context->timerDispatcher.dispatch(evt->timer());
        }

        delete ev;
    }

    *m_context->log << Log::Line::Start
                    << "Main loop finished ..."
                    << Log::Line::End;

    // todo: może nie wymuszać ale dać timeouta
    m_context->tcp.disconnectAll(true);

    return 0;
}

bool MasterServerApplication::loadConfig()
{
    try {
        m_config.readFile((m_context->configPath + "/master.cfg").c_str());

        if (m_config.exists("user_module")) {
            const libconfig::Setting &section = m_config.lookup("user_module");

            if (section.isGroup())
                m_userModule->loadFromLibconfig(section);
        }
    } catch (std::exception &e) {
        *m_context->log << Log::Line::Start
                        << "Error while reading configuration: " << e.what()
                        << Log::Line::End;

        return false;
    }

    return true;
}

bool MasterServerApplication::initModules()
{
    try {
        m_userModule->init();
    } catch (std::exception &e) {
        *m_context->log << Log::Line::Start
                        << "Error while initializing modules: " << e.what()
                        << Log::Line::End;

        return false;
    }

    return true;
}

bool MasterServerApplication::initTcpServer()
{
    m_tcpThread = std::thread([this] {
        m_context->tcp.runLoop();
        m_context->eventQueue.append(new EventSimple(EventSimple::EventId::TcpLoopDied));
    });

    return true;
}

bool MasterServerApplication::initSysEvent()
{
    m_sysThread = std::thread([this]() {
        m_context->sysLoop->runLoop();
        m_context->eventQueue.append(new EventSimple(EventSimple::EventId::SysLoopDied));
    });

    return true;
}

END_NAMESPACE
