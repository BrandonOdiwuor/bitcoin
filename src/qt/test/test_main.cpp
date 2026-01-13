// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <interfaces/init.h>
#include <interfaces/node.h>
#include <qt/bitcoin.h>
#include <qt/guiconstants.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>
#include <qt/test/util.h>

#ifdef ENABLE_WALLET
#include <qt/test/addressbooktests.h>
#include <qt/test/wallettests.h>
#endif // ENABLE_WALLET

#include <QApplication>
#include <QDebug>
#include <QObject>
#include <QSettings>
#include <QTest>

#include <functional>

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS{};

const std::function<std::string()> G_TEST_GET_FULL_NAME{};

// This is all you need to run all the tests
int main(int argc, char* argv[])
{
    // Initialize persistent globals with the testing setup state for sanity.
    // E.g. -datadir in gArgs is set to a temp directory dummy value (instead
    // of defaulting to the default datadir), or globalChainParams is set to
    // regtest params.
    //
    // All tests must use their own testing setup (if needed).
    fs::create_directories([] {
        BasicTestingSetup dummy{ChainType::REGTEST};
        return gArgs.GetDataDirNet() / "blocks";
    }());

    std::unique_ptr<interfaces::Init> init = interfaces::MakeGuiInit(argc, argv);
    gArgs.ForceSetArg("-listen", "0");
    gArgs.ForceSetArg("-listenonion", "0");
    gArgs.ForceSetArg("-discover", "0");
    gArgs.ForceSetArg("-dnsseed", "0");
    gArgs.ForceSetArg("-fixedseeds", "0");
    gArgs.ForceSetArg("-natpmp", "0");

    std::string error;
    if (!gArgs.ReadConfigFiles(error, true)) qWarning() << error.c_str();

    // Prefer the "minimal" platform for the test instead of the normal default
    // platform ("xcb", "windows", or "cocoa") so tests can't unintentionally
    // interfere with any background GUIs and don't require extra resources.
    #if defined(WIN32)
        if (getenv("QT_QPA_PLATFORM") == nullptr) _putenv_s("QT_QPA_PLATFORM", "minimal");
    #else
        setenv("QT_QPA_PLATFORM", "minimal", 0 /* overwrite */);
    #endif


    QCoreApplication::setOrganizationName(QAPP_ORG_NAME);
    QCoreApplication::setApplicationName(QAPP_APP_NAME_DEFAULT "-test");

    int num_test_failures{0};

    {
        BitcoinApplication app;
        app.createNode(*init);

        TestRegistry& test_registry = GetTestRegistry();

        // If the user has specified a *Test suite name, execute that test suite only.
        for (int index = 1; index < argc; ++index) {
            if (qstrcmp(argv[index], "-functions") == 0) {
                for (const QString& test_name : test_registry.keys()) {
                    qDebug().noquote() << test_name;
                }
                return EXIT_SUCCESS;
            } else if (QString(argv[index]).contains("Test")) {
                const QString test_name = QString::fromLocal8Bit(argv[index]);
                if (test_registry.contains(test_name)) {
                    QStringList args = app.arguments();
                    const auto test_suite_constructor = test_registry.value(test_name);
                    QObject* test_suite = (test_suite_constructor)(app);
                    if (!test_suite) {
                        qWarning() << "Failed to create test object for " << argv[index];
                        return EXIT_FAILURE;
                    }
                    return QTest::qExec(test_suite, args);
                } else {
                    qWarning() << "Test class " << argv[index] << " is unknown\n";
                    return EXIT_FAILURE;
                }
            }
        }

        // Otherwise, execute all registered test suites.
        for (const auto& test_suite_constructor : test_registry.values()) {
            QObject* test_suite = (test_suite_constructor)(app);
            if (!test_suite || (QTest::qExec(test_suite, argc, argv)) != 0) {
                ++num_test_failures; 
            }
        }

        if (num_test_failures) {
            qWarning("\nFailed tests: %d\n", num_test_failures);
        } else {
            qDebug("\nAll tests passed.\n");
        }
    }

    QSettings settings;
    settings.clear();

    return num_test_failures;
}
