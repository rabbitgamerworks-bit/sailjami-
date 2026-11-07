/*
 *  Copyright (C) 2026 SailJami contributors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "jamibackend.h"

#include "conversationlistmodel.h"
#include "messagemodel.h"

#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QPointer>
#include <QRegularExpression>

#include <jami/jami.h>
#include <jami/account_const.h>
#include <jami/configurationmanager_interface.h>
#include <jami/conversation_interface.h>

namespace {

QString qs(const std::string& s)
{
    return QString::fromStdString(s);
}

std::string ss(const QString& s)
{
    return s.toStdString();
}

QVariantMap toVariantMap(const std::map<std::string, std::string>& m)
{
    QVariantMap out;
    for (const auto& kv : m)
        out.insert(qs(kv.first), qs(kv.second));
    return out;
}

QVariantMap swarmToVariant(const libjami::SwarmMessage& msg)
{
    QVariantMap out;
    out.insert("id", qs(msg.id));
    out.insert("type", qs(msg.type));
    out.insert("body", toVariantMap(msg.body));
    return out;
}

/** Extract the FN (formatted name) field of a vCard. */
QString vcardDisplayName(const QString& vcard)
{
    static const QRegularExpression re(QStringLiteral("^FN[^:]*:(.*)$"),
                                       QRegularExpression::MultilineOption);
    auto m = re.match(vcard);
    return m.hasMatch() ? m.captured(1).trimmed() : QString();
}

/** Run a functor on the thread of `ctx` (the Qt main thread for the backend). */
template<typename F>
void post(QObject* ctx, F&& f)
{
    QPointer<QObject> guard(ctx);
    QMetaObject::invokeMethod(
        ctx,
        [guard, f = std::forward<F>(f)]() mutable {
            if (guard)
                f();
        },
        Qt::QueuedConnection);
}

} // namespace

JamiBackend::JamiBackend(QObject* parent)
    : QObject(parent)
    , m_conversations(new ConversationListModel(this))
    , m_messages(new MessageModel(this))
{}

JamiBackend::~JamiBackend()
{
    stop();
}

QString JamiBackend::version() const
{
    return QString::fromUtf8(libjami::version());
}

bool JamiBackend::start()
{
    if (m_started)
        return true;
    // Text-only first iteration: no audio/video devices are opened.
    auto flags = static_cast<libjami::InitFlag>(libjami::LIBJAMI_FLAG_NO_LOCAL_MEDIA
                                                | libjami::LIBJAMI_FLAG_CONSOLE_LOG);
    if (!libjami::init(flags)) {
        qWarning() << "libjami init failed";
        return false;
    }
    registerSignals();
    if (!libjami::start()) {
        qWarning() << "libjami start failed";
        libjami::fini();
        return false;
    }
    m_started = true;
    selectAccount();
    m_ready = true;
    emit readyChanged();
    return true;
}

void JamiBackend::stop()
{
    if (!m_started)
        return;
    m_started = false;
    libjami::unregisterSignalHandlers();
    libjami::fini();
}

void JamiBackend::registerSignals()
{
    using namespace libjami;
    using std::string;
    std::map<string, std::shared_ptr<CallbackWrapperBase>> handlers;

    handlers.insert(exportable_callback<ConfigurationSignal::AccountsChanged>([this]() {
        post(this, [this] { onAccountsChanged(); });
    }));
    handlers.insert(exportable_callback<ConfigurationSignal::RegistrationStateChanged>(
        [this](const string& accountId, const string& state, int, const string&) {
            post(this, [this, a = qs(accountId), s = qs(state)] { onRegistrationStateChanged(a, s); });
        }));
    handlers.insert(exportable_callback<ConfigurationSignal::VolatileDetailsChanged>(
        [this](const string& accountId, const std::map<string, string>& details) {
            post(this, [this, a = qs(accountId), d = toVariantMap(details)] {
                onVolatileDetailsChanged(a, d);
            });
        }));
    handlers.insert(exportable_callback<ConfigurationSignal::ProfileReceived>(
        [this](const string& accountId, const string& from, const string& vcard) {
            post(this, [this, a = qs(accountId), f = qs(from), v = qs(vcard)] {
                onProfileReceived(a, f, v);
            });
        }));
    handlers.insert(exportable_callback<ConversationSignal::ConversationReady>(
        [this](const string& accountId, const string& convId) {
            post(this, [this, a = qs(accountId), c = qs(convId)] { onConversationReady(a, c); });
        }));
    handlers.insert(exportable_callback<ConversationSignal::ConversationRemoved>(
        [this](const string& accountId, const string& convId) {
            post(this, [this, a = qs(accountId), c = qs(convId)] { onConversationRemoved(a, c); });
        }));
    handlers.insert(exportable_callback<ConversationSignal::ConversationRequestReceived>(
        [this](const string& accountId, const string& convId, std::map<string, string> meta) {
            post(this, [this, a = qs(accountId), c = qs(convId), m = toVariantMap(meta)] {
                onConversationRequest(a, c, m);
            });
        }));
    handlers.insert(exportable_callback<ConversationSignal::SwarmLoaded>(
        [this](uint32_t,
               const string& accountId,
               const string& convId,
               std::vector<SwarmMessage> messages,
               const std::vector<string>&) {
            QVariantList list;
            for (const auto& m : messages)
                list.append(swarmToVariant(m));
            post(this, [this, a = qs(accountId), c = qs(convId), list] { onSwarmLoaded(a, c, list); });
        }));
    handlers.insert(exportable_callback<ConversationSignal::SwarmMessageReceived>(
        [this](const string& accountId, const string& convId, const SwarmMessage& msg) {
            post(this, [this, a = qs(accountId), c = qs(convId), m = swarmToVariant(msg)] {
                onSwarmMessage(a, c, m, false);
            });
        }));
    handlers.insert(exportable_callback<ConversationSignal::SwarmMessageUpdated>(
        [this](const string& accountId, const string& convId, const SwarmMessage& msg) {
            post(this, [this, a = qs(accountId), c = qs(convId), m = swarmToVariant(msg)] {
                onSwarmMessage(a, c, m, true);
            });
        }));
    handlers.insert(exportable_callback<ConversationSignal::ConversationMemberEvent>(
        [this](const string& accountId, const string& convId, const string&, int) {
            post(this, [this, a = qs(accountId), c = qs(convId)] {
                if (a == m_accountId)
                    updateConversation(c);
            });
        }));

    registerSignalHandlers(handlers);
}

void JamiBackend::selectAccount()
{
    QString selected;
    for (const auto& id : libjami::getAccountList()) {
        auto details = libjami::getAccountDetails(id);
        if (details[libjami::Account::ConfProperties::TYPE] == libjami::Account::ProtocolNames::RING) {
            selected = qs(id);
            break;
        }
    }
    if (selected == m_accountId) {
        refreshAccountDetails();
        return;
    }
    m_accountId = selected;
    m_registrationState.clear();
    m_messages->reset({}, {});
    m_currentConversation.clear();
    refreshAccountDetails();
    reloadConversations();
    emit accountChanged();
    emit registrationStateChanged();
}

void JamiBackend::refreshAccountDetails()
{
    if (m_accountId.isEmpty()) {
        m_jamiId.clear();
        m_displayName.clear();
        m_bootstrapHosts.clear();
        return;
    }
    const std::string id = ss(m_accountId);
    auto details = libjami::getAccountDetails(id);
    auto vol = libjami::getVolatileAccountDetails(id);
    using namespace libjami::Account;
    m_jamiId = qs(details[ConfProperties::USERNAME]);
    m_displayName = qs(details[ConfProperties::DISPLAYNAME]);
    m_bootstrapHosts = qs(details[ConfProperties::HOSTNAME]);
    const QString state = qs(vol[VolatileProperties::Registration::STATUS]);
    if (!state.isEmpty() && state != m_registrationState) {
        m_registrationState = state;
        emit registrationStateChanged();
    }
    emit accountChanged();
}

QString JamiBackend::peerUri(const QString& conversationId) const
{
    const auto members = libjami::getConversationMembers(ss(m_accountId), ss(conversationId));
    for (const auto& m : members) {
        auto it = m.find("uri");
        if (it != m.end() && qs(it->second) != m_jamiId)
            return qs(it->second);
    }
    return {};
}

QString JamiBackend::titleFor(const QString& conversationId) const
{
    auto infos = libjami::conversationInfos(ss(m_accountId), ss(conversationId));
    QString title = qs(infos["title"]);
    if (!title.isEmpty())
        return title;
    const QString peer = peerUri(conversationId);
    if (peer.isEmpty())
        return conversationId.left(8);
    const QString name = m_peerNames.value(peer).toString();
    return name.isEmpty() ? peer : name;
}

void JamiBackend::updateConversation(const QString& conversationId)
{
    ConversationListModel::Item item;
    item.id = conversationId;
    item.peer = peerUri(conversationId);
    item.title = titleFor(conversationId);
    item.isRequest = false;
    m_conversations->upsert(item);
    if (m_currentConversation == conversationId)
        m_messages->setTitle(item.title);
}

void JamiBackend::reloadConversations()
{
    m_conversations->clear();
    if (m_accountId.isEmpty())
        return;
    const std::string acc = ss(m_accountId);
    for (const auto& c : libjami::getConversations(acc)) {
        updateConversation(qs(c));
        // Fetch the last message for the smartlist preview
        libjami::loadConversation(acc, c, "", 1);
    }
    for (const auto& r : libjami::getConversationRequests(acc))
        onConversationRequest(m_accountId, qs(r.at("id")), toVariantMap(r));
}

void JamiBackend::createAccount(const QString& displayName, const QString& password, const QString& bootstrap)
{
    using namespace libjami::Account;
    auto details = libjami::getAccountTemplate(ProtocolNames::RING);
    details[ConfProperties::TYPE] = ProtocolNames::RING;
    details[ConfProperties::DISPLAYNAME] = ss(displayName);
    details[ConfProperties::ALIAS] = ss(displayName.isEmpty() ? QStringLiteral("Jami") : displayName);
    details[ConfProperties::UPNP_ENABLED] = "true";
    if (!password.isEmpty())
        details[ConfProperties::ARCHIVE_PASSWORD] = ss(password);
    if (!bootstrap.trimmed().isEmpty()) {
        // A private bootstrap means a private (test) network: don't publish to
        // the public name server or the public DHT proxy.
        details[ConfProperties::HOSTNAME] = ss(bootstrap.trimmed());
        details[ConfProperties::PROXY_ENABLED] = "false";
        details[ConfProperties::UPNP_ENABLED] = "false";
        details[ConfProperties::TURN::ENABLED] = "false";
    }
    const std::string id = libjami::addAccount(details);
    if (id.empty()) {
        emit accountCreationFailed(tr("libjami refused to create the account"));
        return;
    }
    m_accountId = qs(id);
    m_registrationState = QString::fromLatin1(States::INITIALIZING);
    refreshAccountDetails();
    reloadConversations();
    emit accountChanged();
    emit registrationStateChanged();
}

void JamiBackend::removeAccount()
{
    if (m_accountId.isEmpty())
        return;
    libjami::removeAccount(ss(m_accountId));
    m_accountId.clear();
    selectAccount();
}

void JamiBackend::setDisplayName(const QString& name)
{
    if (m_accountId.isEmpty())
        return;
    std::map<std::string, std::string> details;
    details[libjami::Account::ConfProperties::DISPLAYNAME] = ss(name);
    libjami::setAccountDetails(ss(m_accountId), details);
    libjami::updateProfile(ss(m_accountId), ss(name), "", "", "", 0);
    refreshAccountDetails();
}

void JamiBackend::setBootstrapHosts(const QString& hosts)
{
    if (m_accountId.isEmpty())
        return;
    std::map<std::string, std::string> details;
    details[libjami::Account::ConfProperties::HOSTNAME] = ss(hosts.trimmed());
    libjami::setAccountDetails(ss(m_accountId), details);
    refreshAccountDetails();
}

bool JamiBackend::isValidJamiId(const QString& id)
{
    static const QRegularExpression re(QStringLiteral("^[0-9a-fA-F]{40}$"));
    return re.match(id.trimmed()).hasMatch();
}

bool JamiBackend::addContact(const QString& jamiId)
{
    const QString uri = jamiId.trimmed().toLower();
    if (m_accountId.isEmpty() || !isValidJamiId(uri) || uri == m_jamiId)
        return false;
    // Same as the Android client: adding a contact creates a one-to-one swarm and
    // sends a conversation request to the peer.
    libjami::addContact(ss(m_accountId), ss(uri));
    return true;
}

void JamiBackend::acceptRequest(const QString& conversationId)
{
    libjami::acceptConversationRequest(ss(m_accountId), ss(conversationId));
}

void JamiBackend::declineRequest(const QString& conversationId)
{
    libjami::declineConversationRequest(ss(m_accountId), ss(conversationId));
    m_conversations->remove(conversationId);
}

void JamiBackend::removeConversation(const QString& conversationId)
{
    libjami::removeConversation(ss(m_accountId), ss(conversationId));
    m_conversations->remove(conversationId);
    if (m_currentConversation == conversationId)
        closeConversation();
}

void JamiBackend::openConversation(const QString& conversationId)
{
    m_currentConversation = conversationId;
    const auto* item = m_conversations->find(conversationId);
    m_messages->reset(conversationId, item ? item->title : conversationId.left(8));
    m_conversations->markRead(conversationId);
    libjami::loadConversation(ss(m_accountId), ss(conversationId), "", 64);
}

void JamiBackend::closeConversation()
{
    m_currentConversation.clear();
    m_messages->reset({}, {});
}

void JamiBackend::sendMessage(const QString& text)
{
    const QString body = text.trimmed();
    if (body.isEmpty() || m_currentConversation.isEmpty())
        return;
    libjami::sendMessage(ss(m_accountId), ss(m_currentConversation), ss(body), "");
}

void JamiBackend::copyToClipboard(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
}

// ---------------------------------------------------------------------------
// Signal handlers (main thread)

void JamiBackend::onAccountsChanged()
{
    selectAccount();
}

void JamiBackend::onRegistrationStateChanged(const QString& accountId, const QString& state)
{
    if (accountId != m_accountId)
        return;
    if (m_registrationState != state) {
        m_registrationState = state;
        emit registrationStateChanged();
    }
    // The Jami ID becomes available once the account archive is generated.
    if (m_jamiId.isEmpty())
        refreshAccountDetails();
}

void JamiBackend::onVolatileDetailsChanged(const QString& accountId, const QVariantMap& details)
{
    if (accountId != m_accountId)
        return;
    const QString state = details.value(libjami::Account::VolatileProperties::Registration::STATUS).toString();
    if (!state.isEmpty())
        onRegistrationStateChanged(accountId, state);
}

void JamiBackend::onConversationReady(const QString& accountId, const QString& conversationId)
{
    if (accountId != m_accountId)
        return;
    updateConversation(conversationId);
    libjami::loadConversation(ss(m_accountId), ss(conversationId), "", 1);
}

void JamiBackend::onConversationRemoved(const QString& accountId, const QString& conversationId)
{
    if (accountId != m_accountId)
        return;
    m_conversations->remove(conversationId);
    if (m_currentConversation == conversationId)
        closeConversation();
}

void JamiBackend::onConversationRequest(const QString& accountId,
                                        const QString& conversationId,
                                        const QVariantMap& metadata)
{
    if (accountId != m_accountId)
        return;
    ConversationListModel::Item item;
    item.id = conversationId;
    item.peer = metadata.value("from").toString();
    const QString name = m_peerNames.value(item.peer).toString();
    item.title = !name.isEmpty() ? name
                 : !metadata.value("title").toString().isEmpty() ? metadata.value("title").toString()
                 : item.peer;
    item.isRequest = true;
    item.lastMessage = tr("Invitation received");
    bool ok = false;
    qint64 received = metadata.value("received").toString().toLongLong(&ok);
    item.lastTime = ok ? QDateTime::fromMSecsSinceEpoch(received * 1000) : QDateTime::currentDateTime();
    m_conversations->upsert(item);
}

void JamiBackend::onSwarmLoaded(const QString& accountId, const QString& conversationId, const QVariantList& messages)
{
    if (accountId != m_accountId)
        return;
    for (const auto& v : messages) {
        MessageModel::Item item;
        if (!MessageModel::fromSwarm(v.toMap(), m_jamiId, item))
            continue;
        if (conversationId == m_currentConversation)
            m_messages->upsert(item);
        if (item.type == QLatin1String("text"))
            m_conversations->setLastMessage(conversationId, item.body, item.time, false);
    }
}

void JamiBackend::onSwarmMessage(const QString& accountId,
                                 const QString& conversationId,
                                 const QVariantMap& message,
                                 bool updated)
{
    if (accountId != m_accountId)
        return;
    MessageModel::Item item;
    if (!MessageModel::fromSwarm(message, m_jamiId, item))
        return;
    const bool isCurrent = conversationId == m_currentConversation;
    if (isCurrent)
        m_messages->upsert(item);
    if (item.type == QLatin1String("text") && !updated)
        m_conversations->setLastMessage(conversationId, item.body, item.time, !isCurrent && !item.outgoing);
    if (!m_conversations->find(conversationId))
        updateConversation(conversationId);
}

void JamiBackend::onProfileReceived(const QString& accountId, const QString& peer, const QString& vcard)
{
    if (accountId != m_accountId)
        return;
    const QString name = vcardDisplayName(vcard);
    if (name.isEmpty())
        return;
    m_peerNames.insert(peer, name);
    m_conversations->setTitleForPeer(peer, name);
    if (const auto* item = m_conversations->find(m_currentConversation))
        m_messages->setTitle(item->title);
}
