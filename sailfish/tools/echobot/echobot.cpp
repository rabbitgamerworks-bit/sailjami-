/*
 *  jami-echobot: a minimal Jami peer used to test SailJami.
 *
 *  - creates (or reuses) a Jami account whose DHT node listens on a fixed UDP
 *    port, so it can serve as the bootstrap node of a private test network;
 *  - accepts every conversation request;
 *  - answers every text message with "Echo: <message>".
 *
 *  Usage: jami-echobot [--port 4222] [--bootstrap host:port] [--name "Echo bot"]
 *  The bot's Jami ID is printed on stdout ("JAMI_ID <id>") once known.
 *
 *  Copyright (C) 2026 SailJami contributors
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <jami/jami.h>
#include <jami/account_const.h>
#include <jami/configurationmanager_interface.h>
#include <jami/conversation_interface.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace {

std::atomic_bool running {true};
std::mutex outMutex;

template<typename... Args>
void log(Args&&... args)
{
    std::lock_guard<std::mutex> lk(outMutex);
    ((std::cout << std::forward<Args>(args)), ...);
    std::cout << std::endl;
}

std::string findAccount()
{
    for (const auto& id : libjami::getAccountList()) {
        auto d = libjami::getAccountDetails(id);
        if (d[libjami::Account::ConfProperties::TYPE] == libjami::Account::ProtocolNames::RING)
            return id;
    }
    return {};
}

} // namespace

int main(int argc, char** argv)
{
    std::string port = "4222";
    std::string bootstrap;
    std::string name = "Echo bot";
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "--port"))
            port = argv[i + 1];
        else if (!strcmp(argv[i], "--bootstrap"))
            bootstrap = argv[i + 1];
        else if (!strcmp(argv[i], "--name"))
            name = argv[i + 1];
    }

    std::signal(SIGINT, [](int) { running = false; });
    std::signal(SIGTERM, [](int) { running = false; });

    if (!libjami::init(libjami::LIBJAMI_FLAG_NO_LOCAL_MEDIA))
        return 1;

    std::string accountId;
    std::set<std::string> answered; // message ids already handled
    std::mutex mtx;

    using namespace libjami;
    std::map<std::string, std::shared_ptr<CallbackWrapperBase>> handlers;
    handlers.insert(exportable_callback<ConfigurationSignal::RegistrationStateChanged>(
        [&](const std::string& acc, const std::string& state, int, const std::string&) {
            log("STATE ", state);
            if (state == Account::States::REGISTERED) {
                auto d = getAccountDetails(acc);
                log("JAMI_ID ", d[Account::ConfProperties::USERNAME]);
            }
        }));
    handlers.insert(exportable_callback<ConversationSignal::ConversationRequestReceived>(
        [&](const std::string& acc, const std::string& conv, std::map<std::string, std::string> meta) {
            log("REQUEST ", conv, " from ", meta["from"]);
            acceptConversationRequest(acc, conv);
        }));
    handlers.insert(exportable_callback<ConversationSignal::ConversationReady>(
        [&](const std::string& acc, const std::string& conv) {
            log("READY ", conv);
            sendMessage(acc, conv, "Hello from the Jami echo bot! Send me a message.", "");
        }));
    handlers.insert(exportable_callback<ConversationSignal::SwarmMessageReceived>(
        [&](const std::string& acc, const std::string& conv, const SwarmMessage& msg) {
            if (msg.type != "text/plain")
                return;
            auto author = msg.body.count("author") ? msg.body.at("author") : "";
            auto self = getAccountDetails(acc)[Account::ConfProperties::USERNAME];
            auto text = msg.body.count("body") ? msg.body.at("body") : "";
            log("MESSAGE ", conv, " ", author.substr(0, 8), ": ", text);
            if (author == self)
                return;
            {
                std::lock_guard<std::mutex> lk(mtx);
                if (!answered.insert(msg.id).second)
                    return;
            }
            sendMessage(acc, conv, "Echo: " + text, msg.id);
        }));
    registerSignalHandlers(handlers);

    if (!libjami::start())
        return 1;

    accountId = findAccount();
    if (accountId.empty()) {
        auto details = getAccountTemplate(Account::ProtocolNames::RING);
        details[Account::ConfProperties::TYPE] = Account::ProtocolNames::RING;
        details[Account::ConfProperties::ALIAS] = name;
        details[Account::ConfProperties::DISPLAYNAME] = name;
        details[Account::ConfProperties::DHT_PORT] = port;
        details[Account::ConfProperties::HOSTNAME] = bootstrap;
        details[Account::ConfProperties::UPNP_ENABLED] = "false";
        details[Account::ConfProperties::PROXY_ENABLED] = "false";
        details[Account::ConfProperties::TURN::ENABLED] = "false";
        accountId = addAccount(details);
        log("CREATED ", accountId);
    } else {
        auto d = getAccountDetails(accountId);
        log("LOADED ", accountId, " JAMI_ID ", d[Account::ConfProperties::USERNAME]);
    }

    while (running)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    libjami::unregisterSignalHandlers();
    libjami::fini();
    return 0;
}
