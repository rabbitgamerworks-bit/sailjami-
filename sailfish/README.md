# SailJami – Jami for Sailfish OS

A port of the Jami client to Sailfish OS. The Android user interface (Kotlin,
`../jami-android`) cannot run on Sailfish, so the client is rewritten with the
native **Silica** toolkit (QML) on top of **libjami**, the same Jami daemon
library used by every Jami client (`../daemon`).

```
 Silica QML pages (qml/)          ← rewritten from the Android screens
        │
 JamiBackend + models (src/)      ← Qt 5.6 C++ facade, libjami signals → Qt main thread
        │
 libjami (../daemon)              ← unchanged Jami daemon, built for Sailfish
```

## Status (first iteration)

| Android screen | Sailfish page | Status |
|---|---|---|
| Account wizard (`frag_acc_home_create`, `frag_acc_jami_create`) | `WelcomePage`, `CreateAccountPage` | ✅ create account, display name, optional password, custom DHT bootstrap |
| Smartlist (`activity_home`) | `ConversationsPage` | ✅ conversations, invitations, unread counters |
| Conversation (`frag_conversation`) | `ConversationPage` | ✅ text messages, accept/decline invitations |
| Add contact (search) | `AddContactPage` | ✅ by Jami ID (name server lookup not yet) |
| Account (`frag_account`) | `AccountPage` | ✅ Jami ID, status, display name, bootstrap, delete |
| Calls, file transfer, groups, link device, name registration | – | ⏳ later |

## Building

Requirements: Sailfish SDK (Docker build engine works) with a
SailfishOS 5.1 target.

1. Fetch the contrib tarballs on a machine with internet access (the build
   engine may be behind a proxy):

   ```sh
   cd daemon/contrib && mkdir -p fetch && cd fetch
   ../bootstrap --host=i486-meego-linux-gnu && make fetch
   ```

2. Install build tools and dev packages into the target:

   ```sh
   sfdk tools package-install SailfishOS-5.1.0.11-i486 \
       ninja nasm yasm git bison flex gettext-devel meson \
       pulseaudio-devel alsa-lib-devel systemd-devel libuuid-devel
   ```

3. Build libjami (contrib + daemon) inside the target:

   ```sh
   sfdk engine exec sb2 -t SailfishOS-5.1.0.11-i486 \
       bash sailfish/scripts/build-libjami.sh
   ```

   This installs libjami to `daemon/install-i386`.

4. Build the RPM:

   ```sh
   cd sailfish
   sfdk config target=SailfishOS-5.1.0.11-i486
   sfdk build
   ```

## Testing with a private network

`tools/echobot` is a tiny Jami peer (also linked against libjami) that can act
as the DHT bootstrap node of a private test network. It accepts every
invitation and echoes every message:

```sh
jami-echobot --port 4222        # prints "JAMI_ID <40 hex chars>"
```

In SailJami, create an account with *DHT bootstrap* set to the bot's address
(e.g. `127.0.0.1:4222` when both run on the same device/emulator), then add the
bot's Jami ID as a contact.

## License

GPL-3.0-or-later, like Jami. "Jami" is a trademark of Savoir-faire Linux Inc.
