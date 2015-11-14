#include "handlers/client.h"

#include <common/types.h>

#include <libconfig.h++>

YAIC_NAMESPACE

ClientHandler::ClientHandler(Context *context) :
    m_context(context)
{

}

ClientHandler::~ClientHandler()
{

}

void ClientHandler::loadFromLibconfig(const libconfig::Setting &section)
{
    if (section.exists("listen")) {
        const libconfig::Setting &listenSection = section.lookup("listen");

        if (listenSection.isArray()) {
            for (auto &listener : listenSection)
                m_config.listen.push_back(listener);
        } else if (listenSection.isScalar()) {
            m_config.listen.push_back(listenSection);
        }
    }
}

void ClientHandler::init()
{

}



END_NAMESPACE
