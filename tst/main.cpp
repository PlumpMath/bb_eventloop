#include <vector>
#include <cstdlib>
#include <syslog.h>
#include <unistd.h>
#include <gtest/gtest.h>

namespace
{
    int privateMain(int argc, char **argv)
    {
        openlog("EventLoop-testrunner", LOG_PID, LOG_USER);
        testing::InitGoogleTest(&argc, argv);
        const int ret(RUN_ALL_TESTS());
        closelog();
        return ret;
    }
}

int main(int argc, char **argv)
{
    const char *valgrind(getenv("VALGRIND"));
    if (valgrind)
    {
        unsetenv("VALGRIND");
        std::vector<const char *> newArgv =
        {
            valgrind,
            "--error-exitcode=255",
            "--track-fds=yes",
            "--leak-check=full",
            "--show-reachable=yes",
        };
        const char *suppressions(getenv("VALGRIND_SUPPRESSIONS"));
        std::string suppressionsArg("--suppressions=");
        if (suppressions)
        {
            suppressionsArg += suppressions;
            newArgv.push_back(suppressionsArg.c_str());
        }
        if (getenv("XML_OUTPUT"))
        {
            newArgv.push_back("--xml=yes");
            newArgv.push_back("--log-file=testreport-valgrind.log");
            newArgv.push_back("--xml-file=testreport-valgrind.xml");
        }
        for (int i = 0; i < argc; ++i)
        {
            newArgv.push_back(argv[i]);
        }
        newArgv.push_back(nullptr);
        return execvp(valgrind, const_cast<char **>(newArgv.data()));
    }
    else
    {
        return privateMain(argc, argv);
    }
}
