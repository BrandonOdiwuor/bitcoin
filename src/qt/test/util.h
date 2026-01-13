// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_UTIL_H
#define BITCOIN_QT_TEST_UTIL_H

#include <qt/bitcoin.h>

#include <QObject>
#include <QMap>
#include <QString>

#include <functional>
#include <chrono>

#include <qglobal.h>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

/**
 * Press "Ok" button in message box dialog.
 *
 * @param text - Optionally store dialog text.
 * @param msec - Number of milliseconds to pause before triggering the callback.
 */
void ConfirmMessage(QString* text, std::chrono::milliseconds msec);


// ----------------------------- Dynamic Test Discovery ---------------------------------

/**
 * Function type alias for constructing Qt test suite objects.
 *
 * Each registered test provides a constructor function that takes a
 * BitcoinApplication reference and returns a newly allocated QObject
 * representing the test suite.
 */
typedef std::function<QObject*(BitcoinApplication&)> TestConstructor;

/**
 * Type alias for the map storing registered test suites.
 *
 * The key is the test class name, and the value is a callable that
 * instantiates the corresponding test object.
 */
typedef QMap<QString, TestConstructor> TestRegistry;

/**
 * Access the singleton registry for dynamically registered Qt test suites.
 *
 * @return Reference to the global TestRegistry, which is initialized lazily
 *         and populated during static initialization by the REGISTER_QT_TEST macro.
 *         Used in the test runner to discover and instantiate available tests.
 */
TestRegistry& GetTestRegistry();

/**
 * Macro to register a Qt test suite class for dynamic discovery.
 *
 * @param Class The name of the test class (derived from QObject) to register.
 * @param ...   Optional arguments to forward to the class constructor
 *              (e.g., 'app' for classes needing BitcoinApplication,
 *              'app.node()' for those requiring a Node interface).
 *              Omit if the class has a default constructor.
 *
 * This macro should be placed at the end of the test class's .cpp file,
 * outside any functions or named namespaces. It creates a unique static
 * registrar struct whose constructor inserts a lambda factory into the
 * global TestRegistry during program startup, enabling automatic test
 * detection without manual enumeration in the test runner.
 *
 * Example usage:
 *   REGISTER_QT_TEST(AppTests, app);  // For a class needing BitcoinApplication
 *   REGISTER_QT_TEST(AddressBookTests, app.node());  // For a class needing BitcoinApplication node
 *   REGISTER_QT_TEST(URITests);       // For a class with no parameters
 */
#define REGISTER_QT_TEST(Class, ...)                                                                                \
    namespace {                                                                                                     \
    struct Class##_Registrar {                                                                                      \
        Class##_Registrar() {                                                                                       \
            GetTestRegistry()[#Class] = [](BitcoinApplication& app) -> QObject* { return new Class(__VA_ARGS__); }; \
        }                                                                                                           \
    };                                                                                                              \
    static Class##_Registrar Class##_registrar;                                                                     \
    }

#endif // BITCOIN_QT_TEST_UTIL_H
