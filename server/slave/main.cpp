#include <core/app.h>

int main(int argc, char *argv[])
{
    YAIC::SlaveServerApplication app;

    const char *configPath = "/etc/yaic";
    const char *configName = "slave";

    if (argc >= 3)
        configPath = argv[2];
    if (argc >= 2)
        configName = argv[1];

    int ret = app.run(configPath, configName);

    return ret;
}
