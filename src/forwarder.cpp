// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "forwarder.h"

#include <dbus/dbus.h>

#include <QByteArray>
#include <QDir>
#include <QString>

#include <cstdlib>
#include <cstring>
#include <memory>

namespace
{
// Adds {"key": <string>} to an a{sv} dictionary being built.
void appendString(DBusMessageIter *dict, const char *key, const char *value)
{
    DBusMessageIter entry;
    DBusMessageIter variant;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}
}

std::optional<int> forwardToRunningInstance(int argc, char **argv, const char *serviceName)
{
    // Options (--help, --version…) are answered by this process; and with no files, there is nothing to forward.
    if (argc < 2) {
        return std::nullopt;
    }
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            return std::nullopt;
        }
    }

    DBusError error;
    dbus_error_init(&error);
    const std::unique_ptr<DBusConnection, void (*)(DBusConnection *)> connection(dbus_bus_get_private(DBUS_BUS_SESSION, &error), [](DBusConnection *c) {
        if (c) {
            dbus_connection_close(c);
            dbus_connection_unref(c);
        }
    });
    if (!connection || !dbus_bus_name_has_owner(connection.get(), serviceName, &error)) {
        dbus_error_free(&error);
        return std::nullopt;
    }

    // The object path KDBusService registers: the service name with dots as slashes.
    const QByteArray path = '/' + QByteArray(serviceName).replace('.', '/');
    const std::unique_ptr<DBusMessage, void (*)(DBusMessage *)> call(
        dbus_message_new_method_call(serviceName, path.constData(), "org.kde.KDBusService", "CommandLine"),
        dbus_message_unref);
    if (!call) {
        return std::nullopt;
    }

    // Arguments as the running instance's QCommandLineParser expects them, program name first.
    // Strings must be valid UTF-8 for D-Bus; Qt converts from the locale's encoding, as KDBusService does.
    DBusMessageIter args;
    DBusMessageIter array;
    dbus_message_iter_init_append(call.get(), &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &array);
    for (int i = 0; i < argc; ++i) {
        const QByteArray argument = QString::fromLocal8Bit(argv[i]).toUtf8();
        const char *data = argument.constData();
        dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &data);
    }
    dbus_message_iter_close_container(&args, &array);

    const QByteArray workingDir = QDir::currentPath().toUtf8();
    const char *workingDirData = workingDir.constData();
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &workingDirData);

    // The launcher's activation token lets the compositor focus the already-running window.
    DBusMessageIter platformData;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &platformData);
    if (const char *token = std::getenv("XDG_ACTIVATION_TOKEN")) {
        appendString(&platformData, "activation-token", token);
    }
    if (const char *startupId = std::getenv("DESKTOP_STARTUP_ID")) {
        appendString(&platformData, "desktop-startup-id", startupId);
    }
    dbus_message_iter_close_container(&args, &platformData);

    const std::unique_ptr<DBusMessage, void (*)(DBusMessage *)> reply(dbus_connection_send_with_reply_and_block(connection.get(), call.get(), 5000, &error),
                                                                      dbus_message_unref);
    dbus_int32_t status = 0;
    if (!reply || !dbus_message_get_args(reply.get(), &error, DBUS_TYPE_INT32, &status, DBUS_TYPE_INVALID)) {
        dbus_error_free(&error);
        return std::nullopt; // e.g. the instance is just quitting: become the preview instead
    }
    return status;
}
