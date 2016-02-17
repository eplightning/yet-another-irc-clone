#include <core/app.h>

#include <core/global.h>
#include <core/context.h>
#include <modules/slave.h>
#include <modules/user.h>

#include <common/types.h>
#include <server/logger/stdout.h>
#include <server/misc_utils.h>
#include <server/syseventloop.h>
#include <server/dispatcher.h>

#include <libconfig.h++>
#include <thread>

YAIC_NAMESPACE

MasterServerApplication::MasterServerApplication()
    : m_context(new Context), m_confWorkers(5)
{

}

MasterServerApplication::~MasterServerApplication()
{
    m_context->tcp->stopLoop();
    m_context->sysLoop->stopLoop();

    if (m_tcpThread.joinable())
        m_tcpThread.join();

    if (m_sysThread.joinable())
        m_sysThread.join();

    delete m_context;
}

int MasterServerApplication::run(const char *configPath)
{
    // blokujemy ood razu wszystkie sygnały
    MiscUtils::blockSignals();

    // tworzenie kontekstu
    m_context->configPath = configPath;
    m_context->eventQueue.reset(new EventQueue);
    m_context->log.reset(new LoggerStdout);
    m_context->sysLoop.reset(SysEventLoop::factory(*m_context->eventQueue));
    m_context->tcp.reset(new TcpManager);
    m_context->dispatcher.reset(new PacketDispatcher);

    m_context->log << Logger::Line::Start
                   << "Starting master server using the following configuration path: " << configPath
                   << Logger::Line::End;

    // kontekst gotowy, moduły możemy utworzyć
    m_context->user.reset(new UserModule(m_context));
    m_context->slave.reset(new SlaveModule(m_context));

    if (!loadConfig())
        return 1;

    if (!initModules())
        return 2;

    if (!initTcpServer())
        return 3;

    if (!initSysEvent())
        return 4;

    Context *context = m_context;
    EventLoop *loop = new EventLoop(m_confWorkers, m_context->eventQueue.get(), [context] (Event *ev) {
        bool looping = true;

        if (ev->type() == Event::Type::Packet) {
            EventPacket *evp = static_cast<EventPacket*>(ev);

            if (evp->source() == MASTER_APP_SOURCE_USER)
                context->user->dispatchPacket(evp);
            else if (evp->source() == MASTER_APP_SOURCE_SLAVE)
                context->slave->dispatchPacket(evp);

            delete evp->packet();
        } else if (ev->type() == Event::Type::Simple) {
            EventSimple *evs = static_cast<EventSimple*>(ev);

            context->user->dispatchSimple(evs);
            context->slave->dispatchSimple(evs);

            if (evs->id() == EventSimple::EventId::SignalHangUp || evs->id() == EventSimple::EventId::SignalInterrupt
                    || evs->id() == EventSimple::EventId::SignalQuit || evs->id() == EventSimple::EventId::SignalTerminate
                    || evs->id() == EventSimple::EventId::SysLoopDied || evs->id() == EventSimple::EventId::TcpLoopDied) {
                looping = false;
                context->log->message("Stopping execution ...");
                context->eventQueue->stop();
            }
        } else if (ev->type() == Event::Type::Timer) {
            EventTimer *evt = static_cast<EventTimer*>(ev);

            context->user->dispatchTimer(evt);
            context->slave->dispatchTimer(evt);
        }

        delete ev;

        return looping;
    });

    m_context->log->message("Starting worker threads ...");
    loop->startThreads();
    m_context->log->message("Starting event loop inside main thread ...");
    loop->run();
    m_context->log->message("Waiting for worker threads to finish ...");
    loop->waitForThreads();
    m_context->log->message("Force closing sockets ...");
    m_context->tcp->disconnectAll(true);

    delete loop;

    return 0;
}

bool MasterServerApplication::loadConfig()
{
    try {
        m_config.readFile((m_context->configPath + "/master.cfg").c_str());
    } catch (libconfig::ConfigException &e) {
        m_context->log << Logger::Line::Start
                       << "Error while reading configuration: " << e.what()
                       << Logger::Line::End;

        return true;
    }

    if (m_config.exists("user-module")) {
        const libconfig::Setting &section = m_config.lookup("user-module");

        if (section.isGroup())
            m_context->user->loadConfig(section);
    }

    if (m_config.exists("slave-module")) {
        const libconfig::Setting &section = m_config.lookup("slave-module");

        if (section.isGroup())
            m_context->slave->loadConfig(section);
    }

    if (m_config.exists("worker-threads")) {
        const libconfig::Setting &section = m_config.lookup("worker-threads");

        if (section.isNumber())
            m_confWorkers = section;

        if (m_confWorkers < 0)
            m_confWorkers = 0;
    }

    return true;
}

bool MasterServerApplication::initModules()
{
    if (!m_context->user->init()) {
        m_context->log->error("Error while initializing user module");
        return false;
    }

    if (!m_context->slave->init()) {
        m_context->log->error("Error while initializing slave module");
        return false;
    }

    return true;
}

bool MasterServerApplication::initTcpServer()
{
    m_tcpThread = std::thread([this] {
        m_context->tcp->runLoop();
        m_context->eventQueue->append(new EventSimple(EventSimple::EventId::TcpLoopDied));
    });

    return true;
}

bool MasterServerApplication::initSysEvent()
{
    m_sysThread = std::thread([this]() {
        if (!m_context->sysLoop->runLoop())
            m_context->log->error("Cannot initalize system event loop");

        m_context->eventQueue->append(new EventSimple(EventSimple::EventId::SysLoopDied));
    });

    return true;
}

END_NAMESPACE
