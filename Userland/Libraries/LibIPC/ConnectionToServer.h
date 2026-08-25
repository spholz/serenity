/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibCore/Environment.h>
#include <LibCore/SessionManagement.h>
#include <LibIPC/Connection.h>

namespace IPC {

#define IPC_CLIENT_CONNECTION(klass, socket_path)                                                                    \
    C_OBJECT_ABSTRACT(klass)                                                                                         \
public:                                                                                                              \
    template<typename Klass = klass, class... Args>                                                                  \
    static ErrorOr<NonnullRefPtr<klass>> try_create(Args&&... args)                                                  \
    {                                                                                                                \
        auto forced_session_id = forced_session_id_from_environment();                                               \
        auto parsed_socket_path = TRY(Core::SessionManagement::parse_path_with_sid(socket_path, forced_session_id)); \
        auto socket = TRY(Core::LocalSocket::connect(move(parsed_socket_path)));                                     \
        /* We want to rate-limit our clients */                                                                      \
        TRY(socket->set_blocking(true));                                                                             \
                                                                                                                     \
        return adopt_nonnull_ref_or_enomem(new (nothrow) Klass(move(socket), forward<Args>(args)...));               \
    }

template<typename ClientEndpoint, typename ServerEndpoint>
class ConnectionToServer : public IPC::Connection<ClientEndpoint, ServerEndpoint>
    , public ClientEndpoint::Stub
    , public ServerEndpoint::template Proxy<ClientEndpoint> {
public:
    using ClientStub = typename ClientEndpoint::Stub;
    using IPCProxy = typename ServerEndpoint::template Proxy<ClientEndpoint>;

    ConnectionToServer(ClientStub& local_endpoint, NonnullOwnPtr<Core::LocalSocket> socket)
        : Connection<ClientEndpoint, ServerEndpoint>(local_endpoint, move(socket))
        , ServerEndpoint::template Proxy<ClientEndpoint>(*this, {})
    {
    }

    virtual void die() override
    {
        // Override this function if you don't want your app to exit if it loses the connection.
        exit(0);
    }

protected:
    static Optional<pid_t> forced_session_id_from_environment()
    {
        auto forced_session_id_string = Core::Environment::get("IPC_SESSION_ID"sv);
        if (!forced_session_id_string.has_value())
            return {};

        auto forced_session_id = forced_session_id_string.value().to_number<pid_t>();
        if (!forced_session_id.has_value()) {
            dbgln("LibIPC: Invalid session ID from environment: \"{}\"", forced_session_id_string);
            return {};
        }

        return forced_session_id;
    }
};

}
