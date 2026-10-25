#include "app.hpp"
#include "kanoc/harness.hpp"

int main(int argc, char **argv)
{
    Harness<App> harnessedApp;

    harnessedApp.Init(argv[0]);
    harnessedApp.Play();
    harnessedApp.Destroy();

    return 0;
}
