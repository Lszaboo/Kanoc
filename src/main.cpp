#include "app.hpp"
#include "app/harness.hpp"

int main(int argc, char **argv)
{
    uint32_t deviceID = 0;
    if (argc>1) {
        deviceID = std::stoi(argv[1]);
    }

    auto harnessedapp = Harness<App>();

    harnessedapp.Init();
    harnessedapp.Play();
    harnessedapp.Destroy();

    return 0;
}
