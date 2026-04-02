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
The application source code is organized into a separate directory within this repository (e.g., `Sources/` or `src/`). 

**Important Configuration:** Before compiling, you must open the relevant source file and update the following values with your own details:
* **Authentication:** Your SIP username, password, and SIP domain.
* **Destination:** The target SIP URI or phone number you wish to call.

## ⚙️ Step 5: Compilation and Execution
Navigate into the directory containing your source code. We bypass standard `pkg-config` tooling because Ubuntu uses a custom naming convention for the library (`liblinphone-dev`). Link the library explicitly using `-llinphone`.

**Compile:**
```bash
# Example compilation command using gcc
gcc your_source_file.c -o autocall -llinphone
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
