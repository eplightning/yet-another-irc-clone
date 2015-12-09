#include <core/app.h>

int main(int argc, char *argv[])
{
    YAIC::MasterServerApplication app;

    const char *configPath = "/etc/yaic";
    if (argc >= 2)
        configPath = argv[1];

    return app.run(configPath);
}
