# SIP-Dialer

***

# Linphone SIP Autocaller (Dockerized Ubuntu 24.04 with Hardware Audio)

This repository contains a C-based application that uses the Linphone API to automatically initiate a SIP call to a mobile device. 

Uniquely, this project runs inside an isolated **Ubuntu 24.04 Docker container** but utilizes specific flags and configurations to break through Docker's isolation, allowing the container to utilize the host machine's physical USB microphone and direct network connection for live, two-way audio.

## 🚀 Prerequisites
* A Linux host machine (tested on Ubuntu).
* Docker installed.
* A physical microphone connected to the host.
* A free SIP account (e.g., from `sip.linphone.org`) or a SIP trunk provider.

## 🛠️ Step 1: Launching the Docker Container
To allow the container to route SIP traffic and hear physical hardware, we must launch it with specific network and device flags. 

Run this command on your host machine:
```bash
docker run -it --net=host --device /dev/snd --group-add audio --name linphone-terminal ubuntu:24.04 /bin/bash
```

**What these flags do:**
* `--net=host`: Bypasses Docker's virtual network bridge. SIP requires complex UDP/TCP handshakes that fail behind Docker's NAT. This puts the container directly on your host's network.
* `--device /dev/snd`: Physically maps your host computer's sound cards (ALSA) into the container.
* `--group-add audio`: Grants the container the correct Linux permissions to interact with the microphone.

## 📦 Step 2: Environment Setup & Dependencies
Once inside the container, install the necessary compiler tools and Linphone libraries. We also install the CLI version of Linphone to force the system to download required background data files.

```bash
apt update 
DEBIAN_FRONTEND=noninteractive apt install -y gcc pkg-config liblinphone-dev nano alsa-utils linphone-cli linphone-common
```

Verify that the container can successfully see your physical microphone by running:
```bash
arecord -l
```
*(You should see your hardware listed, e.g., `card 1: Audio [C-Media(R) Audio]`)*

## 📂 Step 3: Fixing the Database Pathing
Minimal Ubuntu Docker images do not contain standard hidden user directories. When the Linphone engine initializes, it attempts to write its SQLite database files to `/root/.local/share/linphone/`. If these folders do not exist, the program will crash with a `Segmentation fault (core dumped)`.

Create the required folder structure manually:
```bash
mkdir -p /root/.local/share/linphone /root/.config/linphone
```

## 💻 Step 4: The Application Code
Create the C file:
```bash
nano autocall.c
```

Paste the following code. **Note:** You must update the `LinphoneAuthInfo` and the `linphone_core_invite` string with your specific SIP credentials and target destination.

```c
#include <linphone/core.h>
#include <unistd.h>

int main() {
    // 1. Initialize Core
    LinphoneCore *lc = linphone_factory_create_core(linphone_factory_get(), NULL, NULL, NULL);

    // 2. Hardware Configuration
    // Disable video, but LEAVE AUDIO ENABLED to use the physical microphone passed through via Docker
    linphone_core_enable_video_capture(lc, FALSE);
    linphone_core_enable_video_display(lc, FALSE);
    
    // 3. Prevent std::logic_error crashes
    // Linphone's C++ backend throws a fatal error if passed NULL for databases. We use empty strings instead.
    linphone_core_set_chat_database_path(lc, "");
    linphone_core_set_call_logs_database_path(lc, "");

    // 4. Authentication
    // Replace with your Username, Password, and SIP Domain
    LinphoneAuthInfo *auth = linphone_auth_info_new(
        "your_username", NULL, "your_password", NULL, NULL, "sip.linphone.org"
    );
    linphone_core_add_auth_info(lc, auth);
    linphone_auth_info_unref(auth);

    // 5. Network Routing & Proxy
    LinphoneProxyConfig *proxy = linphone_core_create_proxy_config(lc);
    LinphoneAddress *addr = linphone_address_new("sip:your_username@sip.linphone.org");
    linphone_proxy_config_set_identity_address(proxy, addr);
    linphone_proxy_config_set_server_addr(proxy, "sip:sip.linphone.org;transport=tls");
    linphone_proxy_config_enable_register(proxy, TRUE);
    
    linphone_core_add_proxy_config(lc, proxy);
    linphone_core_set_default_proxy_config(lc, proxy);
    linphone_address_unref(addr);

    // 6. Wait for TLS Handshake and Server Registration
    for(int i = 0; i < 120; i++) {
        linphone_core_iterate(lc);
        usleep(50000); 
    }

    // 7. Initiate the Call
    // Replace with the destination SIP URI or Phone Number
    linphone_core_invite(lc, "sip:target_user@sip.linphone.org");

    // 8. Keep the application alive to process the audio stream
    while (1) {
        linphone_core_iterate(lc);
        usleep(50000);
    }

    return 0; 
}
```

## ⚙️ Step 5: Compilation and Execution
We bypass standard `pkg-config` tooling because Ubuntu uses a custom naming convention for the library (`liblinphone-dev`). We link the library explicitly using `-llinphone`.

**Compile:**
```bash
gcc autocall.c -o autocall -llinphone
```

**Run:**
```bash
./autocall
```

When the target mobile device rings, answer it. You will now have a live, two-way audio stream utilizing the host machine's hardware from inside the container.

---

## 🔍 Troubleshooting & Known Architectural Quirks

During the development of this containerized environment, several specific roadblocks were solved:

* **`basic_string: construction from null is not valid` (std::logic_error):**
    Linphone's underlying C++ engine is strictly typed. Attempting to disable the chat or call log databases by passing `NULL` to the setter functions will instantly abort the program. Passing an empty string (`""`) safely bypasses this requirement.
* **`Unable to load Identity Address grammar` (Belr Parser Crash):**
    Ubuntu's minimal Docker images strip out "Recommended" packages to save space. Without the `linphone-common` text files, the `belr` parser cannot understand what a `sip:...` URI looks like and crashes. Explicitly installing `linphone-cli` pulls down the required parsing grammars.
* **ALSA Period Size Warnings (`Using 170 instead of 256`):**
    These warnings are **not errors**. Because you are mapping hardware through a container, Linphone is actively adjusting its audio buffer sizes (resampling) to match the exact hardware specifications of your physical USB microphone. This indicates the audio stream is successfully running.
